#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <time.h>

#define PLAYER(x, y) _PLAYER(board, x, y)
#define _PLAYER(board, x, y) board->cell[(x)][(y)][0]

#define POSITION(x, y) _POSITION(board, x, y)
#define _POSITION(board, x, y) board->cell[(x)][(y)][1]

#define CAN_CAPT(moves) moves[1][0][0] != -1 ? 1 : 0

typedef int8_t move_t[2][76][4]; //moves a player can make: [2] ([0]: moves, [1]: captures), [75] buffer, [4] fromx, fromy, tox, toy

#define INF 40000 


/*
the board is represented with a linked hashmap:
    each board cell is present in map.
    If the cell has a piece on it, then it will be
    present in the corrisponding "list";
*/

typedef struct board_t{
    int8_t cell[9][9][2]; //player (so wich list aswell) and position in the list //NOTE: maybe char can be used
    
    int8_t piece_list[2][16][2]; //[2]: player, [16][2]: piece position (x, y)

    uint8_t piece_list_size[2];  //amount of pieces in each list

    uint16_t turn;
}board_t;



typedef struct result_t{
    int score;
    int move;
}result_t;



board_t *initializeBoard();

//general functions
void getMoves(board_t *board, int player, move_t moves);
int validMove(board_t *board, int fromx, int fromy, int tox, int toy);
int movePiece(board_t *board, int fromx, int fromy, int tox, int toy);
void undoMove(board_t *board, int fromx, int fromy, int tox, int toy); //it is guaranteed the move is ok

//ai frunction
int negaMarx(board_t *board, int depth, int alpha, int beta, int best[4]);
int evaluate(board_t *board);

//human functions
void printBoard(board_t *board);
int boardCoords(int *x, int *y);
int checkWin(board_t *board);

//debug functions (END: will at some point have to be removed)
void printList(board_t *board);
void printMoves(move_t moves);

//statistics
uint64_t states_visited;
uint64_t states_pruned;

int main(){
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
    
    //END: remove not used pairs
    // NB: be carefull when changing the order
    init_pair(1, COLOR_BLACK, COLOR_YELLOW);    //board color 1
    init_pair(2, COLOR_BLACK, COLOR_GREEN);     //board color 2
    init_pair(4, COLOR_WHITE, COLOR_YELLOW);    //piece selected 1
    init_pair(3, COLOR_WHITE, COLOR_GREEN);     //piece selected 2
    init_pair(6, COLOR_BLACK, COLOR_BLACK);     //turn color 1
    init_pair(5, COLOR_BLACK, COLOR_WHITE);     //turn color 2
    init_pair(7, COLOR_BLACK, COLOR_CYAN);      //last move

    init_color(COLOR_BLACK, 0, 0, 0);

    MEVENT mevent;


    //---------MAIN---------//
    board_t *board = initializeBoard();
    int fromx, fromy, tox, toy;
    move_t moves; //moves a player can make: [2] ([0]: moves, [1]: captures), [75] buffer, [4] fromx, fromy, tox, toy
    uint8_t move_history[1024][4]; //NOTE: if game goes longer it will crash

    //TODO: ask if you want to play as white or black
    int human = 1;
    int flag;
    clock_t start, end;
    int res;

    getMoves(board, 1, moves);


    while(!checkWin(board)){
        start_turn:

        erase(); //END: remove
        printBoard(board);

        if(board->turn && board->turn % 2 + 1 == human){
            mvchgat(8-toy, 2*tox, 2, A_NORMAL, 7, NULL);
        }

        mvprintw(1, 20, "Player's turn:   \n");
        mvchgat(1, 35, 2, A_NORMAL, board->turn%2+5, NULL);

        mvprintw(3, 20, " UNDO \n");
        mvchgat(3, 20, 6, A_NORMAL, 5, NULL);
        refresh();

        move(10, 0);
        printw("LIST WHITE: ");
        printList(board);
        printw("LIST BLACK: ");
        printList(board);
        printw("\n");
        refresh();


        getMoves(board, board->turn%2+1, moves);
        printMoves(moves);
        flag = 1;

        //-----HUMAN-----//
        move(6, 20);
        if(board->turn % 2 + 1 == human){
            printw("states visited %lu, states pruned %lu (in %ld seconds)", states_visited, states_pruned, start);
            if(board->turn)
                printw(", ai res: %d\n", res);
            refresh();

            //first click
            do{
                getch();
                getmouse(&mevent);
                fromx = mevent.x/2;
                fromy = mevent.y;
                refresh();

                //---UNDO---//
                if(board->turn && fromy == 3 && fromx >= 10 && fromx <= 12){
                    undoMove(board, move_history[board->turn-1][0], move_history[board->turn-1][1], move_history[board->turn-1][2], move_history[board->turn-1][3]);
                    board->turn--;
                    if(board->turn){
                        undoMove(board, move_history[board->turn-1][0], move_history[board->turn-1][1], move_history[board->turn-1][2], move_history[board->turn-1][3]);
                        board->turn--;
                    }

                    goto start_turn; //this is my code, and I shall do what I want!!!
                }

            }while(!boardCoords(&fromx, &fromy) || !PLAYER(fromx, fromy) || PLAYER(fromx, fromy) != board->turn % 2 + 1); //NOTE: this may the only place where it is checked that the piece picked needs to be of the current player

            // mvprintw(fromy, fromx*2, "%d %d", fromx, fromy);

            mvchgat(8-fromy, fromx*2, 2, A_NORMAL, ((fromx+fromy+1)%2)+3, NULL);

            //second click
            getch();
            getmouse(&mevent);
            tox = mevent.x/2;
            toy = mevent.y;
            refresh();
            boardCoords(&tox, &toy);

            if(validMove(board, fromx, fromy, tox, toy)){
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

        //------DESTROYER-----//
        else{
            // human = human % 2 + 1;

            states_visited = 0;
            states_pruned = 0;


            int best[4];

            move(17, 0);

            start = clock();
            res = negaMarx(board, 11, -INF, INF, best);
            end = clock();

            start = ((double) (end - start)) / CLOCKS_PER_SEC * 1000;

            fromx = best[0];
            fromy = best[1];
            tox = best[2];
            toy = best[3];

            // printBo ard(board);
            // move(7, 20);
            // printw("val: %d", res);
            // refresh();
            // getch();
        }


        //valid move
        if(flag && movePiece(board, fromx, fromy, tox, toy)){
            move_history[board->turn][0] = fromx;
            move_history[board->turn][1] = fromy;
            move_history[board->turn][2] = tox;
            move_history[board->turn][3] = toy;
            
            board->turn++;
        }

        refresh();
    }

    erase();
    printBoard(board);
    move(11, 0);
    int winner = checkWin(board);
    printw("   PLAYER %d won", winner);
    printw("\n\n   (Press any key to leave)");
    refresh();

    getch();
    endwin();

    return 0;
}

//initializes the board with the correct pieces, and allocates needed memory
board_t *initializeBoard(){
    board_t *board = (board_t *)malloc(sizeof(board_t));

    //setting up the board
    // int init_board[9][9] = 
    // {
    //     {1, 0, 0, 0, 0, 0, 0, 0, 2},
    //     {1, 1, 0, 0, 0, 0, 0, 2, 2},
    //     {1, 0, 1, 0, 0, 0, 2, 0, 2},
    //     {1, 0, 0, 1, 0, 2, 0, 0, 2},
    //     {1, 0, 0, 0, 0, 0, 0, 0, 2},
    //     {1, 0, 0, 1, 0, 2, 0, 0, 2},
    //     {1, 0, 1, 0, 0, 0, 2, 0, 2},
    //     {1, 1, 0, 0, 0, 0, 0, 2, 2},
    //     {1, 0, 0, 0, 0, 0, 0, 0, 2}
    // };

    int init_board[9][9] = 
    {
        {0, 0, 0, 0, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 1, 0, 0, 2, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 1, 0, 0, 2, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 1, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0}
    };

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

    return board;
}

//prints the board on the screen (currently at the positions 0 0 of stdscr)
void printBoard(board_t *board){
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
int boardCoords(int *x, int *y){
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
void getMoves(board_t *board, int player, move_t moves){
    int size[2] = {0, 0}; //captures and moves
    int fromx, fromy, move;
    int tox, toy, i, j;

    player--;

    //first check captures
    for(i=0; i<board->piece_list_size[player]; i++){
        fromx = board->piece_list[player][i][0];
        fromy = board->piece_list[player][i][1];

        for(j=3; j<5; j++){
            tox = fromx + _moves[player][j][0];
            toy = fromy + _moves[player][j][1];

            if(validMove(board, fromx, fromy, tox, toy)){ //REWRITE: not very elegant
                moves[1][size[1]][0] = fromx;
                moves[1][size[1]][1] = fromy;
                moves[1][size[1]][2] = tox;
                moves[1][size[1]][3] = toy;

                size[1]++;
            }
        }
    }

    //check moves if no captures
    if(!size[0]){
        for(i=0; i<board->piece_list_size[player]; i++){
            fromx = board->piece_list[player][i][0];
            fromy = board->piece_list[player][i][1];

            for(j=0; j<3; j++){
                tox = fromx + _moves[player][j][0];
                toy = fromy + _moves[player][j][1];

                if(validMove(board, fromx, fromy, tox, toy)){ //REWRITE: not very elegant
                    moves[0][size[0]][0] = fromx;
                    moves[0][size[0]][1] = fromy;
                    moves[0][size[0]][2] = tox;
                    moves[0][size[0]][3] = toy;

                    size[0]++;
                }
            }
        }
    }

    moves[0][size[0]][0] = -1;
    moves[1][size[1]][0] = -1;
}


//returns 0 if invalid, 1 for a normal move, 2 for a capture
int validMove(board_t *board, int fromx, int fromy, int tox, int toy){
    //check if move is inbounds
    if(fromx < 0 || fromy < 0 || tox < 0 || toy < 0 || fromx > 8 || fromy > 8 || tox > 8 || toy > 8)
        return FALSE;

    //no piece in starting position or arriving position already occupied //INEF:(small) for the ai the first check isn't necesary
    if(!PLAYER(fromx, fromy))
        return FALSE;
    if(PLAYER(tox, toy))
        return FALSE;

    int player = PLAYER(fromx, fromy);

    //left right
    if(fromy == toy && abs(fromx - tox) == 1)
        return 1;

    //REWRITE: can be written better
    //up down
    if(fromx == tox){
        if(player == 1 && toy - fromy == 1)
            return 1;
        if(player == 2 && toy - fromy == -1)
            return 1;
    }

    //REWRITE: this aswell, to much duplication
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
//FIXME:It should be garenteed that a piece is present on fromx/fromy (IT ISN'T) (at the moment it is being checked in validMove).
int movePiece(board_t *board, int fromx, int fromy, int tox, int toy){
    int move = validMove(board, fromx, fromy, tox, toy); //FIXME: remove this from here, for the human it should be in main
    if(!move)
        return FALSE;
    
    if(move == 1){
        //change coords
        board->piece_list[PLAYER(fromx, fromy) - 1][POSITION(fromx, fromy)][0] = tox; //REWRITE: use memcpy
        board->piece_list[PLAYER(fromx, fromy) - 1][POSITION(fromx, fromy)][1] = toy;
        
        //move "pointer"
        board->cell[tox][toy][0] = board->cell[fromx][fromy][0];
        board->cell[tox][toy][1] = board->cell[fromx][fromy][1];

        //remove starting pos
        board->cell[fromx][fromy][0] = 0;

    }else if(move == 2){
        int signx = (tox - fromx)/2, signy = (toy - fromy)/2; //REWRITE:

        //move capturing piece
        board->piece_list[PLAYER(fromx, fromy) - 1][POSITION(fromx, fromy)][0] = tox; //REWRITE: use memcpy
        board->piece_list[PLAYER(fromx, fromy) - 1][POSITION(fromx, fromy)][1] = toy;

        PLAYER(tox, toy) = PLAYER(fromx, fromy);    //REWRITE: finda a way to asign vars
        POSITION(tox, toy) = POSITION(fromx, fromy);
        PLAYER(fromx, fromy) = 0;

        //remove captured piece (also from list)
        int x = fromx + signx, y = fromy+signy;
        int list = PLAYER(x, y) - 1;
        int oldpos = POSITION(fromx+signx, fromy+signy);

        board->piece_list[list][oldpos][0] = board->piece_list[list][board->piece_list_size[list] - 1][0];
        board->piece_list[list][oldpos][1] = board->piece_list[list][board->piece_list_size[list] - 1][1];

        board->cell[board->piece_list[list][oldpos][0]][board->piece_list[list][oldpos][1]][1] = oldpos;
        board->cell[x][y][0] = 0;

        board->piece_list_size[list]--;
    }

    return TRUE;
}


//undos a given move
//guaranteed that this was the last move, and a valid one
void undoMove(board_t *board, int fromx, int fromy, int tox, int toy){
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
    

    if(move == 1){
        //change coords
        board->piece_list[PLAYER(fromx, fromy) - 1][POSITION(fromx, fromy)][0] = tox; //REWRITE: use memcpy
        board->piece_list[PLAYER(fromx, fromy) - 1][POSITION(fromx, fromy)][1] = toy;
        
        //move "pointer"
        board->cell[tox][toy][0] = board->cell[fromx][fromy][0];
        board->cell[tox][toy][1] = board->cell[fromx][fromy][1];

        //remove starting pos
        board->cell[fromx][fromy][0] = 0;

    }else if(move == 2){
        int signx = (tox - fromx)/2, signy = (toy - fromy)/2; //REWRITE:

        //move capturing piece
        board->piece_list[PLAYER(fromx, fromy) - 1][POSITION(fromx, fromy)][0] = tox; //REWRITE: use memcpy
        board->piece_list[PLAYER(fromx, fromy) - 1][POSITION(fromx, fromy)][1] = toy;

        PLAYER(tox, toy) = PLAYER(fromx, fromy);    //REWRITE: finda a way to asign vars
        POSITION(tox, toy) = POSITION(fromx, fromy);
        PLAYER(fromx, fromy) = 0;

        //read captured piece (also from list)
        int x = fromx + signx, y = fromy+signy;
        int list = PLAYER(tox, toy) % 2;
        int oldpos = POSITION(fromx+signx, fromy+signy);

        board->piece_list[list][board->piece_list_size[list]][0] = x;
        board->piece_list[list][board->piece_list_size[list]][1] = y;

        PLAYER(x, y) = list + 1;
        POSITION(x, y) = board->piece_list_size[list];
        
        board->piece_list_size[list]++;
    }
}


//main function for AI
int negaMarx(board_t *board, int depth, int alpha, int beta, int best[4]){
    // printBoard(board);
    // move(5, 20);
    // printw("states visited %lu, states pruned %lu, %d", states_visited, states_pruned, board->turn%2+1);
    // refresh();
    // getch();

    int eval = evaluate(board);
    erase();
    states_visited++;


    //REWRITE:
    int terminal = checkWin(board);

    if(terminal){
        if(terminal == 1)
            eval = 30000;
        else if(terminal == 2)
            eval = -30000;
        else
            eval = 0;
    }

    if(terminal || !depth)
        return eval;

    
    move_t moves;
    getMoves(board, board->turn%2+1, moves);

    //TODO: stalemate should be checked here

    //TODO: check how many bits are needed
    int score = -INF, move;
    int value;

    int capt = CAN_CAPT(moves);

    for(int i=0; moves[capt][i][0] != -1; i++){
        movePiece(board, moves[capt][i][0], moves[capt][i][1], moves[capt][i][2], moves[capt][i][3]);
        board->turn++;

        value = -1 * negaMarx(board, depth-1, -beta, -alpha, best);

        undoMove(board, moves[capt][i][0], moves[capt][i][1], moves[capt][i][2], moves[capt][i][3]);
        board->turn--;


        if(value > score){
            score = value;
            move = i;
        }
        if(score > alpha)
            alpha = score;
        //prune
        if(score >= beta){
            states_pruned++;
            break;
        }
    }

    best[0] = moves[capt][move][0];
    best[1] = moves[capt][move][1];
    best[2] = moves[capt][move][2];
    best[3] = moves[capt][move][3];

    // printw("best %d: %d %d %d %d\n", depth, best[0], best[1], best[2], best[3]);
    // refresh();

    return score;
}


//NOTE: check if I need to negate
//TODO: how many bits are needed
int evaluate(board_t *board){
    return (board->piece_list_size[(board->turn + 1) % 2] - board->piece_list_size[board->turn % 2]) * -1;
}


//return 1 if current player is winning, 2 for the opponent (0 for none)
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

    // if(moves[0][0][0] == -1 && moves[1][0][0] == -1)
    //     return (board->turn+1)%2+1;

    return 0;
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