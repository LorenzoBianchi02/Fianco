#include <stdio.h>
#include <stdlib.h>

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


board *initialize_board();
void add_board(board *b, int x, int y, int player);


int main(){
    board *board = initialize_board();
}


board *initialize_board(){
    board *b = (board *)malloc(sizeof(board));

    for(int i=0; i<9; i++){
        for(int j=0; j<9; j++){
            list_elem *elem = malloc(sizeof(list_elem));
            elem->x = i;
            elem->y = j;
            elem->player = 0;
            elem->prev = NULL;
            elem->next = NULL;

            b->cell[i][j] = elem;

        }
    }

    b->black_head = NULL;
    b->black_tail = NULL;
    b->white_head = NULL;
    b->white_tail = NULL;


    //setting up the board



    return b;
}

void set_board(board *b, int x, int y, int player){

}