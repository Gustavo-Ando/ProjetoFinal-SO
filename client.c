#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include <curses.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include <unistd.h>

#include "client.h"
#include "client_process_message.h"
#include "message.h"
#include "map.h"
#include "utility.h"
#include "render.h"

#define PORT 8080

/*  
    Thread to start curses
    Responsible for starting curses, setting colors, rendering map and players and getting input
    Params:
        - void *arg (THREAD_ARG_STRUCT): struct containing info about players and buffer to send
    Return:
        -
*/
static void *curses(void *arg){
    THREAD_ARG_STRUCT *thread_arg = (THREAD_ARG_STRUCT *)arg;
    
    initscr(); // Start curses
    curs_set(0); // Disable cursor
    noecho(); // Disable echo (don't write input to screen)
    nodelay(stdscr, 1); // Make getch non-blocking, to render even while no input is received

    // Get window size
    int width, height;
    getmaxyx(stdscr, height, width);
    // Get starting position from which to render
    int start_x = (width - MAP_WIDTH)/2;
    int start_y = (height - MAP_HEIGHT)/2;

    // Configure terminal colors if the terminal has them
    color_config();
    
    // Get input until enter is pressed
    int k = '\0';
    while(k != '\n') {
        // Render map, players, itens, and debug info and then refresh screen
        erase();
        render_map(thread_arg, start_x, start_y);
        render_players(thread_arg, start_x, start_y);
        render_debug(thread_arg);
        refresh();

        // Get input (non-blocking due to nodelay()
        k = getch();
        if(k != ERR){ // If valid
            // Access buffer CR and add content to the buffer of inputs
            pthread_mutex_lock(&thread_arg->buffer_send_mutex);
            if(thread_arg->buffer_send_size >= 64 - 1) fail("Input buffer full");
            thread_arg->buffer_send[thread_arg->buffer_send_size++] = k;
            pthread_mutex_unlock(&thread_arg->buffer_send_mutex);
        }
    }
    return NULL;
}

/*  
    Thread to read the socket
    Responsible for reading the buffer with messages from the server
    Params:
        - void *arg (THREAD_ARG_STRUCT): struct containing info about players and buffer to send
    Return:
        -
*/
static void *socket_read_thread(void *arg){
    THREAD_ARG_STRUCT *thread_arg = (THREAD_ARG_STRUCT *)arg; // Cast
    char buffer[MESSAGE_SIZE] = { 0 };
    // Receive a message from the server
    while(1){
        // Read the buffer
        int read_length = read(thread_arg->client_fd, buffer, MESSAGE_SIZE);
        buffer[read_length] = '\0'; // Terminate string
        int current_index = 0; // Current index to read from buffer
        // While the current index is valid and the corresponding char is not null
        while(current_index < MESSAGE_SIZE && current_index < read_length && buffer[current_index] != '\0'){ 
            // Process message
            switch(msg_get_type(buffer + current_index)){
                case MOVEMENT: process_message_movement(buffer + current_index, thread_arg); break;
                case PLAYERS: process_message_players(buffer + current_index, thread_arg); break;
                case ITEM: process_message_item(buffer + current_index, thread_arg); break;
                case SYSTEM: process_message_system(buffer + current_index, thread_arg); break;
                // If there are no more messages, end the loop
                default: buffer[current_index] = '\0'; break;
            }
            // Update the current index given the message type (different sizes), to read concatenated messages
            current_index += msg_get_size(buffer + current_index);
        }
    }
    return NULL;
}

/*  
    Thread to write a message
    Responsible for writing and sending a message to the server
    Params:
        - void *arg (THREAD_ARG_STRUCT): struct containing info about players and buffer to send
    Return:
        -
*/
static void *socket_write_thread(void *arg){
    THREAD_ARG_STRUCT *thread_arg = (THREAD_ARG_STRUCT *)arg; // Cast
    char message[MESSAGE_SIZE] = { 0 };

    // Repeat
    while(1) {
        int to_send = 0;
        // Access buffer CR
        pthread_mutex_lock(&thread_arg->buffer_send_mutex);
        // If there is key on buffer, remove and add to message
        if(thread_arg->buffer_send_size >= 1) {
            msgC_input(message, thread_arg->buffer_send[0]);
            // Shift the buffer to get next key
            for(int i = 0; i < thread_arg->buffer_send_size - 1; i++){
                thread_arg->buffer_send[i] = thread_arg->buffer_send[i + 1];
            }
            // Update buffer size and signal there's something to send
            thread_arg->buffer_send_size--;
            to_send = 1;
        }
        pthread_mutex_unlock(&thread_arg->buffer_send_mutex);

        // If there is a msg to send, send after leaving CR
        if(to_send) send(thread_arg->client_fd, message, strlen(message), 0);

    }
    return NULL;
}

int main(int argc, char **argv){
    // Creates the client socket
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(client_fd < 0) fail("Error creating socket\n");
    
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

    // Initialize thread arguments
    THREAD_ARG_STRUCT *thread_arg = malloc(sizeof(THREAD_ARG_STRUCT));
    thread_arg->buffer_send_size = 0;
    thread_arg->num_players = 0;
    thread_arg->current_debug_line = 0;
    thread_arg->client_fd = client_fd;
    
    // Initialize all players
    for(int i = 0; i < MAX_PLAYERS; i++){
        thread_arg->players[i].is_active = 0;
        thread_arg->players[i].is_me = 0;
        thread_arg->players[i].x = -1;
        thread_arg->players[i].y = -1;
        thread_arg->players[i].last_x = -1;
        thread_arg->players[i].last_y = -1;
        thread_arg->players[i].item = NONE;
    }

    // Initialize debug
    for(int i = 0; i < 10; i++){
        thread_arg->debug[i][0] = '\0';
    }
    
    // Create threads to send and receive data
    pthread_t read_thr, write_thr, curses_thr;
    pthread_create(&read_thr, NULL, socket_read_thread, thread_arg);
    pthread_create(&write_thr, NULL, socket_write_thread, thread_arg);
    pthread_create(&curses_thr, NULL, curses, thread_arg);

    // Wait for them to finish
    pthread_join(read_thr, NULL);
    pthread_join(write_thr, NULL);
    pthread_join(curses_thr, NULL);

    // Free space
    free(thread_arg);

    // End the connection and close the socket
    close(client_fd);
    return 0;
}