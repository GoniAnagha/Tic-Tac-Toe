#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

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

int main() {
    int server_fd, player1_socket, player2_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    // Initialize the Tic-Tac-Toe board
    init_board();

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
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

    printf("Waiting for players to connect...\n");

    // Accept Player 1
    if ((player1_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("Player 1 connection failed");
        exit(EXIT_FAILURE);
    }
    printf("Player 1 connected\n");

    // Accept Player 2
    if ((player2_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("Player 2 connection failed");
        exit(EXIT_FAILURE);
    }
    printf("Player 2 connected\n");
    send_empty_board(player1_socket, player2_socket);
    int wins;
    // Game loop
    while (1) {
        int row, col;
        int winner = check_winner();
        if (winner) {
            snprintf(buffer, sizeof(buffer), "Player %d wins!\n", winner);
            wins = winner;
            send(player1_socket, buffer, strlen(buffer), 0);
            send(player2_socket, buffer, strlen(buffer), 0);
            char response1[4]; // Buffer for player 1's response
            char response2[4]; // Buffer for player 2's response
            send(player1_socket, "Do you want to play another game?(yes/no)", strlen("Do you want to play another game?(yes/no)"), 0);
            send(player2_socket, "Do you want to play another game?(yes/no)", strlen("Do you want to play another game?(yes/no)"), 0);
            // Receive responses from both players
            recv(player1_socket, response1, sizeof(response1), 0);
            recv(player2_socket, response2, sizeof(response2), 0);

            // Check responses
            if (strcmp(response1, "yes") == 0 && strcmp(response2, "yes") == 0) {
                // Both players want to play again
                send_empty_board(player1_socket, player2_socket); // Send empty board
                init_board();
                player_turn=1;
            } 
            else if(strcmp(response1, "no") == 0 && strcmp(response2, "no") == 0)
            {
                send(player1_socket, "Both players chose not to play!!", strlen("Both players chose not to play!!"), 0);
                send(player2_socket,"Both players chose not to play!!", strlen("Both players chose not to play!!"), 0 );
                close(player1_socket);
                close(player2_socket);
                close(server_fd);
                return 0;
            }
            else {
                // Inform both players that the game is ending
                send(player1_socket, "One player chose not to play again. Exiting...", strlen("One player chose not to play again. Exiting..."), 0);
                send(player2_socket, "One player chose not to play again. Exiting...", strlen("One player chose not to play again. Exiting..."), 0);
                close(player1_socket);
                close(player2_socket);
                close(server_fd);
                return 0;
            }
        } else if (check_draw()) {
            snprintf(buffer, sizeof(buffer), "It's a draw!\n");
            send(player1_socket, buffer, strlen(buffer), 0);
            send(player2_socket, buffer, strlen(buffer), 0);
            char response1[4]; // Buffer for player 1's response
            char response2[4]; // Buffer for player 2's response
            send(player1_socket, "Do you want to play another game?(yes/no)", strlen("Do you want to play another game?(yes/no)"), 0);
            send(player2_socket, "Do you want to play another game?(yes/no)", strlen("Do you want to play another game?(yes/no)"), 0);
            // Receive responses from both players
            recv(player1_socket, response1, sizeof(response1), 0);
            recv(player2_socket, response2, sizeof(response2), 0);
            //printf("%s %s\n", response1, response2);
            // Check responses
            if (strcmp(response1, "yes") == 0 && strcmp(response2, "yes") == 0) {
                // Both players want to play again
                send_empty_board(player1_socket, player2_socket); // Send empty board
                init_board();
                player_turn=1;
            } 
            else if(strcmp(response1, "no") == 0 && strcmp(response2, "no") == 0)
            {
                send(player1_socket, "Both players chose not to play!!", strlen("Both players chose not to play!!"), 0);
                send(player2_socket,"Both players chose not to play!!", strlen("Both players chose not to play!!"), 0 );
                close(player1_socket);
                close(player2_socket);
                return 0;
            }
            else {
                // Inform both players that the game is ending
                send(player1_socket, "One player chose not to play again. Exiting...", strlen("One player chose not to play again. Exiting..."), 0);
                send(player2_socket, "One player chose not to play again. Exiting...", strlen("One player chose not to play again. Exiting..."), 0);
                close(player1_socket);
                close(player2_socket);
                close(server_fd);
                return 0;
            }
            break;
        }

        // Notify the current player for their move
        if (player_turn == 1) {
            send(player1_socket, "Your turn (Player 1 - X):\n", strlen("Your turn (Player 1 - X):\n"), 0);
            recv(player1_socket, buffer, sizeof(buffer), 0);
            sscanf(buffer, "%d %d", &row, &col);

            // Validate and update the board
            if (row >= 0 && row < 3 && col >= 0 && col < 3 && board[row][col] == ' ') {
                update_board(row, col, 'X');
                player_turn = 2;  // Switch turn
            } else {
                send(player1_socket, "Invalid move. Try again.\n", strlen("Invalid move. Try again.\n"), 0);
            }
        } else {
            send(player2_socket, "Your turn (Player 2 - O):\n", strlen("Your turn (Player 2 - O):\n"), 0);
            recv(player2_socket, buffer, sizeof(buffer), 0);
            sscanf(buffer, "%d %d", &row, &col);

            // Validate and update the board
            if (row >= 0 && row < 3 && col >= 0 && col < 3 && board[row][col] == ' ') {
                update_board(row, col, 'O');
                player_turn = 1;  // Switch turn
            } else {
                send(player2_socket, "Invalid move. Try again.\n", strlen("Invalid move. Try again.\n"), 0);
            }
        }

        // Send the updated board to both players
        send_board_to_players(player1_socket, player2_socket);
    }

    // Close connections
    close(player1_socket);
    close(player2_socket);
    //printf("Player %d wins!\n", wins);
    close(server_fd);

    return 0;
}
