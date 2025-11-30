#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include <curses.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include <unistd.h>
#include <poll.h>

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
static void *curses(void *arg) {
    THREAD_ARG_STRUCT *thread_arg = (THREAD_ARG_STRUCT *)arg;

    initscr();          // Start curses
    curs_set(0);        // Disable cursor
    noecho();           // Disable echo (don't write input to screen)
    nodelay(stdscr, 1); // Make getch non-blocking, to render even while no input is received

    // Get window size
    int width, height;
    getmaxyx(stdscr, height, width);
    // Get starting position from which to render
    int start_x = (width - MAP_WIDTH) / 2;
    int start_y = (height - MAP_HEIGHT) / 2;

    // Configure terminal colors if the terminal has them
    color_config();

    // Get input until enter is pressed
    int k = '\0';
    while (k != '\n') {
        // Check if should terminate thread
        pthread_mutex_lock(&thread_arg->leave_mutex);
        int leave = thread_arg->should_leave;
        pthread_mutex_unlock(&thread_arg->leave_mutex);
        if(leave) break;
        
        // Render map, players, itens, and debug info and then refresh screen
        erase();
        render_map(thread_arg, start_x, start_y);
        render_counters(thread_arg, start_x, start_y);
        render_appliances(thread_arg, start_x, start_y);
        render_players(thread_arg, start_x, start_y);
        render_customers(thread_arg, start_x, start_y, width);
        render_score(thread_arg, start_x, start_y + MAP_HEIGHT + 1);
        render_debug(thread_arg);
        refresh();

        // Get input (non-blocking due to nodelay())
        k = getch();
        if (k != ERR) { // If valid
            // Send message containing key to server
            char message[MESSAGE_SIZE] = {0};
            msgC_input(message, k);
            send_message(thread_arg->client_fd, message);
        }
    }

    // Clear screen
    erase();
    refresh();
    endwin();
    
    // Pressed enter, game should terminate
    pthread_mutex_lock(&thread_arg->leave_mutex);
    thread_arg->should_leave = 1;
    pthread_mutex_unlock(&thread_arg->leave_mutex);
    
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
static void *socket_read_thread(void *arg) {
    THREAD_ARG_STRUCT *thread_arg = (THREAD_ARG_STRUCT *)arg; 
    
    // Enlarged buffer to handle fragmentation and multiple messages
    char buffer[MESSAGE_SIZE * 32]; 
    int stored_bytes = 0; // Bytes currently kept in buffer

    // Create pollfd to poll if socket can be read
    struct pollfd pollfds;
    pollfds.fd = thread_arg->client_fd;
    pollfds.events = POLLIN;
    while (1) {
        // Check if should terminate thread
        pthread_mutex_lock(&thread_arg->leave_mutex);
        int leave = thread_arg->should_leave;
        pthread_mutex_unlock(&thread_arg->leave_mutex);
        if(leave) break;
        
        // Poll socket with 1 second of timeout, restarting loop if timeout or error to check if should terminate
        int poll_timeout = 0.25 * 1000; // In msec
        if (poll(&pollfds, 1, poll_timeout) == 0) continue; // If timeout 
        else if(pollfds.revents != POLLIN) continue; // Error
        
        // Calculate available space in buffer
        int bytes_to_read = (sizeof(buffer) - 1) - stored_bytes;
        
        // Safety reset if buffer is full
        if (bytes_to_read <= 0) {
            stored_bytes = 0;
            bytes_to_read = sizeof(buffer) - 1;
        }

        // Append new data after the stored bytes
        int read_length = read(thread_arg->client_fd, buffer + stored_bytes, bytes_to_read);
        if (read_length <= 0) break; // Disconnected or error

        stored_bytes += read_length;
        buffer[stored_bytes] = '\0'; // Safety null terminator

        int processed_bytes = 0;

        // Process all complete messages in the buffer
        while (processed_bytes < stored_bytes) {
            char *current_msg = buffer + processed_bytes;
            if (*current_msg == '\0' || *current_msg == '\n' || *current_msg == '\r') {
                processed_bytes++;
                continue;
            }
            // If there are no bytes left to process, break
            int remaining_bytes = stored_bytes - processed_bytes;
            if (remaining_bytes < 1) break;

            // If the message length s 
            int msg_len = msg_get_size(current_msg);
            if (msg_len <= 0) {
                // Plus one byte and try to find the next
                processed_bytes++;
                continue;
            }
            // If message is incomplete, stop and wait for the rest
            if (msg_len > remaining_bytes) break;

            // Route message to correct handler
            switch (msg_get_type(current_msg)) {
                case MSG_MOVEMENT:
                    process_message_movement(current_msg, thread_arg);
                    break;
                case MSG_PLAYERS:
                    process_message_players(current_msg, thread_arg);
                    break;
                case MSG_ITEM:
                    process_message_item(current_msg, thread_arg);
                    break;
                case MSG_SYSTEM:
                    process_message_system(current_msg, thread_arg);
                    break;
                case MSG_APPLIANCE:
                    process_message_appliance(current_msg, thread_arg);
                    break;
                case MSG_COUNTER:
                    process_message_counter(current_msg, thread_arg);
                    break;
                case MSG_CUSTOMER:
                    process_message_customer(current_msg, thread_arg);
                    break;
                case MSG_SCORE:
                    process_message_score(current_msg, thread_arg);
                    break;
                case MSG_CONNECTION:
                    process_message_connection(current_msg, thread_arg);
                    break;
                default:
                    break;
            }
            // Move to next message
            processed_bytes += msg_len;
        }

        // Handle leftovers (fragmentation)
        if (processed_bytes < stored_bytes) {
            // Move partial message to the start of the buffer
            int remaining = stored_bytes - processed_bytes;
            memmove(buffer, buffer + processed_bytes, remaining);
            stored_bytes = remaining;
        } else {
            // All data processed, clear buffer
            stored_bytes = 0;
        }
    }
    return NULL;
}

int main(int argc, char **argv) {
    // Creates the client socket
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) fail("Error creating socket\n");

    // Create the address struct and fill it
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Convert the addr to binary
    char ip_string[16];
    if(argc >= 2) strncpy(ip_string, argv[1], 16);
    else strcpy(ip_string, "127.0.0.1"); // localhost
    if (inet_pton(AF_INET, ip_string, &server_addr.sin_addr) <= 0) fail("IP address error\n");

    // Connect client and server
    int status = connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (status < 0) fail("Connection error\n");

    // Initialize thread arguments
    THREAD_ARG_STRUCT *thread_arg = malloc(sizeof(THREAD_ARG_STRUCT));
    thread_arg->num_players = 0;
    thread_arg->num_customers = 0;
    thread_arg->current_debug_line = 0;
    thread_arg->client_fd = client_fd;
    thread_arg->score = 0;
    thread_arg->should_leave = 0;
    if(argc >= 3 && strcmp(argv[2], "-debug") == 0) thread_arg->render_debug = 1;
    else thread_arg->render_debug = 0;

    //inicialize all mutex
    pthread_mutex_init(&thread_arg->players_mutex, NULL);
    pthread_mutex_init(&thread_arg->debug_mutex, NULL);
    pthread_mutex_init(&thread_arg->appliances_mutex, NULL);
    pthread_mutex_init(&thread_arg->counters_mutex, NULL);
    pthread_mutex_init(&thread_arg->customers_mutex, NULL);
    pthread_mutex_init(&thread_arg->score_mutex, NULL);
    pthread_mutex_init(&thread_arg->leave_mutex, NULL);

    // Initialize all players
    for (int i = 0; i < MAX_PLAYERS; i++) {
        thread_arg->players[i].is_active = 0;
        thread_arg->players[i].is_me = 0;
        thread_arg->players[i].x = -1;
        thread_arg->players[i].y = -1;
        thread_arg->players[i].last_x = -1;
        thread_arg->players[i].last_y = -1;
        thread_arg->players[i].item = NONE;
    }

    // Inicialize appliances
    thread_arg->num_appliances = init_appliances(thread_arg->appliances);

    // Inicialize counters
    thread_arg->num_counters = init_counters(thread_arg->counters);

    // Inicialize customers
    thread_arg->num_customers = init_customers(thread_arg->customers);

    //Inicialize score
    thread_arg->score = 0;

    // Initialize debug
    for (int i = 0; i < 10; i++) {
        thread_arg->debug[i][0] = '\0';
    }

    // Create threads to send and receive data
    pthread_t read_thr, curses_thr;
    pthread_create(&read_thr, NULL, socket_read_thread, thread_arg);
    pthread_create(&curses_thr, NULL, curses, thread_arg);

    // Wait for them to finish
    pthread_join(read_thr, NULL);
    pthread_join(curses_thr, NULL);

    // Free space
    free(thread_arg);

    // End the connection and close the socket
    close(client_fd);
    return 0;
}
