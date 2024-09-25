#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "fianco.h"
#include "server.h"

#define PORT 12345
#define BUFFER_SIZE 1024

int connect_server(){
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    char *player_id = "player2\n";  // Player ID to send to the server

    erase();
    move(0, 0);

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printw("\n Socket creation error \n");
        refresh();
        return -1;
    }

    // Define the server address
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 or IPv6 address to binary form
    if (inet_pton(AF_INET, "192.168.23.162", &serv_addr.sin_addr) <= 0) {  // Replace with the server's IP address
        printw("\nInvalid address/ Address not supported \n");
        refresh();
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printw("\nConnection Failed \n");
        refresh();
        return -1;
    }

    // Send the player ID to the server
    send(sock, player_id, strlen(player_id), 0);
    printw("Player ID sent: %s", player_id);
    refresh();

    return sock;
}

void sendBoard(int sock, board_t *board){
    char buffer[200];
    int count = 1;
    buffer[0] = '[';

    for(int i=0; i<9; i++){
        buffer[count] = '[';
        count++;
        for(int j=0; j<9; j++){
            buffer[count] = PLAYER(j, i) + '0';
            buffer[count + 1] = ',';
            count += 2;
        }
        count--;
        buffer[count] = ']';
        buffer[count+1] = ',';
        count+=2;
    }

    count--;
    buffer[count] = ']';
    count++;
    buffer[count] = '\n';

    send(sock, buffer, sizeof(buffer), 0);
}

int recBoard(int sock, int init_board[9][9]){
    char buffer[BUFFER_SIZE] = {0};


    int valRead = read(sock, buffer, BUFFER_SIZE);
    // erase();
    // mvprintw(0, 0, "%s", buffer);
    // refresh();
    // getch();
    if(valRead){

        char *board_state = buffer + 10;
        int count=1;

        for(int i=0; i<9; i++){
            // buffer[count] = "[";
            count++;
            for(int j=0; j<9; j++){
                init_board[i][j] = board_state[count] - '0';
                count += 2;
            }
            count+=1;
        }

        return 1;
    }

    sleep(1);

    return 0;
}

void setBoard(int init_board[9][9], board_t *board){
    board->piece_list_size[0] = 0;
    board->piece_list_size[1] = 0;

    int player, list_size;

    for(int i=0; i<9; i++){
        for(int j=0; j<9; j++){
                player = init_board[j][i];
                //if piece add to list
                if(player > 0){
                    list_size = board->piece_list_size[player-1];
                    board->piece_list[player-1][list_size][0] = i;
                    board->piece_list[player-1][list_size][1] = j;

                    board->cell[i][j][0] = player; //player
                    board->cell[i][j][1] = list_size;  //which position in list

                    board->piece_list_size[player-1]++;
                    
                }else
                    board->cell[i][j][0] = 0;

        }
    }

    board->piece_list[0][board->piece_list_size[0]][0] = -1;
    board->piece_list[1][board->piece_list_size[1]][0] = -1;

    board->turn = 0;

    //get hash
    board->hash = getHash(board);

    board->depth = 0;
}