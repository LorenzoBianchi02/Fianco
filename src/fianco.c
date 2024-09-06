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
}board_t;


board_t *initializeBoard();
void printBoard(board_t *board);
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

    printBoard(board);

    int fromx, fromy, tox, toy;

    while(true){
        // do{
            getch();
            getmouse(&mevent);
            fromx = mevent.y;
            fromy = mevent.x/2;
            refresh();
        // }while(!board->cell[fromx][fromy]->player);      //FIXME: crashes

        mvchgat(fromx, fromy*2, 2, A_NORMAL, ((fromx+fromy+1)%2)+3, NULL);

        getch();
        getmouse(&mevent);
        tox = mevent.y;
        toy = mevent.x/2;
        refresh();
        
        if(!movePiece(board, fromx, fromy, tox, toy)){
            mvprintw(11, 2, "INVALID MOVE");
        }

        mvchgat(fromx, fromy*2, 2, A_NORMAL, ((fromx+fromy)%2)+1, NULL);
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
        {1, 1, 1, 1, 1, 1, 1, 1, 1},
        {0, 1, 0, 0, 0, 0, 0, 1, 0},
        {0, 0, 1, 0, 0, 0, 1, 0, 0},
        {0, 0, 0, 1, 0, 1, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 2, 0, 2, 0, 0, 0},
        {0, 0, 2, 0, 0, 0, 2, 0, 0},
        {0, 2, 0, 0, 0, 0, 0, 2, 0},
        {2, 2, 2, 2, 2, 2, 2, 2, 2}
    };

    piece_t *last[2];
    last[0] = NULL;
    last[1] = NULL;

    for(int i=0; i<9; i++){
        for(int j=0; j<9; j++){
                piece_t *elem = (piece_t *)malloc(sizeof(piece_t));
                elem->x = i;
                elem->y = j;
                elem->player = init_board[i][j];

                //if piece add to list
                if(init_board[i][j] != 0){
                    elem->prev = last[elem->player];

                    if(last[elem->player-1])
                        last[elem->player-1]->next = elem;
                    else
                        board->head[elem->player-1] = elem;
                    
                    elem->next = NULL;
                    last[elem->player-1] = elem;
                }

                board->cell[i][j] = elem;
        }
    }

    board->tail[1] = last[1];
    board->tail[2] = last[2];

    return board;
}

void printBoard(board_t *board){
    erase();
    int i, j;
    move(1, 2);
    for(i=8; i>=0; i--){
        for(j=0; j<9; j++){
            attron(COLOR_PAIR(((i+j)%2)+1));
            if(board->cell[i][j]->player == 1)
                printw("%s", "\u26C0 ");
            else if(board->cell[i][j]->player == 2)
                printw("%s", "\u26C2 ");
            else
                printw("  ");
            attroff(COLOR_PAIR(((i+j)%2)+1));
        }
        printw("\n  ");
    }

    attroff(COLOR_PAIR(((i+j)%2)+1));

    refresh();
}

//it should be garenteed that a piece is present on fromx/fromy (IT ISN'T)
int movePiece(board_t *board, int fromx, int fromy, int tox, int toy){
    return false;
}

void  printList(piece_t *l){
    piece_t *p = l;
    move(17, 0);

    while(p){
        printw("%d %d %d  \n", p->x, p->y, p->player);
        p=p->next;
    }

    refresh();
}