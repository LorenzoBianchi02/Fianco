#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>

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
    piece_t *prev;    //FIXME: if these pointers remane here, I can't use the board as a hash
    piece_t *next;
};

typedef struct board_t{
    piece_t *cell[9][9];
    piece_t *head[2];
    piece_t *tail[2];

    int turn;
}board_t;


board_t *initializeBoard();
void printBoard(board_t *board);
int boardCoords(int *x, int *y);
int validMove(board_t *board, int fromx, int fromy, int tox, int toy);
int movePiece(board_t *board, int fromx, int fromy, int tox, int toy);

//debug functions
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
    
    //TODO: remove not used pairs
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


    int fromx, fromy, tox, toy;

    while(true){
        printBoard(board);

        printList(board->head[0]);

        do{
            getch();
            getmouse(&mevent);
            fromx = mevent.x/2;
            fromy = mevent.y;
            refresh();
            mvprintw(10, 0, "%d %d", fromx, fromy);
            refresh();
        }while(!boardCoords(&fromx, &fromy) || !board->cell[fromx][fromy]);

        // mvprintw(fromy, fromx*2, "%d %d", fromx, fromy);

        mvchgat(8-fromy, fromx*2, 2, A_NORMAL, ((fromx+fromy+1)%2)+3, NULL);

        getch();
        getmouse(&mevent);
        tox = mevent.x/2;
        toy = mevent.y;
        refresh();
        boardCoords(&tox, &toy);

        
        if(!movePiece(board, fromx, fromy, tox, toy)){
            mvprintw(11, 2, "INVALID MOVE %d %d", tox, toy); //TODO: when clicking outside of screen it shoulnd't say this (handle this in the functions)
        }else
            mvprintw(11, 2, "VALID MOVE  ");

        mvchgat(8 - fromy, fromx*2, 2, A_NORMAL, ((fromx+fromy)%2)+1, NULL);

        board->turn++;

        refresh();
    }

    getch();
    endwin();

    return 0;
}


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

    piece_t *last[2];
    last[0] = NULL;
    last[1] = NULL;

    for(int i=0; i<9; i++){
        for(int j=0; j<9; j++){
                //if piece add to list
                if(init_board[j][i] != 0){
                    piece_t *elem = (piece_t *)malloc(sizeof(piece_t));
                    elem->x = i;
                    elem->y = j;
                    elem->player = init_board[j][i];

                    elem->prev = last[elem->player];

                    if(last[elem->player-1])
                        last[elem->player-1]->next = elem;
                    else
                        board->head[elem->player-1] = elem;
                    
                    elem->next = NULL;
                    last[elem->player-1] = elem;
                    board->cell[j][i] = elem;
                }else
                    board->cell[j][i] = NULL;

        }
    }

    board->tail[1] = last[1];
    board->tail[2] = last[2];

    board->turn = 0;

    return board;
}

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


int boardCoords(int *x, int *y){
    if(*x < 0 || *y < 0 || *x > 8 || *y > 8){
        mvprintw(1, 20, "invalid position");    
        return false;
    }

    *y = 8-*y;
        
    mvprintw(1, 20, "valid position %d %d", *x, *y);

    return true;
}


//returns 0 if invalid, 1 for a normal move, 2 for a capture
int validMove(board_t *board, int fromx, int fromy, int tox, int toy){
    //no piece in starting position or arriving position already occupied
    if(!board->cell[fromx][fromy])
        return FALSE;
    if(board->cell[tox][toy])
        return FALSE;

    int player = board->cell[fromx][fromy]->player;

    if(fromy == toy && abs(fromx - tox) == 1)
        return 1;

    //TODO: can be written better
    if(fromx == tox){
        if(player == 1 && toy - fromy == 1)
            return 1;
        if(player == 2 && toy - fromy == -1)
            return 1;
    }
    //TODO: this aswell, to much duplication
    if(abs(fromx - tox) == 2 && abs(fromy - toy) == 2){
        if(player == 1 && toy - fromy == 2){
            return 2;
        }
        if(player == 2 && toy - fromy == -2)
            return 2;
    }

    return false;
}

//It should be garenteed that a piece is present on fromx/fromy (IT ISN'T) (at the moment it is being checked in validMove). //FIXME:
//It shoud garenteed that fromx/fromy and tox/toy are valid board cooridinates. (IT ISN'T) (it is being checked here) //FIXME:
//Returns whethers the move has succesfully been done.
int movePiece(board_t *board, int fromx, int fromy, int tox, int toy){
    if(fromx < 0 || fromy < 0 || tox < 0 || toy < 0 || fromx > 8 || fromy > 8 || tox > 8 || toy > 8)
        return false;

    mvprintw(14, 0, "move checking: %d %d %d %d", fromx, fromy, tox, toy);
    refresh();
    
    int move = validMove(board, fromx, fromy, tox, toy);
    if(!move)
        return false;


    piece_t *piece = board->cell[fromx][fromy];
    
    if(move == 1){
        piece->x = tox;
        piece->y = toy;
        
        board->cell[fromx][fromy] = NULL; //FIXME: can I put piece here instead of board->ce...?
        board->cell[tox][toy] = piece;
    }else if(move == 2){
        int signx = (tox - fromx)/2, signy = (toy - fromy)/2;
        board->cell[fromx][fromy] = NULL;
        board->cell[fromx+signx][fromy+signy] = NULL;
        board->cell[fromx+signx*2][fromy+signy*2] = piece;
    }

    return true;
}

void  printList(piece_t *l){
    piece_t *piece = l;
    move(17, 0);

    while(piece){
        printw("%d %d %d  \n", piece->x, piece->y, piece->player);
        piece=piece->next;
    }

    refresh();
}