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
typedef struct list_elem list_elem;
struct list_elem{
    int x;
    int y;
    int player;
    list_elem *prev;    //FIXME: if these pointers remane here, I can't use the board as a hash
    list_elem *next;
};

typedef struct board_t{
    list_elem *cell[9][9];
    list_elem *white_head;
    list_elem *white_tail;
    list_elem *black_head;
    list_elem *black_tail;
}board_t;


board_t *initializeBoard();
void printBoard(board_t *board);


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

    // init_color(COLOR_GREEN, 158, 93, 30);
    // init_color(COLOR_WHITE, 205, 170, 125);
    
    init_pair(1, COLOR_BLACK, COLOR_WHITE); //board color 1
    init_pair(2, COLOR_BLACK, COLOR_BLUE);  //board color 2
    init_pair(3, COLOR_BLACK, COLOR_CYAN);
    init_pair(4, COLOR_BLACK, COLOR_BLACK);
    init_pair(5, COLOR_BLACK, COLOR_MAGENTA);   //TODO: remove not used pairs
    init_pair(6, COLOR_BLACK, COLOR_RED);

    init_color(COLOR_BLACK, 0, 0, 0);


    MEVENT mevent;

    //---------MAIN---------//
    board_t *board = initializeBoard();

    printBoard(board);

    int fromx, fromy, tox, toy;


    while(true){
        getch();
        getmouse(&mevent);
        fromx = mevent.y;
        fromy = mevent.x/2;
        refresh();

        mvchgat(fromx, fromy*2, 2, A_NORMAL, 3, NULL);

        getch();
        getmouse(&mevent);
        tox = mevent.y;
        toy = mevent.x/2;
        refresh();
        
        clear();

        // if(move_piece(&board, from, to))
        //     board.info.turn++;
        // else{
        //     mvprintw(14, 0, "INVALID MOVE");
        // }

        printBoard(board);        
    }




    getch();
    endwin();

    return 0;
}


board_t *initializeBoard(){
    board_t *board = (board_t *)malloc(sizeof(board_t));

    board->black_head = NULL;
    board->black_tail = NULL;
    board->white_head = NULL;
    board->white_tail = NULL;


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

    for(int i=0; i<9; i++){
        for(int j=0; j<9; j++){
            list_elem *elem = (list_elem *)malloc(sizeof(list_elem));
            elem->x = i;
            elem->y = j;
            elem->player = init_board[i][j];
            elem->prev = NULL; //TODO:
            elem->next = NULL;

            board->cell[i][j] = elem;
        }
    }


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
        }
        attroff(COLOR_PAIR(((i+j)%2)+1));
        printw("\n  ");
    }

    attroff(COLOR_PAIR(((i+j)%2)+1));

    refresh();
}