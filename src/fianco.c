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

typedef struct board{
    list_elem *cell[9][9];
    list_elem *white_head;
    list_elem *white_tail;
    list_elem *black_head;
    list_elem *black_tail;
}board;


board *initializeBoard();
void printBoard(board *b);


int main(){
    //---NCURSES---//
    setlocale(LC_ALL, "");

    initscr();
    start_color();
    use_default_colors();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0);
    noecho();

    printw("test\n");
    refresh();

	mousemask(ALL_MOUSE_EVENTS, NULL);

    // init_color(COLOR_GREEN, 158, 93, 30);
    // init_color(COLOR_WHITE, 205, 170, 125);
    
    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    init_pair(2, COLOR_BLACK, COLOR_BLUE);
    init_pair(3, COLOR_BLACK, COLOR_CYAN);
    init_pair(4, COLOR_BLACK, COLOR_BLACK);
    init_pair(5, COLOR_BLACK, COLOR_MAGENTA);

    init_color(COLOR_BLACK, 0, 0, 0);


    MEVENT mevent;

    //---MAIN---//
    board *b = initializeBoard();

    printBoard(b);

/*
    while(1){
        getch();
        getmouse(&mevent);
        from.x = mevent.y;
        from.y = mevent.x/2;
        refresh();

        attron(COLOR_PAIR(3));
        move(from.x, from.y*2);
        printPiece(&board.piece[from.x][from.y]);
        attroff(COLOR_PAIR(3));

        getch();
        getmouse(&mevent);
        to.x = mevent.y;
        to.y = mevent.x/2;
        refresh();
        
        clear();

        if(move_piece(&board, from, to))
            board.info.turn++;
        else{
            mvprintw(14, 0, "INVALID MOVE");
        }

        printBoard(&board);

        printw("\n");
        attron(COLOR_PAIR(1 + 3 * (board.info.turn%2)));
        printw("  ");
        attroff(COLOR_PAIR(1 + 3 * (board.info.turn%2)));
        printw("turn: ");
    }


    
    */

    getch();
    endwin();

    return 0;
}


board *initializeBoard(){
    board *b = (board *)malloc(sizeof(board));

    b->black_head = NULL;
    b->black_tail = NULL;
    b->white_head = NULL;
    b->white_tail = NULL;


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

            b->cell[i][j] = elem;
        }
    }


    return b;
}

void printBoard(board *b){
    erase();
    int i, j;
    move(1, 2);
    for(i=8; i>=0; i--){
        for(j=0; j<9; j++){
            attron(COLOR_PAIR(((i+j)%2)+1));
            if(b->cell[i][j]->player == 1)
                printw("%s", "\u26C0 ");
            else if(b->cell[i][j]->player == 2)
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