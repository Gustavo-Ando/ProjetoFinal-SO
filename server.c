#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <unistd.h>
#include <time.h>
#include <poll.h>
#include <sys/types.h>

#include "server_send_message.h"
#include "server_game_logic.h"
#include "message.h"
#include "map.h"
#include "utility.h"
#include "server.h"

#define PORT 8080

/*
    Thread to read data in master_socket
    Responsible for dealing with incoming connections, adding it to client list
    Params:
        - void *args (INDEXED_THREAD_ARG_STRUCT): struct containing shared data and master_socket index
    Return:
        -
*/
static void *read_master_socket(void *args) {
    INDEXED_THREAD_ARG_STRUCT indexed_struct = *(INDEXED_THREAD_ARG_STRUCT *)args; // Cast
    THREAD_ARG_STRUCT *thread_arg = indexed_struct.thread_arg;
    // In this case, socket_index is actually the fd to the master_socket
    int master_socket = indexed_struct.socket_index;
    int addr_len = sizeof(thread_arg->address);

    // Create pollfd to poll if socket can be read
    struct pollfd pollfds;
    pollfds.fd = master_socket;
    pollfds.events = POLLIN;
    while (1) {
        // Check if should terminate thread (access leave CR)
        pthread_mutex_lock(&thread_arg->leave_mutex);
        int leave = thread_arg->should_leave;
        pthread_mutex_unlock(&thread_arg->leave_mutex);
        if(leave) break;
        
        // Poll socket with 1 second of timeout, restarting loop if timeout or error to check if should terminate
        int poll_timeout = 0.25 * 1000; // In msec
        if (poll(&pollfds, 1, poll_timeout) == 0) continue; // If timeout 
        else if(pollfds.revents != POLLIN) continue; // Error
        
        // Accepts the connection
        int new_socket = accept(master_socket, (struct sockaddr *)&thread_arg->address, (socklen_t *)&addr_len);
        if (new_socket < 0) fail("Accept failed");
        int flag = 1;
        int result = setsockopt(new_socket, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
        if (result < 0) fail("Error on TCP_NODELAY configuration");

        printf("New connection!\n");
        printf(" - Socket FD: %d\n", new_socket);
        printf(" - IP: %s:%d\n", inet_ntoa(thread_arg->address.sin_addr), ntohs(thread_arg->address.sin_port));

        // Access client CR to read number of connected clients and check if a connection is possible
        int is_full = 0;
        pthread_mutex_lock(&thread_arg->clients_mutex);
        if (thread_arg->connected == MAX_PLAYERS) is_full = 1;
        pthread_mutex_unlock(&thread_arg->clients_mutex);

        // If reached the limit, reject the connection and continue
        if (is_full) {
            printf(" - Rejected: Server full.\n\n");
            // Send disconnection message
            char message[MESSAGE_SIZE];
            msgS_connection(message, 0);
            send_message(new_socket, message);
            continue;
        }

        // Access client CR to increase number of connected clients and add client to the list
        pthread_mutex_lock(&thread_arg->clients_mutex);
        int new_index;
        for (int i = 0; i < MAX_PLAYERS; i++) {
            // If the index is already in use, tries the next one
            if (thread_arg->clients[i].socket != 0) continue;
            thread_arg->clients[i].socket = new_socket;
            // Defines initial position and item
            thread_arg->clients[i].x = 6;
            thread_arg->clients[i].y = 5;
            thread_arg->clients[i].item = NONE;
            new_index = i;
            // Increases the number of connected players
            thread_arg->connected++;
            printf(" - Adding socket as %d\n\n", i);
            break;
        }
        pthread_mutex_unlock(&thread_arg->clients_mutex);

        // Send all data to new client
        send_game_state(thread_arg, new_index);
        // Update other clients of the new player
        broadcast_player_connection(thread_arg, new_index);
        broadcast_player_position(thread_arg, new_index);
        broadcast_player_item(thread_arg, new_index);
    }

	return NULL;
}

/*
    Thread to read data from a client
    Responsible for dealing with disconnection (remove from client list), and receiving input
    Params:
        - void *args (INDEXED_THREAD_ARG_STRUCT): struct containing shared data and client index
    Return:
        -
*/
static void *read_client_socket(void *args) {
    INDEXED_THREAD_ARG_STRUCT indexed_struct = *(INDEXED_THREAD_ARG_STRUCT *)args; // Cast
    THREAD_ARG_STRUCT *thread_arg = indexed_struct.thread_arg;
    int index = indexed_struct.socket_index;
    int addr_len = sizeof(thread_arg->address);
    
    // Repeat to check if socket exists
    while (1) {
        // Check if should terminate thread (access leave CR)
        pthread_mutex_lock(&thread_arg->leave_mutex);
        int leave = thread_arg->should_leave;
        pthread_mutex_unlock(&thread_arg->leave_mutex);
        if(leave) break;
        
        // Access client CR to read the socket
        pthread_mutex_lock(&thread_arg->clients_mutex);
        int sd = thread_arg->clients[index].socket;
        pthread_mutex_unlock(&thread_arg->clients_mutex);
        if (sd == 0) continue;

        // Create pollfd to poll if socket can be read
        struct pollfd pollfds;
        pollfds.fd = sd;
        pollfds.events = POLLIN;
        
		// Enlarged buffer to handle fragmentation and multiple messages
		char buffer[MESSAGE_SIZE * 32]; 
		int stored_bytes = 0; // Bytes currently kept in buffer

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
			int read_length = read(sd, buffer + stored_bytes, bytes_to_read);
			
			// Verify disconnection
			if (read_length <= 0) { 
				// Disconnected
                getpeername(sd, (struct sockaddr *)&thread_arg->address, (socklen_t *)&addr_len);
                printf("Host disconnected.\n");
                printf(" - IP: %s:%d\n\n", inet_ntoa(thread_arg->address.sin_addr), ntohs(thread_arg->address.sin_port));

                // close the socket
                close(sd);

                // Access CR to disconnect the client
                pthread_mutex_lock(&thread_arg->clients_mutex);
                thread_arg->clients[index].socket = 0;
                thread_arg->connected--;
                pthread_mutex_unlock(&thread_arg->clients_mutex);
                // Update player's connection
                broadcast_player_connection(thread_arg, index);
                break;
				
			}

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
				int remaining_bytes = stored_bytes - processed_bytes;
				if (remaining_bytes < 1) break;

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
                    case MSG_INPUT:
                        treat_client_input(thread_arg, msgC_input_get_input(current_msg), index);
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
    }

    return NULL;
}


#define CUSTOMER_SPAWN_INTERVAL 8 
/*
    Thread to handle timers and game logic
    Responsible for dealing with customer's arrivals and timers, and appliance's timers
    Params:
        - void *arg (THREAD_ARG_STRUCT): struct containing shared data
    Return:
        -
*/
static void *game_loop_thread(void *arg) {
    THREAD_ARG_STRUCT *thread_arg = (THREAD_ARG_STRUCT *)arg;
	
	struct timespec start;
	clock_gettime(CLOCK_MONOTONIC, &start);
	long int last_customer_tick = 0;
	long int last_customer_spawn = 0;
    while (1) {
        // Check if should terminate thread (access leave CR)
        pthread_mutex_lock(&thread_arg->leave_mutex);
        int leave = thread_arg->should_leave;
        pthread_mutex_unlock(&thread_arg->leave_mutex);
        if(leave) break;

        // Get time
		struct timespec now;
		clock_gettime(CLOCK_MONOTONIC, &now);
		int diff_time = seconds_between(start, now);

		// If CUSTOMER_SPAWN_INTERVAL seconds have passed since last spawn, spawn a new customer
		if(diff_time >= last_customer_spawn + CUSTOMER_SPAWN_INTERVAL) {
			last_customer_spawn = diff_time;
			create_customer(thread_arg);
		}

		// If 1 second has passed since last tick, tick customers
		if(diff_time >= last_customer_tick + 1) {
			last_customer_tick = diff_time;
			tick_customers(thread_arg);
		}
        
		// Tick appliances (verification of time elapsed is handled internally for each appliance)
		tick_appliances(thread_arg);
    }
    return NULL;
}

/*
    Thread to handle keyboard input and deal with terminating server
    Responsible for reading input and setting should_leave on enter, broadcasting to all clients a disconnectarch linux install curses
    Params:
        - void *arg (THREAD_ARG_STRUCT): struct containing shared data
    Return:
        -
*/
static void *read_input(void *arg) {
    THREAD_ARG_STRUCT *thread_arg = (THREAD_ARG_STRUCT *)arg;

    while(1){
        char read_char;
        scanf("%c", &read_char);

        // If pressed enter, terminate
        if(read_char == '\n') {
            // Notify all clients of disconnection
            broadcast_disconnect(thread_arg);
            
            // Terminate server (access leave CR)
            pthread_mutex_lock(&thread_arg->leave_mutex);
            thread_arg->should_leave = 1;
            pthread_mutex_unlock(&thread_arg->leave_mutex);
            break;
        }
    }

    return NULL;
}

int main(int argc, char **argv) {
    srand(time(NULL));

    THREAD_ARG_STRUCT *thread_arg = malloc(sizeof(THREAD_ARG_STRUCT));

    //Inicialize mutex
    pthread_mutex_init(&thread_arg->clients_mutex, NULL);
    pthread_mutex_init(&thread_arg->appliances_mutex, NULL);
    pthread_mutex_init(&thread_arg->counters_mutex, NULL);
    pthread_mutex_init(&thread_arg->customers_mutex, NULL);
    pthread_mutex_init(&thread_arg->score_mutex, NULL);
    pthread_mutex_init(&thread_arg->leave_mutex, NULL);


    // Initialize clients
    thread_arg->connected = 0;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        thread_arg->clients[i].socket = 0;
    }

    // Initialize appliances
    thread_arg->num_appliances = init_appliances(thread_arg->appliances);

    // Initialize counters
    thread_arg->num_counters = init_counters(thread_arg->counters);

    // Initialize customers
    thread_arg->num_customers = init_customers(thread_arg->customers);

    // Initialize score
    thread_arg->score = 0;

    // Initialize leave
    thread_arg->should_leave = 0;
    
    // Initialize server socket
    int master_socket, opt = 1;
    if (!(master_socket = socket(AF_INET, SOCK_STREAM, 0))) fail("Socket failed");
    if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) fail("Setsockopt failed");

    // Set address
    thread_arg->address.sin_family = AF_INET;
    thread_arg->address.sin_addr.s_addr = INADDR_ANY;
    thread_arg->address.sin_port = htons(PORT);

    // Bind and listen socket
    if (bind(master_socket, (struct sockaddr *)&thread_arg->address, sizeof(thread_arg->address)) < 0) fail("Bind failed");
    if (listen(master_socket, 3) < 0) fail("Listen failed");

    printf("Waiting for connections...\n");

    // Create a thread for the server
    INDEXED_THREAD_ARG_STRUCT *master_arg_struct = malloc(sizeof(INDEXED_THREAD_ARG_STRUCT));
    master_arg_struct->socket_index = master_socket;
    master_arg_struct->thread_arg = thread_arg;
    pthread_t master_thr;
    pthread_create(&master_thr, NULL, read_master_socket, master_arg_struct);

    // Create a thread for each of the clients
    pthread_t client_thr[MAX_PLAYERS];
    // Keeps a list of indexed structs (passing by reference)
    INDEXED_THREAD_ARG_STRUCT *indexed_arg_struct = malloc(4 * sizeof(INDEXED_THREAD_ARG_STRUCT));
    for (int i = 0; i < MAX_PLAYERS; i++) {
        indexed_arg_struct[i].socket_index = i;
        indexed_arg_struct[i].thread_arg = thread_arg;
        pthread_create(&client_thr[i], NULL, read_client_socket, &indexed_arg_struct[i]);
    }

    // Create Game Loop Thread
    pthread_t game_thr;
    pthread_create(&game_thr, NULL, game_loop_thread, thread_arg);

    // Create input Thread
    pthread_t input_thr;
    pthread_create(&input_thr, NULL, read_input, thread_arg);
    
    // Join threads
    pthread_join(game_thr, NULL);
    pthread_join(input_thr, NULL);
    pthread_join(master_thr, NULL);
    for (int i = 0; i < MAX_PLAYERS; i++) pthread_join(client_thr[i], NULL);

    // Free space
    free(indexed_arg_struct);
    free(master_arg_struct);
    free(thread_arg);

    return 0;
}