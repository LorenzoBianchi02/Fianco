#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 12345
#define BUFFER_SIZE 1024

int server() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    char *player_id = "player2\n";  // Player ID to send to the server
    char *move = "[[1,1,1,1,1,1,1,1,1],[0,1,0,0,0,0,0,1,0],[0,0,1,0,0,0,1,0,0],[0,0,0,1,0,0,0,0,0],[0,0,0,0,0,1,0,0,0],[0,0,0,2,0,2,0,0,0],[0,0,2,0,0,0,2,0,0],[0,2,0,0,0,0,0,2,2],[2,2,2,2,2,2,2,2,0]]\n";     // Example move to send to the server

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    // Define the server address
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 or IPv6 address to binary form
    if (inet_pton(AF_INET, "192.168.23.162", &serv_addr.sin_addr) <= 0) {  // Replace with the server's IP address
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    // Send the player ID to the server
    send(sock, player_id, strlen(player_id), 0);
    printf("Player ID sent: %s", player_id);

    // Receive and respond to server messages in a loop
    while (1) {
        // Clear the buffer
        memset(buffer, 0, BUFFER_SIZE);

        // Read data from the server
        int valread = read(sock, buffer, BUFFER_SIZE);
        if (valread > 0) {
            printf("Server: %s\n", buffer);

            // Check if the message is asking for a move
            if (strncmp(buffer, "Your move:", 10) == 0) {
                // Extract the board state (you can process it if needed)
                char *board_state = buffer + 10;
                printf("Received board state: %s\n", board_state);
                

                // Send move to the server
                send(sock, move, strlen(move), 0);
                printf("Move sent: %s\n", move);
            }
        }

        sleep(2);
    }

    // Close the socket
    close(sock);
    return 0;
}