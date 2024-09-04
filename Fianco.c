#include <stdio.h>
#include <stdlib.h>

int negaMax(int board[9][9], int depth, int alfa, int beta);

int main(){
    int board[9][9];
    printf("%d\n", negaMax(board, 1, -1000, 1000));
}

int negaMax(int board[9][9], int depth, int alfa, int beta){
    
}