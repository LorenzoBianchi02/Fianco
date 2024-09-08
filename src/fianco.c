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
    piece_t *prev;    //NOTE: if these pointers remain here, I can't use the board as a hash (the pointers aren't always the same, see TODO for a possible solution)
    piece_t *next;
};

typedef struct board_t{
    piece_t *cell[9][9];
    piece_t *head[2];
    piece_t *tail[2];

    int moves[2][75][4]; //moves a player can make: [2] capture/move
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

    while(true){
        printBoard(board);

        int tmp = 0;
        move(13, 0);
        printw("CAPTURES: \n");
        getMoves(board, 1);
        refresh();
        while(board->moves[1][tmp][0] != -1){
            printw("%d %d %d %d\n", board->moves[0][tmp][0], board->moves[0][tmp][1], board->moves[0][tmp][2], board->moves[0][tmp][3]);
            refresh();
            tmp++;
        }
        printw("DONE %d %d\n", tmp, board->moves[1][tmp][0]);
        refresh();
        getch();

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
    player--;

    piece_t *piece = board->head[player];
    int size[2] = {0, 0};
    int fromx, fromy, move;
    int tox, toy;

    while(piece){
        fromx = piece->x;
        fromy = piece->y;

        for(int i=0; i<5; i++){
            tox = fromx + _moves[player][i][0];
            toy = fromy + _moves[player][i][1];
            move = validMove(board, fromx, fromy, tox, toy);
            if(move)
                printw("%d: %d %d %d %d\n", move, fromx, fromy, tox, toy);

            if(move){ //REWRITE: not very elegant
                board->moves[move - 1][size[move - 1]][0] = fromx;
                board->moves[move - 1][size[move - 1]][0] = fromy;
                board->moves[move - 1][size[move - 1]][0] = tox;
                board->moves[move - 1][size[move - 1]][0] = toy;
            }
        }

        piece=piece->next;
    }

    board->moves[0][size[0]][0] = -1;
    board->moves[1][size[0]][0] = -1;
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

    piece_t *piece = board->cell[fromx][fromy];
    
    if(move == 1){
        piece->x = tox;
        piece->y = toy;
        
        board->cell[fromx][fromy] = NULL; //REWRITE: can I put piece here instead of board->ce...?
        board->cell[tox][toy] = piece;
    }else if(move == 2){
        int signx = (tox - fromx)/2, signy = (toy - fromy)/2;

        board->cell[fromx][fromy] = NULL;
        board->cell[fromx+signx][fromy+signy] = NULL;
        board->cell[fromx+signx*2][fromy+signy*2] = piece;
    }

    return TRUE;
}

//prints the coords of a players pieces
void  printList(piece_t *l){
    piece_t *piece = l;
    move(17, 0);

    while(piece){
        printw("%d %d %d  \n", piece->x, piece->y, piece->player);
        piece=piece->next;
    }

    refresh();
}