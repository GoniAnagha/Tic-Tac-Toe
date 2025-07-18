#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

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
            printf("Enter yes or no: ");
            fflush(stdout);
            scanf("%3s", next_game);
            
            // Clear any remaining input in stdin
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            
            if (strcmp(next_game, "yes") != 0 && strcmp(next_game, "no") != 0) {
                printf("Invalid input. Defaulting to 'no'.\n");
                strcpy(next_game, "no");
            }
            send(client_socket, next_game, strlen(next_game), 0);
        }
        
        // Check if server is ending the game session
        if (strstr(buffer, "Exiting...") || strstr(buffer, "chose not to play")) {
            printf("Game session ended. Disconnecting...\n");
            break;
        }

        // Get the player's move and send to the server
        if (strstr(buffer, "Your turn")) {
            int row, col;
            printf("Enter row and column (1, 2, or 3): ");
            fflush(stdout);
            
            char move_input[100];
            if (fgets(move_input, sizeof(move_input), stdin) == NULL) {
                printf("Error reading input\n");
                continue;
            }
            
            if (sscanf(move_input, "%d %d", &row, &col) != 2) {
                printf("Invalid input. Please enter two numbers like: 1 2\n");
                continue;
            }

            snprintf(buffer, sizeof(buffer), "%d %d", row-1, col-1);
            send(client_socket, buffer, strlen(buffer), 0);
        }
    }

    // Close the socket
    close(client_socket);
    return 0;
}
