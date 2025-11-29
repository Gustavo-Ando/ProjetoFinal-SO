#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <netinet/tcp.h>

#include <pthread.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>

#include "server_send_message.h"
#include "message.h"
#include "map.h"
#include "utility.h"
#include "server.h"

#define PORT 8080

#define TIME_TO_COOK 5
#define TIME_TO_BURN 14

/*
    Thread to read data in master_socket
    Responsible for dealing with incoming connections, adding it to client list
    Params:
        - void *args (INDEXED_THREAD_ARG_STRUCT): struct containing shared data and master_socket index
    Return:
        -
*/
static void *read_master_socket(void *args)
{
    INDEXED_THREAD_ARG_STRUCT indexed_struct = *(INDEXED_THREAD_ARG_STRUCT *)args; // Cast
    THREAD_ARG_STRUCT *thread_arg = indexed_struct.thread_arg;
    // In this case, socket_index is actually the fd to the master_socket
    int master_socket = indexed_struct.socket_index;
    int addr_len = sizeof(thread_arg->address);

    while (1)
    {
        // Accepts the connection
        int new_socket = accept(master_socket, (struct sockaddr *)&thread_arg->address, (socklen_t *)&addr_len);
        if (new_socket < 0) fail("Accept failed");
        int flag = 1;
        int result = setsockopt(new_socket, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
        if (result < 0) {
            perror("Erro ao configurar TCP_NODELAY");
        }
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
            char *message = "Connection rejected";
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

// [TODO] mover essas 2 func pra um arquivo separado
/*
    Funcion to do the application interaction
    Responsible for updating applications' and players' itens, combining itens and starting apps
    Params:
        - THREAD_ARG_STRUCT *thread_arg: struct containing shared data
        - int px: player's x coord
        - int py: player's y coord
        - int index: index of the client responsible for the action
    Return:
        - int app_updated_index: index of the updated app
*/
int app_interaction(THREAD_ARG_STRUCT *thread_arg, int px, int py, int index){
    int app_updated_index = -1;
    int app_id = get_appliance_id_at(px, py);
    mutex_lock_both(&thread_arg->appliances_mutex, &thread_arg->clients_mutex);

    // Player's item
    enum Item_type p_item = thread_arg->clients[index].item;

    if (p_item != NONE && thread_arg->appliances[app_id].content != NONE) { // If appliance and player both have an item, try to combine
        enum Item_type combined = try_combine(p_item, thread_arg->appliances[app_id].content);

        if (combined != NONE) {
            // Give the combined item to the player
            thread_arg->clients[index].item = combined;
            thread_arg->appliances[app_id].state = EMPTY;
            thread_arg->appliances[app_id].content = NONE;
            thread_arg->appliances[app_id].time_left = 0;
            app_updated_index = app_id;
        }
    } else if (thread_arg->appliances[app_id].state == EMPTY) { // Player puts the item on the appliance
        int success = 0;

        // Check if item and appliance match
        if (thread_arg->appliances[app_id].type == APP_OVEN && p_item == HAMBURGER) {
            thread_arg->appliances[app_id].content = HAMBURGER;
            success = 1;
        } else if (thread_arg->appliances[app_id].type == APP_FRYER && p_item == FRIES) {
            thread_arg->appliances[app_id].content = FRIES;
            success = 1;
        }
        // If success, update player item and app state
        if (success) {
            thread_arg->clients[index].item = NONE;
            thread_arg->appliances[app_id].state = COOKING;
            thread_arg->appliances[app_id].start_time = time(NULL);
            thread_arg->appliances[app_id].time_left = TIME_TO_COOK;
            app_updated_index = app_id;
        }
    } else if ((thread_arg->appliances[app_id].state != COOKING) && p_item == NONE) { // Player gets the item from app if it's done cooking
        thread_arg->clients[index].item = thread_arg->appliances[app_id].content;
        thread_arg->appliances[app_id].state = EMPTY;
        thread_arg->appliances[app_id].content = NONE;
        thread_arg->appliances[app_id].time_left = 0;
        app_updated_index = app_id;
    }
    
    mutex_unlock_both(&thread_arg->appliances_mutex, &thread_arg->clients_mutex);
    return app_updated_index;
}

/*
    Funcion to do the counter interaction
    Responsible for updating counter' and players' itens and combining itens
    Params:
        - THREAD_ARG_STRUCT *thread_arg: struct containing shared data
        - int px: player's x coord
        - int py: player's y coord
        - int index: index of the client responsible for the action
    Return:
        - int counter_updated_index:  index of the updated counter
*/
int counter_interaction(THREAD_ARG_STRUCT *thread_arg, int px, int py, int index){
    mutex_lock_both(&thread_arg->counters_mutex, &thread_arg->clients_mutex);
    
    int counter_id = get_counter_id_at(px, py);
    int counter_updated_index = -1;
    
    enum Item_type player_item = thread_arg->clients[index].item;
    enum Item_type counter_item = thread_arg->counters[counter_id].content;

    // If palyer and counter both have an item, try to combine
    if (player_item != NONE && counter_item != NONE) {
        enum Item_type combined = try_combine(player_item, counter_item);
        // If combination was successful, update both player and counter itens
        // Player gets the combined item, counter is now empty
        if (combined != NONE) {
            thread_arg->clients[index].item = combined;
            thread_arg->counters[counter_id].content = NONE;
            counter_updated_index = counter_id;
        }
        // If combination was not a success, nothing to do
    } else if (player_item != NONE && counter_item == NONE) { // Place the player item on empty counter
        thread_arg->counters[counter_id].content = player_item;
        thread_arg->clients[index].item = NONE;
        counter_updated_index = counter_id;
    } else if (player_item == NONE && counter_item != NONE) { // Get an item from the counter
        thread_arg->clients[index].item = counter_item;
        thread_arg->counters[counter_id].content = NONE;
        counter_updated_index = counter_id;
    }

    mutex_unlock_both(&thread_arg->counters_mutex, &thread_arg->clients_mutex);
    return counter_updated_index;
}

/*
    Function to remove one instance of an item from a customer's order
    Params:
        - CUSTOMER *c: pointer to the customer
        - enum Item_type item: item to remove
    Return:
        - int: 0 if successful, -1 if not found or error
*/

static int customer_remove_order_item(CUSTOMER *c, enum Item_type item) {
    if (!c) return -1;
    if (c->order_size <= 0) return -1;
    // find first match
    for (int i = 0; i < c->order_size; ++i) {
        if (c->order[i] == item) {
            // shift left
            for (int j = i; j + 1 < c->order_size; ++j) {
                c->order[j] = c->order[j+1];
            }
            // clear tail
            c->order[c->order_size - 1] = NONE;
            c->order_size--;
            return 0;
        }
    }
    return -1; // not found
}


/*
    Function to do the customer interaction
    Responsible for updating customers' and players' itens, combining itens and giving score
    Params:
        - THREAD_ARG_STRUCT *thread_arg: struct containing shared data
        - int px: player's x coord
        - int py: player's y coord
        - int index: index of the client responsible for the action
    Return:
        - int score_gained: score gained from the interaction
*/
int customer_interaction(THREAD_ARG_STRUCT *thread_arg, int px, int py, int index) {

    int score_gained = 0;
    int customer_id = get_customer_id_at(px, py);

    // Flags to defer broadcasting until after unlocking
    int should_broadcast_customer = 0;
    int should_broadcast_score = 0;
    int target_customer_id = -1;

    // Lock shared resources
    mutex_lock_both(&thread_arg->customers_mutex, &thread_arg->clients_mutex);
    
    // Safety check: Validate distance
    if (customer_id >= 0) {
        CUSTOMER *ccheck = &thread_arg->customers[customer_id];
        int dx = ccheck->x - px;
        int dy = ccheck->y - py;
        if (abs(dx) > 2 || abs(dy) > 2) {
            // If too far, unlock and return
            mutex_unlock_both(&thread_arg->customers_mutex, &thread_arg->clients_mutex);
            return 0;
        }
    }

    enum Item_type player_item = thread_arg->clients[index].item;

    // Validate customer state and player item
    if (customer_id < 0 || !thread_arg->customers[customer_id].active || player_item == NONE) {
        mutex_unlock_both(&thread_arg->customers_mutex, &thread_arg->clients_mutex);
        return 0;
    }

    CUSTOMER *c = &thread_arg->customers[customer_id];

    // Attempt to deliver the item
    if (customer_remove_order_item(c, player_item) == 0) {
        // Remove item from player
        thread_arg->clients[index].item = NONE;

        // Score update
        pthread_mutex_lock(&thread_arg->score_mutex);
        thread_arg->score += 1;
        pthread_mutex_unlock(&thread_arg->score_mutex);
        
        // Set broadcast flags
        should_broadcast_score = 1;
        target_customer_id = customer_id;
        should_broadcast_customer = 1;

        // Check if the order is complete
        if (c->order_size == 0) {
            c->active = 0;
            c->delivered = 1;
            // Clear order array safely
            for (int j = 0; j < MAX_ORDER; j++) c->order[j] = NONE;
        }
    }

    // Unlock mutexes
    mutex_unlock_both(&thread_arg->customers_mutex, &thread_arg->clients_mutex);

    // Broadcast updates
    if (should_broadcast_customer) {
        broadcast_customer_update(thread_arg, target_customer_id);
        broadcast_player_item(thread_arg, index); // Update player's empty hand
    }
    
    if (should_broadcast_score) {
        broadcast_score(thread_arg);
    }

    return score_gained;
}

/*
    Function to treat input from a client
    Responsible for checking if a move or action is valid, processing it's effects and sending result to all clients
    Params:
        - THREAD_ARG_STRUCT *args: struct containing shared data
        - char input: character inputed from client
        - int index: index of client responsible for input
    Return:
        -
*/
static void treat_client_input(THREAD_ARG_STRUCT *thread_arg, char input, int index) {

    int try_moved = 0, try_action = 0;
    int app_updated_index = -1, counter_updated_index = -1, customer_updated_index = -1;

    switch (input) {
        // Movement (W, A, S, D)
        case 'w':
            try_moved = 1;
            pthread_mutex_lock(&thread_arg->clients_mutex);
            if (!(game_map[thread_arg->clients[index].y - 1][thread_arg->clients[index].x]))
                thread_arg->clients[index].y--;
            pthread_mutex_unlock(&thread_arg->clients_mutex);
            break;
        case 'a':
            try_moved = 1;
            pthread_mutex_lock(&thread_arg->clients_mutex);
            if (!(game_map[thread_arg->clients[index].y][thread_arg->clients[index].x - 2]))
                thread_arg->clients[index].x -= 2;
            pthread_mutex_unlock(&thread_arg->clients_mutex);
            break;
        case 's':
            try_moved = 1;
            pthread_mutex_lock(&thread_arg->clients_mutex);
            if (!(game_map[thread_arg->clients[index].y + 1][thread_arg->clients[index].x]))
                thread_arg->clients[index].y++;
            pthread_mutex_unlock(&thread_arg->clients_mutex);
            break;
        case 'd':
            try_moved = 1;
            pthread_mutex_lock(&thread_arg->clients_mutex);
            if (!(game_map[thread_arg->clients[index].y][thread_arg->clients[index].x + 2]))
                thread_arg->clients[index].x += 2;
            pthread_mutex_unlock(&thread_arg->clients_mutex);
            break;
    
        // Interaction
        case ' ':
            try_action = 1;

            // Access client's CR to get position, item, and item in map
            pthread_mutex_lock(&thread_arg->clients_mutex);
            int px = thread_arg->clients[index].x;
            int py = thread_arg->clients[index].y;
            enum Item_type client_item = thread_arg->clients[index].item;
            enum Item_type map_item = item_map[py][px];
            pthread_mutex_unlock(&thread_arg->clients_mutex);

            // Check action to perform
            if (map_item != NONE && client_item == NONE){ // Get itens from map
                pthread_mutex_lock(&thread_arg->clients_mutex);
                thread_arg->clients[index].item = map_item;
                pthread_mutex_unlock(&thread_arg->clients_mutex);
            } else if (map_item != NONE && client_item != NONE) { // Try to combine itens
                enum Item_type combined = try_combine(client_item, map_item);
    
                pthread_mutex_lock(&thread_arg->clients_mutex);
                if (combined != NONE) thread_arg->clients[index].item = combined;
                pthread_mutex_unlock(&thread_arg->clients_mutex);
            } else if (trash_map[py][px]) { // Trash item
                pthread_mutex_lock(&thread_arg->clients_mutex);
                thread_arg->clients[index].item = NONE;
                pthread_mutex_unlock(&thread_arg->clients_mutex);
            } else if(appliance_interaction_map[py][px] != -1){ // Appliance interaction
                app_updated_index = app_interaction(thread_arg, px, py, index);
            } else if(counter_interaction_map[py][px] != -1){ // Counter interaction
                counter_updated_index = counter_interaction(thread_arg, px, py, index);
            } else if(customer_interaction_map[py][px] != -1){ // Customer interaction
                customer_updated_index = customer_interaction(thread_arg, px, py, index);
            }
            break;
    
        default:
            break;
    }

    printf("Received input from client %d: %c\n", index, input);
    printf(" - %d, %d\n", thread_arg->clients[index].x, thread_arg->clients[index].y);

    if (try_moved) broadcast_player_position(thread_arg, index);
    if (try_action) broadcast_player_item(thread_arg, index);
    if (app_updated_index != -1) broadcast_appliance_status(thread_arg, app_updated_index);
    if (counter_updated_index != -1) broadcast_counter_update(thread_arg, counter_updated_index);
    if (customer_updated_index != -1) broadcast_customer_update(thread_arg, customer_updated_index);
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
        // Access client CR to read the socket
        pthread_mutex_lock(&thread_arg->clients_mutex);
        int sd = thread_arg->clients[index].socket;
        pthread_mutex_unlock(&thread_arg->clients_mutex);
        if (sd == 0)
            continue;
        // If exists, treat client until it disconnects
        while (1) {
            char buffer[MESSAGE_SIZE + 1];
            // Keeps track of the size of the message received
            int read_length = read(sd, buffer, MESSAGE_SIZE);
            buffer[read_length] = '\0'; // Null terminate message

            // If there is no message, the client disconnected
            if (read_length == 0) {
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
            else {
                // Index to read message
                int current_index = 0;
                // While is not the end of the message
                while (current_index < MESSAGE_SIZE && current_index < read_length && buffer[current_index] != '\0') {
                    // Treat it based on its type
                    switch (msg_get_type(buffer + current_index)) {
                    case MSG_INPUT:
                        treat_client_input(thread_arg, msgC_input_get_input(buffer + current_index), index);
                        break;
                    default:
                        buffer[current_index] = '\0';
                        break;
                    }
                    // Updates index given the message size to deal with concatenated messages
                    current_index += msg_get_size(buffer + current_index);
                }
            }
        }
    }

    return NULL;
}


#define CUSTOMER_SPAWN_INTERVAL 8   // seconds between customer spawns
static int customer_spawn_timer = 0;
static int tick_counter = 0;

/*
    Thread to handle game physics and timers (Ovens/Fryers)
*/
static void *game_loop_thread(void *arg) {
    THREAD_ARG_STRUCT *thread_arg = (THREAD_ARG_STRUCT *)arg;

    while (1) {
        struct timespec ts;
        ts.tv_sec = 0; 
        ts.tv_nsec = 100000000L; //0.1s
        nanosleep(&ts, NULL);

        tick_counter++;
        if (tick_counter < 10) 
            continue;
        tick_counter = 0;

        // Customer spawning
        customer_spawn_timer++;

        if (customer_spawn_timer >= CUSTOMER_SPAWN_INTERVAL) {
            customer_spawn_timer = 0;

            pthread_mutex_lock(&thread_arg->customers_mutex);

            // Search for an inactive customer slot
            int created = 0;
            for (int i = 0; i < thread_arg->num_customers; i++) {
                CUSTOMER *c = &thread_arg->customers[i];

                if (!c->active) {
                    // Activate customer
                    c->active = 1;
                    c->delivered = 0;

                    // Random time left (between 20 and 40 seconds)
                    c->time_left = 20 + rand() % 21;

                    // Random order size (between 1 and MAX_ORDER)
                    c->order_size = 1 + (rand() % MAX_ORDER);
                    if (c->order_size < 1) c->order_size = 1;
                    if (c->order_size > MAX_ORDER) c->order_size = MAX_ORDER;

                    // Generate random order
                    for (int j = 0; j < c->order_size; j++) {
                        int r = rand() % 5;
                        switch (r) {
                            case 0: c->order[j] = BURGER_BREAD; break;
                            case 1: c->order[j] = SALAD; break;
                            case 2: c->order[j] = JUICE; break;
                            case 3: c->order[j] = FULL_BURGER; break;
                            case 4: c->order[j] = FRIES_READY; break;
                        }
                    }   

                    // clear remaining order slots
                    for (int j = c->order_size; j < MAX_ORDER; ++j) {
                        c->order[j] = NONE;
                    }


                    c->id = i;

                    pthread_mutex_unlock(&thread_arg->customers_mutex);

                    // Send update to all clients
                    broadcast_customer_update(thread_arg, i);

                    created = 1;
                    break;
                }
            }

            if (!created) {
                pthread_mutex_unlock(&thread_arg->customers_mutex);
            }
        }

        int customers_to_update[MAX_CUSTOMERS];
        int update_count = 0;
        int score_changed = 0;

        // Customer timer logic
        pthread_mutex_lock(&thread_arg->customers_mutex);
        for (int i = 0; i < thread_arg->num_customers; i++) {
            CUSTOMER *c = &thread_arg->customers[i];
            
            if (c->active) {
                c->time_left--;
                
                if (c->time_left <= 0) {
                    c->time_left = 0;
                    c->active = 0;

                    //Minus 1 score
                    pthread_mutex_lock(&thread_arg->score_mutex);
                    thread_arg->score -= 1; 
                    pthread_mutex_unlock(&thread_arg->score_mutex);

                    score_changed = 1;
                    
                    customers_to_update[update_count++] = i;
                }
                
                customers_to_update[update_count++] = i;
            }
        }
        pthread_mutex_unlock(&thread_arg->customers_mutex);

        for (int k = 0; k < update_count; k++) {
            broadcast_customer_update(thread_arg, customers_to_update[k]);
        }

        if (score_changed) {
            broadcast_score(thread_arg);
        }
        
        // Appliances logic

        pthread_mutex_lock(&thread_arg->appliances_mutex);

        time_t now = time(NULL);

        for (int i = 0; i < thread_arg->num_appliances; i++) {
            int changed = 0;
            int current_time_elapsed = (int)difftime(now, thread_arg->appliances[i].start_time);
            int new_time_left = 0;

            // COOKING to READY
            if (thread_arg->appliances[i].state == COOKING) {
                new_time_left = TIME_TO_COOK - current_time_elapsed;
                if (new_time_left < 0)
                    new_time_left = 0;

                thread_arg->appliances[i].time_left = new_time_left;
                changed = 1;

                if (current_time_elapsed >= TIME_TO_COOK) {
                    thread_arg->appliances[i].state = READY;
                    thread_arg->appliances[i].content = (thread_arg->appliances[i].type == APP_OVEN) ? HAMBURGER_READY : FRIES_READY;
                    thread_arg->appliances[i].time_left = TIME_TO_BURN - current_time_elapsed;
                }
            }
            // READY to BURNED
            else if (thread_arg->appliances[i].state == READY) {
                new_time_left = TIME_TO_BURN - current_time_elapsed;
                if (new_time_left < 0)
                    new_time_left = 0;

                thread_arg->appliances[i].time_left = new_time_left;
                changed = 1;

                if (current_time_elapsed >= TIME_TO_BURN) {
                    thread_arg->appliances[i].state = BURNED;
                    thread_arg->appliances[i].content = (thread_arg->appliances[i].type == APP_OVEN) ? HAMBURGER_BURNED : FRIES_BURNED;
                    thread_arg->appliances[i].time_left = 0;
                }
            }

            // Warns all clients if there was a change
            if (changed) {
                pthread_mutex_unlock(&thread_arg->appliances_mutex); // Unlock before calling broadcast
                broadcast_appliance_status(thread_arg, i);
                pthread_mutex_lock(&thread_arg->appliances_mutex); // Lock again to go back to the loop
            }
        }

        pthread_mutex_unlock(&thread_arg->appliances_mutex);
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

    //Inicialize score
    thread_arg->score = 0;

    // Initialize server socket
    int master_socket, opt = 1;
    if (!(master_socket = socket(AF_INET, SOCK_STREAM, 0)))
        fail("Socket failed");
    if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
        fail("Setsockopt failed");

    // Set address
    thread_arg->address.sin_family = AF_INET;
    thread_arg->address.sin_addr.s_addr = INADDR_ANY;
    thread_arg->address.sin_port = htons(PORT);

    // Bind and listen socket
    if (bind(master_socket, (struct sockaddr *)&thread_arg->address, sizeof(thread_arg->address)) < 0)
        fail("Bind failed");
    if (listen(master_socket, 3) < 0)
        fail("Listen failed");

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

    // Join threads
    pthread_join(master_thr, NULL);
    for (int i = 0; i < MAX_PLAYERS; i++)
        pthread_join(client_thr[i], NULL);

    // Free space
    free(indexed_arg_struct);
    free(master_arg_struct);
    free(thread_arg);

    return 0;
}