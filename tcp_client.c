#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

int main() {
    int client_socket;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    // Create socket
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("Invalid address/ Address not supported\n");
        return -1;
    }

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection Failed\n");
        return -1;
    }

    // Game loop: receive and send messages
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = read(client_socket, buffer, sizeof(buffer));
        if (bytes_received <= 0) {
            printf("\nServer closed the connection.\n");
            break;
        }
        
        printf("%s", buffer);

        if (strstr(buffer, "Do you want to play another game?")) {
            char next_game[4];
            scanf("%s", next_game);
            send(client_socket, next_game, strlen(next_game), 0);
        }

        // Get the player's move and send to the server
        if (strstr(buffer, "Your turn")) {
            int row, col;
            printf("Enter row and column (1, 2, or 3): ");
            scanf("%d %d", &row, &col);
            snprintf(buffer, sizeof(buffer), "%d %d", row-1, col-1);
            send(client_socket, buffer, strlen(buffer), 0);
        }
    }

    // Close the socket
    close(client_socket);
    return 0;
}
