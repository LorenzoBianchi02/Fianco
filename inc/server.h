#ifndef SERVER_H
#define SERVER_H

//returns sock (int, -1 ERROR)
int connect_server();

void sendBoard(int sock, board_t *board);
int recBoard(int sock, int init_board[9][9]);

void setBoard(int init_board[9][9], board_t *board);

#endif