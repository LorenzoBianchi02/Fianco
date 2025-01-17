#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <time.h>
#include <string.h>
#include <sys/resource.h>

#include "fianco.h"
#include "server.h"


int debug;

uint64_t hashTable[9][9][2]; //global rand values

//--STATS--//
uint64_t states_visited;
uint64_t states_pruned;
uint64_t TT_prunes; 
uint64_t killer_prunes;
uint64_t null_prunes;
uint64_t collision;
int research[64];

int8_t out_of_time;
uint8_t last_moves_considered;

int main(){
    //increase memory allocation allowance
    struct rlimit lim;
    
    // Set memory lock limit to 8 GB
    lim.rlim_cur = 8L * 1024 * 1024 * 1024;  // Soft limit
    lim.rlim_max = 8L * 1024 * 1024 * 1024;  // Hard limit
    
    if (setrlimit(RLIMIT_MEMLOCK, &lim) == -1) {
        perror("Failed to set memory limit");
        return 1;
    }

    //---------NCURSES---------//
    setlocale(LC_ALL, "");

    initscr();
    start_color();
    use_default_colors();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0);

	mousemask(ALL_MOUSE_EVENTS, NULL);

    init_color(COLOR_GREEN, 619, 365, 118);
    init_color(COLOR_YELLOW, 804, 667, 490);
    init_color(COLOR_MAGENTA, 0, 280, 780);
    
    //NB: be carefull when changing the order
    init_pair(1, COLOR_BLACK, COLOR_YELLOW);    //board color 1
    init_pair(2, COLOR_BLACK, COLOR_GREEN);     //board color 2
    init_pair(4, COLOR_WHITE, COLOR_YELLOW);    //piece selected 1
    init_pair(3, COLOR_WHITE, COLOR_GREEN);     //piece selected 2
    init_pair(6, COLOR_BLACK, COLOR_BLACK);     //turn color 1
    init_pair(5, COLOR_BLACK, COLOR_WHITE);     //turn color 2
    init_pair(7, COLOR_BLACK, COLOR_CYAN);      //last move
    init_pair(8, COLOR_BLACK, COLOR_MAGENTA);

    init_color(COLOR_BLACK, 0, 0, 0);

    MEVENT mevent;


    //---------MAIN---------//
    srand(time(0));
    initRandTable();

    board_t *board = initializeBoard();
    uint8_t fromx, fromy, tox, toy;

    transposition_table_t *transpos_table = (transposition_table_t *)malloc(TT_SIZE * sizeof(transposition_table_t)); //NOTE: this has to be equal to 2^primary key bits

    int human = 1;
    int server = 0, sock;
    int flag;
    value_t res;
    int depth, time_used=0, last_depth=0;
    clock_t start, end, tot_time=0;
    research[0] = 0;

    if(server > 0){
        sock = connect_server();
    }

    int tmp = lookupTT(transpos_table, board->hash);    //FIXME: why am I doing this? (afraid to remove)


    move_t moves;
    getMoves(board, 1, moves);


    while(!checkWin(board)){
        start_turn:

        // erase(); //DEBUG: remove 
        printBoard(board);

        if(board->turn){
            mvchgat(8-toy, 2*tox, 2, A_NORMAL, 8, NULL);
            mvchgat(8-fromy, 2*fromx, 2, A_NORMAL, 7, NULL);
        }
 
        mvprintw(1, 20, "Player's turn:   \n");
        mvchgat(1, 35, 2, A_NORMAL, board->turn%2+5, NULL);

        mvprintw(3, 20, " UNDO \n");
        mvchgat(3, 20, 6, A_NORMAL, 5, NULL);

        move(10, 0);
        printw("LIST WHITE: ");
        printList(board);
        printw("LIST BLACK: ");
        printList(board);
        printw("\n");


        getMoves(board, board->turn%2+1, moves);
        int capt = CAN_CAPT(moves);
        printMoves(moves);
        printw("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"); //lazy (but functional) way to remove old used space bellow
        flag = 1;

        move(6, 20);
        printw("states visited: %lu, PRUNES: standard: %lu, transposition: %lu, killer: %lu", states_visited, states_pruned, TT_prunes, killer_prunes);
        mvprintw(7, 20, "%d seconds (total: %d) (%.0f n/s), collisions: %lu, research: [", time_used, (int)((double)tot_time/CLOCKS_PER_SEC), (float)(states_visited * CLOCKS_PER_SEC)/(end-start), collision);
        for(int r=0; r<13; r++)
            printw("%d ", research[r]);
        printw("]");
        printw(",   (prev depth: %d (%d))", last_depth, last_moves_considered);

        if(board->turn){
            move(8, 20);
            clrtoeol();
            printw("EVALUATION: %d, depth: %d,    considering: %d %d %d %d", res, depth, moves[capt][0][0], moves[capt][0][1], moves[capt][0][2], moves[capt][0][3]);
            
        }

        refresh();


        //check for stalemate
        if(moves[0][0][0] == -1 && moves[1][0][0] == -1)
            break;

        //-----HUMAN-----//
        if(board->turn % 2 + 1 == human){
            //first click
            do{
                getch();
                getmouse(&mevent);
                fromx = mevent.x/2;
                fromy = mevent.y;
                refresh();

                //---UNDO---//
                if(board->turn && fromy == 3 && fromx >= 10 && fromx <= 12){
                    undoMove(board, board->move_history[board->turn-1]);

                    if(board->turn){
                        undoMove(board, board->move_history[board->turn-1]);
                    }

                    goto start_turn; //this is my code, and I shall do what I want!!!
                }

            }while(!boardCoords(&fromx, &fromy) || !PLAYER(fromx, fromy) || PLAYER(fromx, fromy) != board->turn % 2 + 1); //NOTE: this may be the only place where it is checked that the piece picked needs to be of the current player


            mvchgat(8-fromy, fromx*2, 2, A_NORMAL, ((fromx+fromy+1)%2)+3, NULL);

            //second click
            getch();
            getmouse(&mevent);
            tox = mevent.x/2;
            toy = mevent.y;
            refresh();
            boardCoords(&tox, &toy);

            uint8_t tmp_move[4] = {fromx, fromy, tox, toy};

            if(validMove(board, tmp_move)){
                getMoves(board, human, moves);
                printMoves(moves);
                int capt = CAN_CAPT(moves);
                flag=0;

                for(int h=0; moves[capt][h][0] != -1; h++){
                    if(moves[capt][h][0] == fromx && moves[capt][h][1] == fromy && moves[capt][h][2] == tox && moves[capt][h][3] == toy){
                        flag=1;
                        break;
                    }
                }
            }

                
        

            mvchgat(8 - fromy, fromx*2, 2, A_NORMAL, ((fromx+fromy)%2)+1, NULL);
        }


        //-----SERVER--------//
        else if(board->turn % 2 + 1 == server){
            int init_board[9][9];
            recBoard(sock, init_board);
            setBoard(init_board, board);

            board->turn++;
        }

        //------AI-----//
        else{
            // human = human % 2 + 1; //uncomment to only play against yourself
            
            //if you only have one possible move
            if(moves[1][0][0] != -1 && moves[1][1][0] == -1){
                fromx = moves[1][0][0];
                fromy = moves[1][0][1];
                tox = moves[1][0][2];
                toy = moves[1][0][3];
            }else{
                //reset stats stuff
                states_visited = 0;
                states_pruned = 0;
                TT_prunes = 0;
                collision = 0;
                memset(research, 0, sizeof(int) * 64);
                killer_prunes = 0;
                null_prunes = 0;
                memset(board->prune_history, 0, sizeof(int) * 81 * 81);


                //start timer
                start = clock();


                for(int i=0; i<100; i++){
                    board->killer_move[i][0][0] = -1;
                    board->killer_move[i][1][0] = -1;
                }

                //clear TT
                memset(transpos_table, 0, sizeof(transpos_table) * TT_SIZE);



                //time calculation
                out_of_time = 0;

                if(!board->turn || (MAX_TIME - tot_time/CLOCKS_PER_SEC) >= MAX_TIME/5) 
                    time_used = 10;
                else{
                    time_used = (MAX_TIME - tot_time/CLOCKS_PER_SEC)/10;
                    // time_used = 10;
                    if(time_used <= 0)
                        time_used = 1;  //min 1 second
                    if(time_used > 10)  //safety measure
                        time_used = 10;
                }
                
                board->time_out = time(NULL) + time_used;


                value_t old_res;
                int i;
                
                for(i=1; !out_of_time; i++){
                    board->depth = 0;

                    res = negaMaxRoot(board, transpos_table, i, -INF, INF, moves);
                    depth = i;

                    if(res == -INF)
                        res = old_res;

                    old_res = res;

                    move(8, 20);
                    clrtoeol();
                    printw("EVALUATION: %d, depth: %d,    considering: %d %d %d %d", res, i, moves[capt][0][0], moves[capt][0][1], moves[capt][0][2], moves[capt][0][3]);
                    refresh();
                }

                last_depth = i;

                fromx = moves[capt][0][0];
                fromy = moves[capt][0][1];
                tox = moves[capt][0][2];
                toy = moves[capt][0][3];

                end = clock();

                tot_time += end-start;
                
            }
            

            // human = human % 2 + 1;
        }

        //valid move
        //flag indicates if it captures when it has to
        uint8_t tmp_move[4] = {fromx, fromy, tox, toy};
        if(flag && movePiece(board, tmp_move)){
            if(server > 0){
                sendBoard(sock, board);
            }
        }

        refresh();
    }


    //---ENDGAME---//
    erase();
    printBoard(board);
    move(11, 0);
    int winner = checkWin(board);
    if(moves[0][0][0] == -1 && moves[1][0][0] == -1)
        winner = (board->turn + 1) % 2 + 1;
    printw("   PLAYER %d won", winner);
    printw("\n\n   (Press any key to leave)");
    refresh();

    getch();
    endwin();

    free(transpos_table);
    free(board);

    return 0;
}

//initializes the board with the correct pieces, and allocates needed memory
board_t *initializeBoard(){
    board_t *board = (board_t *)malloc(sizeof(board_t));

    // setting up the board
    int init_board[9][9] = 
    {
        {1, 0, 0, 0, 0, 0, 0, 0, 2},
        {1, 1, 0, 0, 0, 0, 0, 2, 2},
        {1, 0, 1, 0, 0, 0, 2, 0, 2},
        {1, 0, 0, 1, 0, 2, 0, 0, 2},
        {1, 0, 0, 0, 0, 0, 0, 0, 2},
        {1, 0, 0, 1, 0, 2, 0, 0, 2},
        {1, 0, 1, 0, 0, 0, 2, 0, 2},
        {1, 1, 0, 0, 0, 0, 0, 2, 2},
        {1, 0, 0, 0, 0, 0, 0, 0, 2}
    };

    // int init_board[9][9] = 
    // {
    //     {0, 0, 0, 0, 0, 0, 0, 0, 0},
    //     {0, 0, 0, 0, 0, 0, 0, 0, 0},
    //     {0, 0, 1, 0, 0, 2, 0, 0, 0},
    //     {0, 0, 0, 0, 0, 2, 0, 2, 0},
    //     {0, 0, 0, 0, 0, 0, 0, 0, 0},
    //     {0, 0, 0, 0, 0, 0, 0, 0, 0},
    //     {0, 0, 0, 0, 0, 0, 0, 0, 0},
    //     {0, 1, 0, 0, 0, 0, 0, 0, 0},
    //     {0, 0, 0, 0, 0, 0, 0, 0, 0}
    // };

    board->piece_list_size[0] = 0;
    board->piece_list_size[1] = 0;

    int player, list_size;

    for(int i=0; i<9; i++){
        for(int j=0; j<9; j++){
                player = init_board[i][j];
                //if piece add to list
                if(player > 0){
                    list_size = board->piece_list_size[player-1];
                    board->piece_list[player-1][list_size][0] = i;
                    board->piece_list[player-1][list_size][1] = j;

                    board->cell[i][j][0] = player; //player
                    board->cell[i][j][1] = list_size;  //which position in list

                    board->piece_list_size[player-1]++;
                    
                }else
                    board->cell[i][j][0] = 0;

        }
    }

    board->piece_list[0][board->piece_list_size[0]][0] = -1;
    board->piece_list[1][board->piece_list_size[1]][0] = -1;

    board->turn = 0;

    //get hash
    board->hash = getHash(board);

    board->depth = 0;

    return board;
}

//prints the board on the screen (currently at the positions 0 0 of stdscr)
void printBoard(board_t * board){
    // erase();
    int i, j;
    move(0, 0);

    for(j=8; j>=0; j--){
        for(i=0; i<9; i++){
                attron(COLOR_PAIR(((i+j)%2)+1));
                if(PLAYER(i, j) == 1)
                    printw("%s", "\u26C0 ");
                else if(PLAYER(i, j) == 2)
                    printw("%s", "\u26C2 ");
                else
                    printw("  ");
                attroff(COLOR_PAIR(((i+j)%2)+1));
            
        }
        printw("\n");
    }

    attroff(COLOR_PAIR(((i+j)%2)+1));

    refresh();
}

//tranforms stdscr coords to [9][9] coords
int boardCoords(uint8_t *x, uint8_t *y){
    if(*x < 0 || *y < 0 || *x > 8 || *y > 8)
        return false;

    *y = 8-*y;
        
    return true;
}



//moves a piece can make
int _moves[2][5][2] = {
    {{0, 1}, {-1, 0}, {1, 0}, {-2, 2}, {2, 2}},
    {{0, -1}, {-1, 0}, {1, 0}, {-2, -2}, {2, -2}}};

//populates the moves matrix [0] has captures and [1] has moves, makes it easier to check if can (has to) capture
int getMoves(board_t *board, int player, move_t moves){
    int size[2] = {0, 0}; //captures and moves
    uint8_t coords[4];
    int i, j, move;

    player--;

    //first check captures
    for(i=0; i<board->piece_list_size[player]; i++){
        coords[0] = board->piece_list[player][i][0];
        coords[1] = board->piece_list[player][i][1];

        for(j=3; j<5; j++){
            coords[2] = coords[0] + _moves[player][j][0];
            coords[3] = coords[1] + _moves[player][j][1];

            if(validMove(board, coords)){
                memcpy(moves[1][size[1]], coords, 4);
                size[1]++;
            }
        }
    }

    //check moves if no captures
    if(!size[0]){
        for(i=0; i<board->piece_list_size[player]; i++){
            coords[0] = board->piece_list[player][i][0];
            coords[1] = board->piece_list[player][i][1];

            for(j=0; j<3; j++){
                coords[2] = coords[0] + _moves[player][j][0];
                coords[3] = coords[1] + _moves[player][j][1];

                if(validMove(board, coords)){
                    memcpy(moves[0][size[0]], coords, 4);
                    size[0]++;
                }
            }
        }
    }

    moves[0][size[0]][0] = -1;
    moves[1][size[1]][0] = -1;

    return i;
}


//returns 0 if invalid, 1 for a normal move, 2 for a capture
int validMove(board_t *board, uint8_t coords[4]){
    uint8_t fromx = coords[0];
    uint8_t fromy = coords[1];
    uint8_t tox = coords[2];
    uint8_t toy = coords[3];
    
    //check if move is inbounds
    if(fromx < 0 || fromy < 0 || tox < 0 || toy < 0 || fromx > 8 || fromy > 8 || tox > 8 || toy > 8)
        return FALSE;

    //no piece in starting position or arriving position already occupied
    if(!PLAYER(fromx, fromy))
        return FALSE;
    if(PLAYER(tox, toy))
        return FALSE;

    int player = PLAYER(fromx, fromy);

    //left right
    if(fromy == toy && abs(fromx - tox) == 1)
        return 1;

    //up down
    if(fromx == tox){
        if(player == 1 && toy - fromy == 1)
            return 1;
        if(player == 2 && toy - fromy == -1)
            return 1;
    }

    //capture
    if(abs(fromx - tox) == 2 && abs(fromy - toy) == 2){
        int signx = (tox - fromx)/2, signy = (toy - fromy)/2;
        if(PLAYER(fromx+signx, fromy+signy) == 0 || PLAYER(fromx+signx, fromy+signy) == PLAYER(fromx, fromy))
            return FALSE;

        if(player == 1 && toy - fromy == 2)
            return 2;
        
        if(player == 2 && toy - fromy == -2)
            return 2;
    }

    return FALSE;
}

//Returns whethers the move has succesfully been done.
//The hash also gets automaticly updated.
//FIXME:It should be garenteed that a piece is present on fromx/fromy (IT ISN'T) (at the moment it is being checked in validMove).
int movePiece(board_t *board, uint8_t coords[4]){
    board->turn++;
    board->depth++;

    int move = validMove(board, coords); //FIXME: remove this from here, for the human it should be in main
    if(!move){
        board->turn--;
        board->depth--;
        return FALSE;
    }

    uint8_t fromx = coords[0];
    uint8_t fromy = coords[1];
    uint8_t tox = coords[2];
    uint8_t toy = coords[3];

    int list_from = PLAYER(fromx, fromy) - 1;
    
    //hash from
    board->hash ^= hashTable[fromx][fromy][list_from];

    if(move == 1){

        //change coords
        board->piece_list[list_from][POSITION(fromx, fromy)][0] = tox;
        board->piece_list[list_from][POSITION(fromx, fromy)][1] = toy;
        
        //move "pointer"
        board->cell[tox][toy][0] = board->cell[fromx][fromy][0];
        board->cell[tox][toy][1] = board->cell[fromx][fromy][1];

        //remove starting pos
        board->cell[fromx][fromy][0] = 0;

    }else if(move == 2){
        int signx = (tox - fromx)/2, signy = (toy - fromy)/2;

        //move capturing piece
        board->piece_list[list_from][POSITION(fromx, fromy)][0] = tox;
        board->piece_list[list_from][POSITION(fromx, fromy)][1] = toy;

        PLAYER(tox, toy) = PLAYER(fromx, fromy);
        POSITION(tox, toy) = POSITION(fromx, fromy);
        PLAYER(fromx, fromy) = 0;

        //remove captured piece (also from list)
        int x = fromx + signx, y = fromy+signy;
        int list_xy = PLAYER(x, y) - 1;
        int oldpos = POSITION(fromx+signx, fromy+signy);

        board->hash ^= hashTable[x][y][list_xy];

        board->piece_list[list_xy][oldpos][0] = board->piece_list[list_xy][board->piece_list_size[list_xy] - 1][0];
        board->piece_list[list_xy][oldpos][1] = board->piece_list[list_xy][board->piece_list_size[list_xy] - 1][1];

        board->cell[board->piece_list[list_xy][oldpos][0]][board->piece_list[list_xy][oldpos][1]][1] = oldpos;
        board->cell[x][y][0] = 0;

        board->piece_list_size[list_xy]--;
    }

    //hash to
    board->hash ^= hashTable[tox][toy][PLAYER(tox, toy) - 1];

    board->move_history[board->turn-1][0] = fromx;
    board->move_history[board->turn-1][1] = fromy;
    board->move_history[board->turn-1][2] = tox;
    board->move_history[board->turn-1][3] = toy;

    return move;
}


//Undos a given move.
//The hash also gets automaticly updated.
//It is guaranteed that this was the last move, and a valid one.
void undoMove(board_t *board, uint8_t coords[4]){
    uint8_t fromx = coords[0];
    uint8_t fromy = coords[1];
    uint8_t tox = coords[2];
    uint8_t toy = coords[3];

    int move;
    if(abs(fromx - tox) == 2)
        move = 2;
    else
        move = 1;

    int tmp;
    //swap fromx and tox
    tmp = tox;
    tox = fromx;
    fromx = tmp;
    //swap formy and toy
    tmp = toy;
    toy = fromy;
    fromy = tmp;

    int list_from = PLAYER(fromx, fromy) - 1;
    
    //hash from
    board->hash ^= hashTable[fromx][fromy][list_from];
    

    if(move == 1){
        //change coords
        board->piece_list[list_from][POSITION(fromx, fromy)][0] = tox;
        board->piece_list[list_from][POSITION(fromx, fromy)][1] = toy;
        
        //move "pointer"
        board->cell[tox][toy][0] = board->cell[fromx][fromy][0];
        board->cell[tox][toy][1] = board->cell[fromx][fromy][1];

        //remove starting pos
        board->cell[fromx][fromy][0] = 0;

    }else if(move == 2){
        int signx = (tox - fromx)/2, signy = (toy - fromy)/2;

        //move capturing piece
        board->piece_list[list_from][POSITION(fromx, fromy)][0] = tox;
        board->piece_list[list_from][POSITION(fromx, fromy)][1] = toy;

        PLAYER(tox, toy) = PLAYER(fromx, fromy);
        POSITION(tox, toy) = POSITION(fromx, fromy);
        PLAYER(fromx, fromy) = 0;

        //re-add captured piece (also from list)
        int x = fromx + signx, y = fromy+signy;
        int list = PLAYER(tox, toy) % 2;
        int oldpos = POSITION(fromx+signx, fromy+signy);


        board->piece_list[list][board->piece_list_size[list]][0] = x;
        board->piece_list[list][board->piece_list_size[list]][1] = y;

        PLAYER(x, y) = list + 1;
        POSITION(x, y) = board->piece_list_size[list];
        
        board->piece_list_size[list]++;

        //hash added piece
        board->hash ^= hashTable[x][y][list];
    }

    //hash to
    board->hash ^= hashTable[tox][toy][PLAYER(tox, toy) - 1];

    board->turn--;
    board->depth--;
}


//main function for AI
value_t negaMax(board_t *board, transposition_table_t *transpos, int depth, int alpha, int beta, uint8_t best[4]){
    states_visited++;

    //TIME-OUT
    if(states_visited << 46 == 0){
        if(time(NULL) >= board->time_out)
            out_of_time = 1;
    }
    if(out_of_time)
        return 0;

    int old_alpha = alpha;
    value_t eval;

    uint32_t key = KEY(board->hash);


    //TRANSOPITION TABLE
    int height = lookupTT(transpos, board->hash);

    if(height >= depth){
        value_t value = transpos[key].value;
        int type = transpos[key].type;

        if(type == EXACT)
            return value;
        if(type == LOWER_BOUND){
            if(value > alpha)
                alpha = value;
        }else if(type == UPPER_BOUND){
            if(value < beta)
                beta = value;
        }

        //forward pruning
        if(alpha >= beta){
            TT_prunes++;

            memcpy(best, transpos[key].moves, 4);

            board->prune_history[best[0]][best[1]][best[2]][best[3]]++;

            return transpos[key].value;
        }
    }


    //LEAF NODE
    if(!depth)
        return evaluate(board);


    value_t score = -INF, value;

    //TRANSPOSITION MOVE
    if(height >= 0){
        uint8_t move[4];
        memcpy(move, transpos[key].moves, 4);

        // first try the TT move
        movePiece(board, move);

        score = -negaMax(board, transpos, depth-1, -beta, -alpha, best);

        undoMove(board, move);

        if(score >= beta){
            memcpy(best, transpos[key].moves, 4);
            TT_prunes++;

            goto done;
        }
    }

    //TERMINAL POSITION
    int terminal = checkWin(board);

    if(terminal){
        if(terminal == 1)
            eval = WIN - board->depth*100;
        else if(terminal == 2)
            eval = LOSS + board->depth*100;
        else
            eval = DRAW;

        return eval;
    }
    
    move_t moves;
    getMoves(board, board->turn%2+1, moves);

    //check stalemate
    if(moves[0][0][0] == -1 && moves[1][0][0] == -1)
        return LOSS + board->depth*100;


    int move = 0;
    int capt = CAN_CAPT(moves);

    //QUIET MOVE
    //if you can capture or only have 1 move, the depth doesn't get decreased
    int one_move = 0;
    if(moves[1][0][0] != -1 || capt)
        one_move = 1;


    //KILLER MOVE
    for(int i=0; i<2; i++){
        if(board->killer_move[board->depth][i][0] < 9){
            //check for valid move
            if(movePiece(board, board->killer_move[board->depth][i])){
                
                value = -negaMax(board, transpos, depth-1, -beta, -alpha, best);
                undoMove(board, board->killer_move[board->depth-1][i]);

                if(value > score)
                    score = value;

                if(score >= beta){
                    memcpy(best, board->killer_move[board->depth][i], 4);
                    killer_prunes++;

                    goto done;
                }
            }
        }
    }

    //HISTORY HEURISTIC
    //reorder moves based on history hueristic, only for up to #NUM_HIST best moves
    if(!capt){
        int max_i, val, val_tmp;
        int i, j;
        uint8_t tmp_move[4];
        for(i=0; i<NUM_HIST, moves[0][i][0]!=-1; i++){
            max_i = i;
            val = moves[0][0][0];

            for(j=i+1; moves[0][j][0]!=-1; j++){
                val_tmp = board->prune_history[moves[0][j][0]][moves[0][j][1]][moves[0][j][2]][moves[0][j][3]];
                if(val_tmp > val){
                    max_i = j;
                    val = val_tmp;
                }
            }

            memcpy(tmp_move, moves[0][max_i], 4);
            memcpy(moves[0][max_i], moves[0][i], 4);
            memcpy(moves[0][i], tmp_move, 4);
        }
    }


    for(int i=0; moves[capt][i][0] != -1; i++){
        //TT move has already been tried
        if(height >= 0 && transpos[key].moves[0] == moves[capt][i][0] && transpos[key].moves[1] == moves[capt][i][1] && transpos[key].moves[2] == moves[capt][i][2] && transpos[key].moves[3] == moves[capt][i][3])
            continue;

        movePiece(board, moves[capt][i]);

        value = -negaMax(board, transpos, depth-(1-one_move), -beta, -alpha, best);

        undoMove(board, moves[capt][i]);

        if(value > score){
            score = value;
            move = i;
        }
        if(score > alpha)
            alpha = score;
        //prune
        if(score >= beta){
            states_pruned++;

            //update killer moves
            memcpy(board->killer_move[board->depth][1], board->killer_move[board->depth][0], 4);
            memcpy(board->killer_move[board->depth][0], moves[capt][i], 4);

            break;
        }
    }

    memcpy(best, moves[capt][move], 4);


    //store in TT
    done:
    int bound = EXACT;
    if(score <= old_alpha) bound = UPPER_BOUND;
    else if(score >= beta) bound = LOWER_BOUND;

    if(height <= depth){
        storeTT(transpos, board->hash, score, bound, best[0], best[1], best[2], best[3], depth);
    }

    //history hueristic
    board->prune_history[best[0]][best[1]][best[2]][best[3]]++;

    return score;
}

value_t negaMaxRoot(board_t *board, transposition_table_t *transpos, int depth, int alpha, int beta, move_t moves){
    states_visited++;

    int i;
    value_t score = -INF, value;
    uint8_t best[4]; //FIXME: not needed right?
    value_t scores[50];

    int capt = CAN_CAPT(moves);

    for(i=0; moves[capt][i][0] != -1; i++){
        movePiece(board, moves[capt][i]);

        if(!i){
            value = -negaMax(board, transpos, depth-1, -beta, -alpha, best);    
        }else{
            value = -negaMax(board, transpos, depth-1, -alpha - 1, -alpha, best);

            if(alpha < value && value < beta && !out_of_time){
                value = -negaMax(board, transpos, depth-1, -beta, -alpha, best);
                research[depth]++; 
            }
        }


        undoMove(board, moves[capt][i]);
        scores[i] = value;
        
        
        //time out
        if(out_of_time)
            break;

        if(value > score)
            score = value;
        if(score > alpha)
            alpha = score;
        //prune
        if(score >= beta){
            states_pruned++;
            break;
        }
    }

    //reorder root moves
    int max;
    value_t max_val;
    uint8_t tmp_move[4];
    int j, h, tmp;


    for(j=0; j<i; j++){
        max_val = scores[j];
        max = j;
        for(h=j+1; h<i; h++){
            if(scores[h] > max_val){
                max = h;
                max_val = scores[h];
            }
        }

        tmp = scores[max];  //INEF: only second line need DEBUG:
        scores[max] = scores[j];
        scores[j] = tmp;

        memcpy(tmp_move, moves[capt][j], 4);
        memcpy(moves[capt][j], moves[capt][max], 4);
        memcpy(moves[capt][max], tmp_move, 4);
    }

    if(out_of_time){
        last_moves_considered = i;
    }

    return scores[0];
}

//[0][1]: player y position, [2]: player x position
// value_t pos_value[2][9][9] = {
//     {
//      {0, 0, 10, 10, 20, 10, 10, 0, 0},
//      {0, 0, 10, 20, 20, 20, 10, 0, 0},
//      {0, 0, 10, 40, 40, 40, 10, 0, 0},
//      {50, 50, 50, 90, 90, 90, 50, 50, 50},
//      {150, 140, 120, 140, 140, 140, 120, 140, 150},
//      {170, 160, 150, 150, 150, 150, 150, 160, 170},
//      {200, 180, 150, 150, 150, 150, 150, 180, 200},
//      {300, 250, 200, 200, 200, 200, 200, 250, 300},
//      {300, 300, 300, 300, 300, 300, 300, 300, 300},
//     },
//     {
//      {300, 300, 300, 300, 300, 300, 300, 300, 300},
//      {300, 250, 200, 200, 200, 200, 200, 250, 300},
//      {200, 180, 150, 150, 150, 150, 150, 180, 200},
//      {170, 160, 150, 150, 150, 150, 150, 160, 170},
//      {150, 140, 120, 140, 140, 140, 120, 140, 150},
//      {50, 50, 50, 90, 90, 90, 50, 50, 50},
//      {0, 0, 10, 40, 40, 40, 10, 0, 0},
//      {0, 0, 10, 20, 20, 20, 10, 0, 0},
//      {0, 0, 10, 10, 20, 10, 10, 0, 0},
//     }
// };

value_t pos_value[2][9][9] = {
    {
     {0, 0, 0, 10, 20, 10, 0, 0, 0},
     {0, 0, 10, 20, 30, 20, 10, 0, 0},
     {0, 10, 20, 30, 40, 30, 20, 10, 0},
     {50, 50, 50, 90, 90, 90, 50, 50, 50},
     {100, 100, 100, 140, 140, 140, 100, 100, 100},
     {170, 150, 150, 150, 150, 150,  150, 150, 170},
     {220, 170, 150, 150, 150, 150, 150, 170, 220},
     {250, 220, 200, 200, 200, 200, 200, 220, 250},
     {250, 250, 200, 200, 200, 200, 200, 250, 250},
    },
    {
     {250, 250, 200, 200, 200, 200, 200, 250, 250},
     {250, 220, 200, 200, 200, 200, 200, 220, 250},
     {220, 170, 150, 150, 150, 150, 150, 170, 220},
     {170, 150, 150, 150, 150, 150, 150, 150, 170},
     {100, 100, 100, 140, 140, 140, 100, 100, 100},
     {50, 50, 50, 90, 90, 90, 50, 50, 50},
     {0, 10, 20, 30, 40, 30, 20, 10, 0},
     {0, 0, 10, 20, 30, 20, 10, 0, 0},
     {0, 0, 0, 10, 20, 10, 0, 0, 0},
    }
};


value_t evaluate(board_t *board){
    value_t score = ((board->piece_list_size[board->turn % 2] - board->piece_list_size[(board->turn + 1) % 2]) * 2000);

    int player = board->turn % 2;

    for(int i=0; i<board->piece_list_size[1]; i++){
        score += pos_value[player][board->piece_list[player][i][1]][board->piece_list[player][i][0]];
    }

    player = (player + 1) % 2;

    for(int i=0; i<board->piece_list_size[0]; i++){
        score -= pos_value[player][board->piece_list[player][i][1]][board->piece_list[player][i][0]];
    }

    return score;    
}


//Return 1 if current player is winning, 2 for the opponent (0 for none).
//NOTE: Stalemate does not get checked (intentional)
int checkWin(board_t *board){
    
    //REWRITE:
    if(board->turn % 2 + 1 == 1){
        //end of board reached
        for(int i=0; i<9; i++){
            if(PLAYER(i, 8) == 1)
                return 1;
            if(PLAYER(i, 0) == 2)
                return 2;
        }

        //no more pieces
        if(!board->piece_list_size[0])
            return 2;
        if(!board->piece_list_size[1])
            return 1;
    }else{
        for(int i=0; i<9; i++){
            if(PLAYER(i, 8) == 1)
                return 2;
            if(PLAYER(i, 0) == 2)
                return 1;
        }

        if(!board->piece_list_size[0])
            return 1;
        if(!board->piece_list_size[1])
            return 2;
    }

    return 0;
}


void initRandTable(){
    for(int i=0; i<9; i++){
        for(int j=0; j<9; j++){
            hashTable[i][j][0] = (((uint64_t)(rand() % RAND_MAX)) << 32 | rand() % RAND_MAX);
            hashTable[i][j][1] = (((uint64_t)(rand() % RAND_MAX)) << 32 | rand() % RAND_MAX);
        }
    }
}

uint64_t getHash(board_t *board){
    uint64_t hash = 0;
    int i, j;
    
    for(i=0; i<9; i++){
        for(j=0; j<9; j++){
            hash ^= hashTable[i][j][PLAYER(i, j)];
        }
    }

    return hash;
}


//Retruns 1 if key is present in table
int lookupTT(transposition_table_t table[TT_SIZE], uint64_t key){
    uint32_t low_key = KEY(key);
    if(table[low_key].type){
        if(table[low_key].key == KEY_HIGH(key)){
            return table[low_key].depth;
        }
        collision++;
    }

    return -1;
}

//currently using DEEP replacement scheme
void storeTT(transposition_table_t table[TT_SIZE], uint64_t key, value_t value, uint8_t type, uint8_t fromx, uint8_t fromy, uint8_t tox, uint8_t toy, uint8_t depth){  
    //depth here is equal to how much further I will look (have looked)
    uint32_t key_small = KEY(key);


    if(!lookupTT(table, key) || table[key_small].depth < depth){
        table[key_small].value = value;
        table[key_small].type = type;

        table[key_small].moves[0] = fromx;
        table[key_small].moves[1] = fromy;
        table[key_small].moves[2] = tox;
        table[key_small].moves[3] = toy;
        
        table[key_small].depth = depth;

        table[key_small].key = KEY_HIGH(key);
    }
}



void printList(board_t *board){
    move(10, 0);
    for(int i=0; i<board->piece_list_size[0]; i++){
        printw("%d %d, ", board->piece_list[0][i][0], board->piece_list[0][i][1]);
    }
    printw("(%d) ", board->piece_list_size[0]);
    printw("\n");
    for(int i=0; i<board->piece_list_size[1]; i++){
        printw("%d %d, ", board->piece_list[1][i][0], board->piece_list[1][i][1]);
    }
    printw("(%d) ", board->piece_list_size[1]);
    printw("\n");

    refresh();
}


void printMoves(move_t moves){
    move(15, 0);

    int size[2] = {0, 0};

    printw("CAPTURES: \n");
        refresh();
        while(moves[1][size[1]][0] != -1){
            printw("%d %d %d %d\n", moves[1][size[1]][0], moves[1][size[1]][1], moves[1][size[1]][2], moves[1][size[1]][3]);
            refresh();
            size[1]++;
        }
        printw("DONE %d\n", size[1]);
        printw("MOVES: \n");
        refresh();
        while(moves[0][size[0]][0] != -1){
            printw("%d %d %d %d\n", moves[0][size[0]][0], moves[0][size[0]][1], moves[0][size[0]][2], moves[0][size[0]][3]);
            refresh();
            size[0]++;
        }
        printw("DONE %d\n", size[0]);
        refresh();
}