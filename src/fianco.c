#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>

#define PLAYER(x, y) _PLAYER(board, x, y)
#define _PLAYER(board, x, y) board->cell[(x)][(y)][0]

#define POSITION(x, y) _POSITION(board, x, y)
#define _POSITION(board, x, y) board->cell[(x)][(y)][1]

/*
the board is represented with a linked hashmap:
    each board cell is present in map.
    If the cell has a piece on it, then it will be
    present in the corrisponding "list";
*/

typedef struct board_t{
    int8_t cell[9][9][2]; //player (so wich list aswell) and position in the list //NOTE: maybe char can be used
    
    int8_t piece_list[2][16][2]; //[2]: player, [16][2]: piece position (x, y)

    int8_t piece_list_size[2];   //amount of pieces in each list

    int8_t moves[2][75][4]; //moves a player can make: [2] ([0]: moves, [1]: captures), [75] buffer, [4] fromx, fromy, tox, toy
}board_t;



board_t *initializeBoard();

//general functions
void printBoard(board_t *board);
void getMoves(board_t *board, int player);
int validMove(board_t *board, int fromx, int fromy, int tox, int toy);
int movePiece(board_t *board, int fromx, int fromy, int tox, int toy);


//human functions
int boardCoords(int *x, int *y);

//debug functions (END: will at some point have to be removed)
void printList(board_t *board, int list);


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
    init_pair(5, COLOR_BLACK, COLOR_MAGENTA);
    init_pair(6, COLOR_BLACK, COLOR_RED);

    init_color(COLOR_BLACK, 0, 0, 0);

    MEVENT mevent;


    //---------MAIN---------//
    board_t *board = initializeBoard();
    int turn = 0;

    int fromx, fromy, tox, toy;

    while(TRUE){
        erase(); //FIXME: remove
        printBoard(board);
        move(12, 0);
        printw("LIST 0: ");
        printList(board, 0);
        printw("LIST 1: ");
        printList(board, 1);
        printw("\n");
        refresh();

        int size[2] = {0, 0};
        getMoves(board, turn%2+1);

        printw("CAPTURES: \n");
        refresh();
        while(board->moves[1][size[1]][0] != -1){
            printw("%d %d %d %d\n", board->moves[1][size[1]][0], board->moves[1][size[1]][1], board->moves[1][size[1]][2], board->moves[1][size[1]][3]);
            refresh();
            size[1]++;
        }
        printw("DONE %d\n", size[1]);
        printw("MOVES: \n");
        refresh();
        while(board->moves[0][size[0]][0] != -1){
            printw("%d %d %d %d\n", board->moves[0][size[0]][0], board->moves[0][size[0]][1], board->moves[0][size[0]][2], board->moves[0][size[0]][3]);
            refresh();
            size[0]++;
        }
        printw("DONE %d\n", size[0]);
    
        refresh();

        do{
            getch();
            getmouse(&mevent);
            fromx = mevent.x/2;
            fromy = mevent.y;
            refresh();
        }while(!boardCoords(&fromx, &fromy) || !board->cell[fromx][fromy] || PLAYER(fromx, fromy) != turn % 2 + 1);

        // mvprintw(fromy, fromx*2, "%d %d", fromx, fromy);

        mvchgat(8-fromy, fromx*2, 2, A_NORMAL, ((fromx+fromy+1)%2)+3, NULL);

        getch();
        getmouse(&mevent);
        tox = mevent.x/2;
        toy = mevent.y;
        refresh();
        boardCoords(&tox, &toy);

        
        if(movePiece(board, fromx, fromy, tox, toy))
            turn++;

        mvchgat(8 - fromy, fromx*2, 2, A_NORMAL, ((fromx+fromy)%2)+1, NULL);

        refresh();
    }

    getch();
    endwin();

    return 0;
}

//initializes the board with the correct pieces, and allocates needed memory
board_t *initializeBoard(){
    board_t *board = (board_t *)malloc(sizeof(board_t));

    //setting up the board
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
    {{-1, 0}, {1, 0}, {0, 1}, {-2, 2}, {2, 2}},
    {{-1, 0}, {1, 0}, {0, -1}, {-2, -2}, {2, -2}}};

//TODO: testing, print the moves
//populates the moves matrix [0] has captures and [1] has moves, makes it easier to check if can (has to) capture
void getMoves(board_t *board, int player){
    int size[2] = {0, 0};
    int fromx, fromy, move;
    int tox, toy, i, j;

    player--;

    for(i=0; i<board->piece_list_size[player]; i++){
        fromx = board->piece_list[player][i][0];
        fromy = board->piece_list[player][i][1];

        for(j=0; j<5; j++){
            tox = fromx + _moves[player][j][0];
            toy = fromy + _moves[player][j][1];
            move = validMove(board, fromx, fromy, tox, toy);

            if(move){ //REWRITE: not very elegant
                board->moves[move - 1][size[move - 1]][0] = fromx;
                board->moves[move - 1][size[move - 1]][1] = fromy;
                board->moves[move - 1][size[move - 1]][2] = tox;
                board->moves[move - 1][size[move - 1]][3] = toy;

                size[move - 1]++;
            }
        }
    }

    board->moves[0][size[0]][0] = -1;
    board->moves[1][size[1]][0] = -1;
}


//returns 0 if invalid, 1 for a normal move, 2 for a capture
int validMove(board_t *board, int fromx, int fromy, int tox, int toy){
    //check if move is inbounds //FIXME: move the check of fromx/fromy to main
    if(fromx < 0 || fromy < 0 || tox < 0 || toy < 0 || fromx > 8 || fromy > 8 || tox > 8 || toy > 8)
        return FALSE;


    //no piece in starting position or arriving position already occupied
    if(!PLAYER(fromx, fromy))
        return FALSE;
    if(PLAYER(tox, toy))
        return FALSE;

    int player = PLAYER(fromx, fromy);

    if(fromy == toy && abs(fromx - tox) == 1)
        return 1;

    //REWRITE: can be written better
    if(fromx == tox){
        if(player == 1 && toy - fromy == 1)
            return 1;
        if(player == 2 && toy - fromy == -1)
            return 1;
    }
    //REWRITE: this aswell, to much duplication
    if(abs(fromx - tox) == 2 && abs(fromy - toy) == 2){
        int signx = (tox - fromx)/2, signy = (toy - fromy)/2;
        if(PLAYER(fromx+signx, fromy+signy) == 0 || PLAYER(fromx+signx, fromy+signy) == PLAYER(fromx, fromy))
            return FALSE;

        if(player == 1 && toy - fromy == 2){
            return 2;
        }
        if(player == 2 && toy - fromy == -2)
            return 2;
    }

    return FALSE;
}

//Returns whethers the move has succesfully been done.
//FIXME:It should be garenteed that a piece is present on fromx/fromy (IT ISN'T) (at the moment it is being checked in validMove).
//TODO: this function is too ineficient to use for the model
int movePiece(board_t *board, int fromx, int fromy, int tox, int toy){
    int move = validMove(board, fromx, fromy, tox, toy); //FIXME: remove this from here, for the human it should be in main
    if(!move)
        return FALSE;
    
    if(move == 1){
        //change coords
        board->piece_list[PLAYER(tox, toy) - 1][POSITION(fromx, fromy)][0] = tox; //FIXME: use memcpy or pointers
        board->piece_list[PLAYER(tox, toy) - 1][POSITION(fromx, fromy)][1] = toy;
        
        //move "pointer"
        board->cell[tox][toy][0] = board->cell[fromx][fromy][0];
        board->cell[tox][toy][1] = board->cell[fromx][fromy][1];

        //remove starting pos
        board->cell[fromx][fromy][0] = 0;

    }else if(move == 2){
        int signx = (tox - fromx)/2, signy = (toy - fromy)/2; //REWRITE:

        //change coords
        board->piece_list[PLAYER(tox, toy) - 1][POSITION(fromx, fromy)][0] = tox; //FIXME: use memcpy or pointers
        board->piece_list[PLAYER(tox, toy) - 1][POSITION(fromx, fromy)][1] = toy;
        PLAYER(tox, toy) = PLAYER(fromx, fromy);

        //remove starting pos
        PLAYER(fromx, fromy) = 0;

        //remove captured piece (also from list)
        int x = fromx + signx, y = fromy+signy;
        int list = PLAYER(x, y) - 1;
        int oldpos = POSITION(fromx+signx, fromy+signy);



        //swap current with the -1, then the -1 with the second to last one
        board->piece_list[list][oldpos][0] = board->piece_list[list][board->piece_list_size[list] - 1][0];
        board->piece_list[list][oldpos][1] = board->piece_list[list][board->piece_list_size[list] - 1][1];
        board->piece_list[list][board->piece_list_size[list] - 1][0] = -1;        

        board->cell[board->piece_list[list][oldpos][0]][board->piece_list[list][oldpos][1]][1] = oldpos;
        board->piece_list_size[list]--;
    }

    return TRUE;
}

void printList(board_t *board, int list){
    int i = 0;
    for(i=0; i<board->piece_list_size[list]; i++){
        printw("%d %d, ", board->piece_list[list][i][0], board->piece_list[list][i][1]);
    }
    printw("\n");

    refresh();
}