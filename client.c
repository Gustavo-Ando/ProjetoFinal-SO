#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include <curses.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include <unistd.h>

#include "message.h"
#include "map.h"

#define PORT 8080

char buffer_send[64];
int buffer_send_size = 0;
pthread_mutex_t buffer_send_mutex;

// Struct with players' position and status
typedef struct _player {
    int x, y;
    int last_x, last_y;
    enum Item_type item;
    int is_active;
} PLAYER;

PLAYER players[4];
int num_players = 0;
pthread_mutex_t players_mutex;

char debug[10][20];
int current_debug_line = 0;
pthread_mutex_t debug_mutex;

// Function to treat errors
void fail(char *str){
    perror(str);
    exit(EXIT_FAILURE);
}

static void *curses(void *arg){
    initscr(); // Start curses
    curs_set(0); // Disable cursor
    noecho(); // Disable echo (don't write input to screen)
    nodelay(stdscr, 1);

    // get window size
    int width, height;
    getmaxyx(stdscr, height, width);
    int start_x = (width - MAP_WIDTH)/2;
    int start_y = (height - MAP_HEIGHT)/2;

    // If the terminal has colors, set them
    if(has_colors()){
        start_color();
        use_default_colors();

        // Buildings
        init_pair(0, COLOR_WHITE, -1); // Default
        init_pair(1, COLOR_WHITE, COLOR_WHITE); // Walls
        init_pair(2, COLOR_RED , -1); // Trash
        init_pair(3, COLOR_WHITE, COLOR_BLUE); // Plates 1
        init_pair(4, COLOR_WHITE, COLOR_CYAN); // Plates 2
        init_pair(5, COLOR_RED, -1); // Oven

        // Itens
        init_pair(10, COLOR_YELLOW, -1); // Bread
        init_pair(11, COLOR_RED, -1); // Hamburger
        init_pair(12, COLOR_BLACK, -1); // Hamburger Burned
        init_pair(13, COLOR_YELLOW, -1); // Hamburger Ready
        init_pair(14, COLOR_GREEN, -1); // Salad
        init_pair(15, COLOR_CYAN, -1); // Juice
        init_pair(16, COLOR_YELLOW, -1); // French Fries
        init_pair(17, COLOR_BLACK, -1); // French Fries Burned

        // Numbers
        init_pair(20, COLOR_BLACK, COLOR_WHITE); // Default
        init_pair(21, COLOR_YELLOW, COLOR_WHITE); // Warning
        init_pair(22, COLOR_RED, COLOR_WHITE); // Emergency

        // Players/Customers
        init_pair(30, COLOR_WHITE, -1); // Customers
        init_pair(31, COLOR_RED, -1); // P1
        init_pair(32, COLOR_BLUE, -1); // P2
        init_pair(33, COLOR_GREEN, -1); // P3
        init_pair(34, COLOR_YELLOW, -1); // P4
    }

    char player_char[4] = {'a', 'b', 'c', 'd'};

    // Gets input
    int k = '\0';
    while(k != '\n') {
        erase();
        // Render map
        for(int line = 0; line < MAP_HEIGHT; line++){
            for(int col = 0; col < MAP_WIDTH; col++){
                attron(COLOR_PAIR(color_map[line][col]) | attr_map[line][col]);
                mvaddch(start_y+line, start_x+col, map[line][col]);
                attroff(COLOR_PAIR(color_map[line][col]) | attr_map[line][col]);
            }
        }
        // Update player's position
        pthread_mutex_lock(&players_mutex);
        for(int i = 0; i < 4; i++){
            if(players[i].is_active){
                attron(COLOR_PAIR(30 + 1 + i) | A_BOLD);
                mvaddch(start_y+players[i].y, start_x+players[i].x, player_char[i]);
                attroff(COLOR_PAIR(30 + 1 + i) | A_BOLD);
                if(players[i].item != NONE) {
                    int color_index;
                    switch(players[i].item){
                        case PAO: color_index = 10; break;
                        case HAMBURGUER: color_index = 11; break;
                        case HAMBURGUER_QUEIMADO: color_index = 12; break;
                        case HAMBURGUER_PRONTO: color_index = 13; break;
                        case SALADA: color_index = 14; break;
                        case SUCO: color_index = 15; break;
                        case BATATA: color_index = 16; break;
                        case BATATA_QUEIMADA: color_index = 17; break;
                    }
                    attron(COLOR_PAIR(color_index));
                    mvaddch(start_y + players[i].last_y, start_x + players[i].last_x, players[i].item);
                    attroff(COLOR_PAIR(color_index));
                }
            }
        }
        pthread_mutex_unlock(&players_mutex);
        // Debug
        pthread_mutex_lock(&debug_mutex);
        for(int i = 0; i < 10; i++){
            if((current_debug_line + 10 - 1)%10== i) attron(COLOR_PAIR(5));
            mvprintw(i, 0, "%.20s", debug[i]);
            if((current_debug_line + 10 - 1)%10== i) attroff(COLOR_PAIR(5));
        }
        pthread_mutex_unlock(&debug_mutex);
        refresh();

        k = getch();
        if(k != ERR){
            // Access CR and add content to the buffer
            pthread_mutex_lock(&buffer_send_mutex);
            if(buffer_send_size >= 64 - 1) fail("Input buffer full");
            buffer_send[buffer_send_size++] = k;
            pthread_mutex_unlock(&buffer_send_mutex);
        }
    }
    return NULL;
}

static void process_message_item(char *message){
    pthread_mutex_lock(&players_mutex);
    int index = msgS_item_get_player_index(message);
    enum Item_type item = msgS_item_get_item_type(message);
    players[index].item = item;
    pthread_mutex_unlock(&players_mutex);
    
    pthread_mutex_lock(&debug_mutex);
    sprintf(debug[current_debug_line], "ITEM %d:%c", index, item);
    current_debug_line = (current_debug_line + 1) % 10;
    pthread_mutex_unlock(&debug_mutex);
}

static void process_message_movement(char *message){
    // Access CR and update the given player's item
    pthread_mutex_lock(&players_mutex);
    int index = msgS_movement_get_player_index(message), x = msgS_movement_get_x(message), y = msgS_movement_get_y(message);
    if(x != players[index].x || y != players[index].y){
        players[index].last_x = players[index].x;
        players[index].last_y = players[index].y;
        players[index].x = x;
        players[index].y = y;
        if(players[index].last_x == -1 || players[index].last_y == -1){
            players[index].last_x = x;
            players[index].last_y = y;
        }
    }
    pthread_mutex_unlock(&players_mutex);
}

static void process_message_players(char *message){
    pthread_mutex_lock(&debug_mutex);
    sprintf(debug[current_debug_line], "CON %d:%d.", msgS_players_get_player_index(message), msgS_players_get_status(message));
    current_debug_line = (current_debug_line + 1) % 10;
    pthread_mutex_unlock(&debug_mutex);

    // Access CR and update the given player's status, updating number of players in the game
    pthread_mutex_lock(&players_mutex);
    players[msgS_players_get_player_index(message)].is_active = msgS_players_get_status(message);
    if(msgS_players_get_status(message) == 1) num_players++;
    else num_players--;
    pthread_mutex_unlock(&players_mutex);
}

static void *socket_read_thread(void *arg){
    int client_fd = *(int*)arg;
    char buffer[MESSAGE_SIZE + 1] = {0};
    // Receive a message from the server
    while(1){
        // Read the buffer
        int read_length = read(client_fd, buffer, MESSAGE_SIZE);
        buffer[read_length] = '\0';
        int current_index = 0;
        while(current_index < MESSAGE_SIZE && current_index < read_length && buffer[current_index] != '\0'){
            // Update the current index given the message type (different sizes)
            switch(msg_get_type(buffer + current_index)){
                case MOVEMENT:  process_message_movement(buffer + current_index); break;
                case PLAYERS:  process_message_players(buffer + current_index); break;
                case ITEM: process_message_item(buffer + current_index); break;
                // If there are no more messages, end the loop
                default: buffer[current_index] = '\0';  break;
            }
            current_index += msg_get_size(buffer + current_index);
        }
    }
    return NULL;
}

static void *socket_write_thread(void *arg){
    int client_fd = *(int *)arg;
    char message[MESSAGE_SIZE] = { 0 };

    // Repeat
    while(1) {
        int to_send = 0;
        // Access CR
        pthread_mutex_lock(&buffer_send_mutex);
        // If there is key on buffer, remove and add to message
        if(buffer_send_size >= 1) {
            msgC_input(message, buffer_send[0]);
            // Shift the buffer to get next key
            for(int i = 0; i < buffer_send_size - 1; i++){
                buffer_send[i] = buffer_send[i + 1];
            }
            buffer_send_size--;
            to_send = 1;
        }
        pthread_mutex_unlock(&buffer_send_mutex);

        // If there is a msg to send, send after leaving CR
        if(to_send) send(client_fd, message, strlen(message), 0);

    }
    return NULL;
}

int main(int argc, char **argv){
    // Creates the client socket
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(client_fd < 0) fail("Error creating socket\n");

    // Initialize all players
    for(int i = 0; i < 4; i++){
        players[i].is_active = 0;
        players[i].x = -1;
        players[i].y = -1;
        players[i].last_x = -1;
        players[i].last_y = -1;
        players[i].item = NONE;
    }

    for(int i = 0; i < 10; i++){
        debug[i][0] = '\0';
    }
    
    // Create the address struct and fill it
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Convert the addr to binary
    if(inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) fail("IP address error\n");

    // Connect client and server
    int status = connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(status < 0) fail("Connection error\n");

    // Send a message confirming the connection
    char message[MESSAGE_SIZE];
    msgC_connection(message, 1);
    send(client_fd, message, strlen(message), 0);

    // Create threads to send and receive data
    pthread_t read_thr, write_thr, curses_thr;
    pthread_create(&read_thr, NULL, socket_read_thread, &client_fd);
    pthread_create(&write_thr, NULL, socket_write_thread, &client_fd);
    pthread_create(&curses_thr, NULL, curses, &client_fd);

    // Wait for them to finish
    pthread_join(read_thr, NULL);
    pthread_join(write_thr, NULL);
    pthread_join(curses_thr, NULL);

    // End the connection and close the socket
    close(client_fd);
    return 0;
}

