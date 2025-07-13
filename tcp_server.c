#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>

#define PORT 8080

char board[3][3];
int player_turn = 1;  // 1 for player 1, 2 for player 2

// Initialize the board to be empty
void init_board() {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            board[i][j] = ' ';
        }
    }
}

// Display the current state of the Tic-Tac-Toe board
void print_board() {
    printf("\n");
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            printf(" %c ", board[i][j]);
            if (j < 2) printf("|");
        }
        if (i < 2) printf("\n---|---|---\n");
    }
    printf("\n");
}

void send_empty_board(int client1, int client2) {
    char message[1024];
    memset(message, 0, sizeof(message));
    
    // Create an empty 3x3 Tic-Tac-Toe board
    snprintf(message, sizeof(message), 
        "   |   |   \n"
        "---|---|---\n"
        "   |   |   \n"
        "---|---|---\n"
        "   |   |   \n");
    
    // Send the empty board to both clients
    send(client1, message, strlen(message), 0);
    send(client2, message, strlen(message), 0);
}

// Check for a winner
int check_winner() {
    // Check rows and columns
    for (int i = 0; i < 3; i++) {
        if (board[i][0] == board[i][1] && board[i][1] == board[i][2] && board[i][0] != ' ')
            return (board[i][0] == 'X') ? 1 : 2;
        if (board[0][i] == board[1][i] && board[1][i] == board[2][i] && board[0][i] != ' ')
            return (board[0][i] == 'X') ? 1 : 2;
    }
    // Check diagonals
    if (board[0][0] == board[1][1] && board[1][1] == board[2][2] && board[0][0] != ' ')
        return (board[0][0] == 'X') ? 1 : 2;
    if (board[0][2] == board[1][1] && board[1][1] == board[2][0] && board[0][2] != ' ')
        return (board[0][2] == 'X') ? 1 : 2;

    return 0;  // No winner
}

// Check for a draw (if no empty cells remain)
int check_draw() {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (board[i][j] == ' ') return 0;
        }
    }
    return 1;  // Draw
}

// Update the board based on player's move
void update_board(int row, int col, char mark) {
    board[row][col] = mark;
}

// Send the current board state to both clients
void send_board_to_players(int player1_socket, int player2_socket) {
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    // Format the board as a string
    snprintf(buffer, sizeof(buffer),
             "\n %c | %c | %c\n---|---|---\n %c | %c | %c\n---|---|---\n %c | %c | %c\n",
             board[0][0], board[0][1], board[0][2],
             board[1][0], board[1][1], board[1][2],
             board[2][0], board[2][1], board[2][2]);

    // Send the board to both players
    send(player1_socket, buffer, strlen(buffer), 0);
    send(player2_socket, buffer, strlen(buffer), 0);
}

// Improved function to clear any pending input from a socket
void clear_socket_buffer(int socket_fd) {
    int original_flags = fcntl(socket_fd, F_GETFL, 0);
    fcntl(socket_fd, F_SETFL, original_flags | O_NONBLOCK);
    
    char temp_buffer[1024];
    int bytes_read;
    
    // Keep reading until no more data is available
    do {
        bytes_read = recv(socket_fd, temp_buffer, sizeof(temp_buffer), 0);
    } while (bytes_read > 0);
    
    // Restore original socket flags
    fcntl(socket_fd, F_SETFL, original_flags);
}

// Function to handle player responses for "play again" question
int handle_play_again_responses(int player1_socket, int player2_socket) {
    char response1[4] = {0};
    char response2[4] = {0};
    
    send(player1_socket, "Do you want to play another game?(yes/no): ", 
         strlen("Do you want to play another game?(yes/no): "), 0);
    send(player2_socket, "Do you want to play another game?(yes/no): ", 
         strlen("Do you want to play another game?(yes/no): "), 0);
    
    // Clear any pending input first
    clear_socket_buffer(player1_socket);
    clear_socket_buffer(player2_socket);
    
    // Receive responses from both players
    int r1 = recv(player1_socket, response1, sizeof(response1) - 1, 0);
    int r2 = recv(player2_socket, response2, sizeof(response2) - 1, 0);
    
    if (r1 <= 0 || r2 <= 0) {
        send(player1_socket, "A player disconnected. Session ended.\n", 
             strlen("A player disconnected. Session ended.\n"), 0);
        send(player2_socket, "A player disconnected. Session ended.\n", 
             strlen("A player disconnected. Session ended.\n"), 0);
        return -1; // Error
    }
    
    // Null-terminate the responses
    response1[r1] = '\0';
    response2[r2] = '\0';
    
    // Remove any trailing newlines
    char *newline = strchr(response1, '\n');
    if (newline) *newline = '\0';
    newline = strchr(response2, '\n');
    if (newline) *newline = '\0';
    
    // Check responses
    if (strcmp(response1, "yes") == 0 && strcmp(response2, "yes") == 0) {
        return 1; // Both want to play again
    } else if (strcmp(response1, "no") == 0 && strcmp(response2, "no") == 0) {
        send(player1_socket, "Both players chose not to play!! Session ended.\n", 
             strlen("Both players chose not to play!! Session ended.\n"), 0);
        send(player2_socket, "Both players chose not to play!! Session ended.\n", 
             strlen("Both players chose not to play!! Session ended.\n"), 0);
        return 0; // Both don't want to play
    } else {
        send(player1_socket, "One player chose not to play again. Session ended.\n", 
             strlen("One player chose not to play again. Session ended.\n"), 0);
        send(player2_socket, "One player chose not to play again. Session ended.\n", 
             strlen("One player chose not to play again. Session ended.\n"), 0);
        return 0; // One doesn't want to play
    }
}

int main() {
    int server_fd, player1_socket, player2_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the specified port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 2) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server started. Waiting for players to connect...\n");

    // Main server loop - keeps running and accepts new players
    while (1) {
        printf("Waiting for players to connect...\n");

        // Accept Player 1
        if ((player1_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Player 1 connection failed");
            continue; // Try again instead of exiting
        }
        printf("Player 1 connected\n");

        // Accept Player 2
        if ((player2_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Player 2 connection failed");
            close(player1_socket);
            continue; // Try again instead of exiting
        }
        printf("Player 2 connected\n");
        
        // Initialize game for new players
        init_board();
        player_turn = 1;
        send_empty_board(player1_socket, player2_socket);

        // Game session loop for current pair of players
        while (1) {
            int row, col;
            int winner = check_winner();
            
            if (winner) {
                snprintf(buffer, sizeof(buffer), "Player %d wins!\n", winner);
                send(player1_socket, buffer, strlen(buffer), 0);
                send(player2_socket, buffer, strlen(buffer), 0);
                
                int play_again = handle_play_again_responses(player1_socket, player2_socket);
                if (play_again == 1) {
                    send_empty_board(player1_socket, player2_socket);
                    init_board();
                    player_turn = 1;
                    continue;
                } else {
                    break; // End this game session
                }
            } else if (check_draw()) {
                snprintf(buffer, sizeof(buffer), "It's a draw!\n");
                send(player1_socket, buffer, strlen(buffer), 0);
                send(player2_socket, buffer, strlen(buffer), 0);
                
                int play_again = handle_play_again_responses(player1_socket, player2_socket);
                if (play_again == 1) {
                    send_empty_board(player1_socket, player2_socket);
                    init_board();
                    player_turn = 1;
                    continue;
                } else {
                    break; // End this game session
                }
            }

            // Clear any pending input from both sockets
            clear_socket_buffer(player1_socket);
            clear_socket_buffer(player2_socket);

            // Notify the current player for their move
            if (player_turn == 1) {
                send(player1_socket, "Your turn (Player 1 - X):\n", 
                     strlen("Your turn (Player 1 - X):\n"), 0);
                send(player2_socket, "Waiting for Player 1's move...\n", 
                     strlen("Waiting for Player 1's move...\n"), 0);
            } else {
                send(player2_socket, "Your turn (Player 2 - O):\n", 
                     strlen("Your turn (Player 2 - O):\n"), 0);
                send(player1_socket, "Waiting for Player 2's move...\n", 
                     strlen("Waiting for Player 2's move...\n"), 0);
            }

            // Use select to monitor both sockets and only accept input from current player
            fd_set readfds;
            int max_fd = (player1_socket > player2_socket) ? player1_socket : player2_socket;
            int valid_move = 0;
            
            while (!valid_move) {
                FD_ZERO(&readfds);
                FD_SET(player1_socket, &readfds);
                FD_SET(player2_socket, &readfds);
                
                int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
                
                if (activity < 0) {
                    printf("Select error\n");
                    break;
                }
                
                // Check if player 1 sent data
                if (FD_ISSET(player1_socket, &readfds)) {
                    memset(buffer, 0, sizeof(buffer));
                    int bytes_received = recv(player1_socket, buffer, sizeof(buffer) - 1, 0);
                    
                    if (bytes_received <= 0) {
                        printf("Player 1 disconnected.\n");
                        send(player2_socket, "Player 1 disconnected. Session ended.\n", 
                             strlen("Player 1 disconnected. Session ended.\n"), 0);
                        goto end_session;
                    }
                    
                    buffer[bytes_received] = '\0';
                    
                    if (player_turn == 1) {
                        // It's player 1's turn, process the move
                        if (sscanf(buffer, "%d %d", &row, &col) != 2 || row < 0 || row >= 3 || 
                            col < 0 || col >= 3 || board[row][col] != ' ') {
                            send(player1_socket, "Invalid input or move. Try again.\n", 34, 0);
                            continue;
                        }
                        
                        update_board(row, col, 'X');
                        player_turn = 2;
                        valid_move = 1;
                    } else {
                        // It's not player 1's turn, ignore the input
                        send(player1_socket, "It's not your turn. Please wait.\n", 
                             strlen("It's not your turn. Please wait.\n"), 0);
                    }
                }
                
                // Check if player 2 sent data
                if (FD_ISSET(player2_socket, &readfds)) {
                    memset(buffer, 0, sizeof(buffer));
                    int bytes_received = recv(player2_socket, buffer, sizeof(buffer) - 1, 0);
                    
                    if (bytes_received <= 0) {
                        printf("Player 2 disconnected.\n");
                        send(player1_socket, "Player 2 disconnected. Session ended.\n", 
                             strlen("Player 2 disconnected. Session ended.\n"), 0);
                        goto end_session;
                    }
                    
                    buffer[bytes_received] = '\0';
                    
                    if (player_turn == 2) {
                        // It's player 2's turn, process the move
                        if (sscanf(buffer, "%d %d", &row, &col) != 2 || row < 0 || row >= 3 || 
                            col < 0 || col >= 3 || board[row][col] != ' ') {
                            send(player2_socket, "Invalid input or move. Try again.\n", 34, 0);
                            continue;
                        }
                        
                        update_board(row, col, 'O');
                        player_turn = 1;
                        valid_move = 1;
                    } else {
                        // It's not player 2's turn, ignore the input
                        send(player2_socket, "It's not your turn. Please wait.\n", 
                             strlen("It's not your turn. Please wait.\n"), 0);
                    }
                }
            }

            // Send the updated board to both players
            send_board_to_players(player1_socket, player2_socket);
        }

        end_session:
        // Close connections for this pair of players
        printf("Game session ended. Closing connections for current players.\n");
        close(player1_socket);
        close(player2_socket);
        
        // Server continues running and waits for new players
        printf("Server ready for new players.\n");
    }

    // This line should never be reached, but just in case
    close(server_fd);
    return 0;
}
