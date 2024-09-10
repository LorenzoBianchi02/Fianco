#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>

#define EMPTY 0
#define WHITE 1
#define BLACK 2

/*
the board is represented with a linked hashmap:
    each board cell is present in map.
    If the cell has a piece on it, then it will be
    present in the corrisponding list;
*/
typedef struct piece_t piece_t;
struct piece_t{
    int x;
    int y;
    int player;
    piece_t *prev;    //NOTE: if these pointers remain here, I can't use the board as a hash (the pointers aren't always the same, see TODO for a possible solution)
    piece_t *next;
};

typedef struct board_t{
    piece_t *cell[9][9];
    piece_t *piece_list[3][16];

    int moves[2][75][4]; //moves a player can make: [2] ([0]: moves, [1]: captures)
                         //                         [75] buffer
                         //                         [2] x, y    
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
void  printList(piece_t *l);


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

        move(13, 0);
        printList(board->head[1]);
        printList(board->head[2]);

        int tmp[2] = {0, 0};
        getMoves(board, turn%2+1);

        printw("CAPTURES: \n");
        refresh();
        while(board->moves[1][tmp[1]][0] != -1){
            printw("%d %d %d %d\n", board->moves[1][tmp[1]][0], board->moves[1][tmp[1]][1], board->moves[1][tmp[1]][2], board->moves[1][tmp[1]][3]);
            refresh();
            tmp[1]++;
        }
        printw("DONE %d\n", tmp[1]);
        printw("MOVES: \n");
        refresh();
        while(board->moves[0][tmp[0]][0] != -1){
            printw("%d %d %d %d\n", board->moves[0][tmp[0]][0], board->moves[0][tmp[0]][1], board->moves[0][tmp[0]][2], board->moves[0][tmp[0]][3]);
            refresh();
            tmp[0]++;
        }
        printw("DONE %d\n", tmp[0]);
    
        refresh();

        do{
            getch();
            getmouse(&mevent);
            fromx = mevent.x/2;
            fromy = mevent.y;
            refresh();
        }while(!boardCoords(&fromx, &fromy) || !board->cell[fromx][fromy] || board->cell[fromx][fromy]->player != turn % 2 + 1);

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

    piece_t *last[3];
    last[1] = NULL;
    last[2] = NULL;

    for(int i=0; i<9; i++){
        for(int j=0; j<9; j++){
                //if piece add to list
                if(init_board[j][i] != 0){
                    piece_t *elem = (piece_t *)malloc(sizeof(piece_t));
                    elem->x = i;
                    elem->y = j;
                    elem->player = init_board[j][i];

                    elem->prev = last[elem->player];

                    if(last[elem->player])
                        last[elem->player]->next = elem;
                    else
                        board->head[elem->player] = elem;
                    
                    elem->next = NULL;
                    last[elem->player] = elem;

                    board->cell[j][i] = elem;
                }else
                    board->cell[j][i] = NULL;

        }
    }

    board->tail[1] = last[1];
    board->tail[2] = last[2];

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
                if(board->cell[i][j]){
                    if(board->cell[i][j]->player == 1)
                        printw("%s", "\u26C0 ");
                    else if(board->cell[i][j]->player == 2)
                        printw("%s", "\u26C2 ");
                }else
                    printw("  ");
                attroff(COLOR_PAIR(((i+j)%2)+1));
            
        }
        printw("\n");
    }

    attroff(COLOR_PAIR(((i+j)%2)+1));

    refresh();
}



//moves a piece can make
int _moves[2][5][2] = {
    {{-1, 0}, {1, 0}, {0, 1}, {-2, 2}, {2, 2}},
    {{-1, 0}, {1, 0}, {0, -1}, {-2, -2}, {2, -2}}};

//TODO: testing, print the moves
//populates the moves matrix [0] has captures and [1] has moves, makes it easier to check if can (has to) capture
void getMoves(board_t *board, int player){
    piece_t *piece = board->head[player];
    int size[2] = {0, 0};
    int fromx, fromy, move;
    int tox, toy;

    while(piece){
        fromx = piece->x;
        fromy = piece->y;

        for(int i=0; i<5; i++){
            tox = fromx + _moves[player-1][i][0];
            toy = fromy + _moves[player-1][i][1];
            move = validMove(board, fromx, fromy, tox, toy);

            if(move){ //REWRITE: not very elegant
                board->moves[move - 1][size[move - 1]][0] = fromx;
                board->moves[move - 1][size[move - 1]][1] = fromy;
                board->moves[move - 1][size[move - 1]][2] = tox;
                board->moves[move - 1][size[move - 1]][3] = toy;

                size[move - 1]++;
            }
        }

        piece=piece->next;
    }

    board->moves[0][size[0]][0] = -1;
    board->moves[1][size[1]][0] = -1;
}




//tranforms stdscr coords to [9][9] coords
int boardCoords(int *x, int *y){
    if(*x < 0 || *y < 0 || *x > 8 || *y > 8)
        return false;

    *y = 8-*y;
        
    return true;
}


//returns 0 if invalid, 1 for a normal move, 2 for a capture
int validMove(board_t *board, int fromx, int fromy, int tox, int toy){
    //check if move is inbounds //FIXME: move the check of fromx/fromy to main
    if(fromx < 0 || fromy < 0 || tox < 0 || toy < 0 || fromx > 8 || fromy > 8 || tox > 8 || toy > 8)
        return FALSE;

    //no piece in starting position or arriving position already occupied
    if(!board->cell[fromx][fromy])
        return FALSE;
    if(board->cell[tox][toy])
        return FALSE;

    int player = board->cell[fromx][fromy]->player;

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
        if(board->cell[fromx+signx][fromy+signy] == NULL || board->cell[fromx+signx][fromy+signy]->player == board->cell[fromx][fromy]->player)
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
        board->cell[tox][toy] = board->cell[fromx][fromy];  //INEF: to much board->cell[from][fromy] repitition
        board->cell[fromx][fromy] = NULL;

        board->cell[fromx][fromy]->x = tox;
        board->cell[fromx][fromy]->y = toy;
    }else if(move == 2){
        int signx = (tox - fromx)/2, signy = (toy - fromy)/2; //REWRITE:

        board->cell[fromx+signx*2][fromy+signy*2] = board->cell[fromx][fromy];
        board->cell[fromx][fromy] = NULL;
        board->cell[fromx+signx][fromy+signy] = NULL;
    }

    return TRUE;
}

//prints the coords of a players pieces
void printList(piece_t *l){
    piece_t *piece = l;

    while(piece){
        printw("%d %d %d, ", piece->x, piece->y, piece->player);
        piece=piece->next;
    }
    printw("\n");

    refresh();
}