#ifndef FIANCO_H
#define FIANCO_H

//FIXME: not all headers should be included here
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <time.h>
#include <string.h>

#define MAX_MOVES 1024
#define DRAW 0
#define WIN 30000
#define LOSS -30000

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

    uint8_t depth;  //do not be fooled (again), this is how deep you are in the tree

    uint8_t move_history[1024][4]; //NOTE: if game goes longer it will crash

    uint8_t killer_move[100][2][4]; //2 killer moves

    uint8_t end_game; //if the game is in a end game position
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
//using 64 bits for hash, 28 (atm) for primary key //TODO: use 30 bits with smaller struct
#define KEY(val) (uint32_t)((val) & 0x1FFFFFFF)
#define KEY_HIGH(val) (uint32_t)((val) >> 32)
#define TT_SIZE 536870912

#define NOT_PRESENT 0
#define EXACT 1
#define LOWER_BOUND 2
#define UPPER_BOUND 3

//12 bytes
typedef struct transposition_table_t{
    value_t value;
    uint8_t type;
    uint8_t moves[4];   //FIXME: this fits in 2 bytes instead of 4
    uint8_t depth;

    uint32_t key;

} transposition_table_t;


void initRandTable();
uint64_t getHash(board_t *board_t);
int lookupTT(transposition_table_t *transpos, uint64_t key);
void storeTT(transposition_table_t *transpos, uint64_t key, value_t value, uint8_t type, uint8_t fromx, uint8_t fromy, uint8_t tox, uint8_t toy, uint8_t depth);


//--AI--//
#define INF 32000 //fits into an int16_t

value_t negaMarx(board_t *board, transposition_table_t *transpos, int depth, int alpha, int beta, uint8_t best[4]);
value_t negaMarxRoot(board_t *board, transposition_table_t *transpos, int depth, int alpha, int beta, move_t moves);
value_t evaluate(board_t *board);



//--GUI--//
void printBoard(board_t *board);
int boardCoords(uint8_t *x, uint8_t *y);
int checkWin(board_t *board);

//--DEBUG--// (END: will at some point have to be removed)
void printList(board_t *board);
void printMoves(move_t moves);

#define DEBUG(string) if(debug){printw(string);refresh();getch();}



#endif