#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <time.h>
#include <string.h>


//--BOARD--//
#define PLAYER(x, y) _PLAYER(board, x, y)
#define _PLAYER(board, x, y) board->cell[(x)][(y)][0]

#define POSITION(x, y) _POSITION(board, x, y)
#define _POSITION(board, x, y) board->cell[(x)][(y)][1]

#define CAN_CAPT(moves) moves[1][0][0] != -1 ? 1 : 0

/*
the board is represented with a linked hashmap:
    each board cell is present in map.
    If the cell has a piece on it, then it will be
    present in the corrisponding "list";
*/
typedef struct board_t{
    int8_t cell[9][9][2]; //player (so wich list aswell) and position in the list
    
    uint8_t piece_list[2][16][2]; //[2]: player, [16][2]: piece position (x, y)

    uint8_t piece_list_size[2];  //amount of pieces in each list

    uint16_t turn;

    uint64_t hash;
}board_t;

board_t *initializeBoard();



//--GENERAL--//
typedef int16_t value_t;

typedef int8_t move_t[2][50][4]; //moves a player can make: [2] ([0]: moves, [1]: captures), [75] buffer, [4] fromx, fromy, tox, toy
void getMoves(board_t *board, int player, move_t moves);
int validMove(board_t *board, uint8_t fromx, uint8_t fromy, uint8_t tox, uint8_t toy);
int movePiece(board_t *board, uint8_t fromx, uint8_t fromy, uint8_t tox, uint8_t toy);
void undoMove(board_t *board, uint8_t fromx, uint8_t fromy, uint8_t tox, uint8_t toy);


//--TRANSPOSITION TABLE--//
//using 64 bits for hash, 24 (atm) for primary key
#define KEY(val) (uint32_t)((val) & 0xFFFFFF)
#define TT_SIZE 16777216

#define NOT_PRESENT 0
#define EXACT 1
#define LOWER_BOUND 2
#define UPPER_BOUND 3

typedef struct transposition_table_t{
    value_t value;
    uint8_t type;
    uint8_t moves[4];
    uint8_t depth;

    uint64_t key;

} transposition_table_t;

uint64_t hashTable[9][9][2]; //global rand values

void initRandTable();
uint64_t getHash(board_t *board_t);
int lookupTT(transposition_table_t *transpos, uint64_t key);
void storeTT(transposition_table_t *transpos, uint64_t key, value_t value, uint8_t type, uint8_t fromx, uint8_t fromy, uint8_t tox, uint8_t toy, uint8_t depth);


//--AI--//
#define INF 32500 //fits into an int16_t

value_t negaMarx(board_t *board, transposition_table_t *transpos, int depth, int alpha, int beta, uint8_t best[4]);
value_t evaluate(board_t *board);



//--GUI--//
void printBoard(board_t *board);
int boardCoords(uint8_t *x, uint8_t *y);
int checkWin(board_t *board);

//--DEBUG--// (END: will at some point have to be removed)
void printList(board_t *board);
void printMoves(move_t moves);
int debug;
#define DEBUG(string) if(debug){printw(string);refresh();getch();}

//--STATS--//
uint64_t states_visited;
uint64_t states_pruned;
uint64_t TT_found; 

int main(){
    mvprintw(0, 0, "test\n");
    refresh();
    getch();
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
    //NB: be carefull when changing the order
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
    srand(time(0));
    initRandTable();

    board_t *board = initializeBoard();
    uint8_t fromx, fromy, tox, toy;
    move_t moves;
    uint8_t move_history[1024][4]; //NOTE: if game goes longer it will crash

    transposition_table_t *transpos_table = (transposition_table_t *)malloc(TT_SIZE * sizeof(transposition_table_t)); //NOTE: this has to be equal to 2^primary key bits

    int human = 2;
    int flag;
    clock_t start, end;
    int res;


    int tmp = lookupTT(transpos_table, board->hash);

    //TODO: ask if you want to play as white or black

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
            printw("states visited %lu, states pruned %lu (%lu in TT) (in %ld seconds), hash: %ld", states_visited, states_pruned, TT_found, start, board->hash);
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

            }while(!boardCoords(&fromx, &fromy) || !PLAYER(fromx, fromy) || PLAYER(fromx, fromy) != board->turn % 2 + 1); //NOTE: this may be the only place where it is checked that the piece picked needs to be of the current player

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
            // human = human % 2 + 1; //uncomment to only play against yourself

            states_visited = 0;
            states_pruned = 0;
            TT_found = 0;

            //clear TT
            memset(transpos_table, 0, sizeof(transpos_table) * TT_SIZE);

            uint8_t best[4];

            move(17, 0);

            start = clock();
            res = negaMarx(board, transpos_table, 11, -INF, INF, best);
            end = clock();

            start = ((double) (end - start)) / CLOCKS_PER_SEC;

            fromx = best[0];
            fromy = best[1];
            tox = best[2];
            toy = best[3];

            // human = human % 2 + 1;
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
    //     {0, 0, 0, 0, 0, 0, 0, 0, 0},
    //     {0, 0, 1, 0, 0, 0, 2, 0, 0},
    //     {0, 0, 0, 0, 0, 0, 0, 0, 0},
    //     {0, 0, 0, 1, 0, 0, 2, 0, 0},
    //     {0, 0, 0, 0, 0, 0, 0, 0, 0},
    //     {0, 0, 0, 0, 0, 0, 0, 0, 0},
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
void getMoves(board_t *board, int player, move_t moves){
    int size[2] = {0, 0}; //captures and moves
    uint8_t fromx, fromy, tox, toy;
    int i, j, move;

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
int validMove(board_t *board, uint8_t fromx, uint8_t fromy, uint8_t tox, uint8_t toy){
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
//The hash also gets automaticly updated.
//FIXME:It should be garenteed that a piece is present on fromx/fromy (IT ISN'T) (at the moment it is being checked in validMove).
int movePiece(board_t *board, uint8_t fromx, uint8_t fromy, uint8_t tox, uint8_t toy){
    int move = validMove(board, fromx, fromy, tox, toy); //FIXME: remove this from here, for the human it should be in main
    if(!move)
        return FALSE;

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

    return move;
}


//Undos a given move.
//The hash also gets automaticly updated.
//It is guaranteed that this was the last move, and a valid one.
void undoMove(board_t *board, uint8_t fromx, uint8_t fromy, uint8_t tox, uint8_t toy){
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
        board->piece_list[list_from][POSITION(fromx, fromy)][0] = tox; //REWRITE: use memcpy
        board->piece_list[list_from][POSITION(fromx, fromy)][1] = toy;
        
        //move "pointer"
        board->cell[tox][toy][0] = board->cell[fromx][fromy][0];
        board->cell[tox][toy][1] = board->cell[fromx][fromy][1];

        //remove starting pos
        board->cell[fromx][fromy][0] = 0;

    }else if(move == 2){
        int signx = (tox - fromx)/2, signy = (toy - fromy)/2; //REWRITE:

        //move capturing piece
        board->piece_list[list_from][POSITION(fromx, fromy)][0] = tox; //REWRITE: use memcpy
        board->piece_list[list_from][POSITION(fromx, fromy)][1] = toy;

        PLAYER(tox, toy) = PLAYER(fromx, fromy);    //REWRITE: finda a way to asign vars
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
}


//main function for AI
value_t negaMarx(board_t *board, transposition_table_t *transpos, int depth, int alpha, int beta, uint8_t best[4]){
    int old_alpha = alpha;

    //REWRITE: can I do transposition_table_t tmp = transpos[KEY(board->hash)]???
    if(lookupTT(transpos, board->hash) && transpos[KEY(board->hash)].depth >= depth){
        uint32_t key = KEY(board->hash);
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

        if(alpha >= beta){
            states_pruned++; //NOTE: is this a pruning??
            TT_found++;
            memcpy(best, transpos[key].moves, 4);
            return transpos[key].value;
        }
    }


    value_t eval;
    states_visited++;


    //REWRITE:
    int terminal = checkWin(board);

    if(terminal){
        if(terminal == 1)
            eval = 30000 - board->turn*10;      //REWRITE: should depend on board->turn, only on how far it has searched so far
        else if(terminal == 2)
            eval = -30000 + board->turn*10;
        else
            eval = 0;
    }else
        eval = evaluate(board);

    if(terminal || !depth)
        return eval;

    
    move_t moves;
    getMoves(board, board->turn%2+1, moves);

    //check stalemate
    if(moves[0][0][0] == -1 && moves[1][0][0] == -1)
        return -30000 + board->turn*10;



    int move = 0;
    value_t score = -INF, value;

    int capt = CAN_CAPT(moves);

    for(int i=0; moves[capt][i][0] != -1; i++){
        movePiece(board, moves[capt][i][0], moves[capt][i][1], moves[capt][i][2], moves[capt][i][3]);
        board->turn++;

        value = -1 * negaMarx(board, transpos, depth-1, -beta, -alpha, best);

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


    int bound = EXACT;
    if(score <= old_alpha) bound = UPPER_BOUND;
    else if(score >= beta) bound = LOWER_BOUND;

    best[0] = moves[capt][move][0];
    best[1] = moves[capt][move][1];
    best[2] = moves[capt][move][2];
    best[3] = moves[capt][move][3];

    storeTT(transpos, board->hash, score, bound, best[0], best[1], best[2], best[3], depth);

    // printw("best %d: %d %d %d %d\n", depth, best[0], best[1], best[2], best[3]);
    // refresh();

    return score;
}


value_t evaluate(board_t *board){
    int count = 0;
    
    return ((board->piece_list_size[(board->turn + 1) % 2] - board->piece_list_size[board->turn % 2])*1000 - board->turn * 10) * -1;
}


//Return 1 if current player is winning, 2 for the opponent (0 for none).
//Stalemate does not get checked here for efficiency reasons
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
    if(table[low_key].type && table[low_key].key == key)
        return TRUE;

    return FALSE;
}

//NOTE: currently using DEEP replacement scheme
void storeTT(transposition_table_t table[TT_SIZE], uint64_t key, value_t value, uint8_t type, uint8_t fromx, uint8_t fromy, uint8_t tox, uint8_t toy, uint8_t depth){ //TODO: add args    
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

        table[key_small].key = key;
        
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