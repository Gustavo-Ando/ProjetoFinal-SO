#include <pthread.h>
#include <string.h>
#include <stdio.h>

#include "server_send_message.h"
#include "utility.h"
#include "message.h"
#include "map.h"

/*
    Function to send current game state to a specific client
    Responsible for sending a client it's index, each players current connection, position and item
    Params:
        - THREAD_ARG_STRUCT *thread_arg: struct contating shared data
        - int index: index of client to send the data to
    Return:
        -
*/
void send_game_state(THREAD_ARG_STRUCT *thread_arg, int index) {
    // Access clients CR
    pthread_mutex_lock(&thread_arg->clients_mutex);
    // Send system message containing player index
    char message[MESSAGE_SIZE];
    msgS_system(message, index);
    send_message(thread_arg->clients[index].socket, message);

    // For each player, get it's data and send to player
    for (int i = 0; i < MAX_PLAYERS; i++) {
        // Create a message for all clients informing their status (connected or not)
        msgS_players(message, i, thread_arg->clients[i].socket != 0);
        // Send and check if succesful
        send_message(thread_arg->clients[index].socket, message);
        // Create and send a message with connected players' position and item
        if (thread_arg->clients[i].socket != 0) {
            msgS_movement(message, i, thread_arg->clients[i].x, thread_arg->clients[i].y);
            send_message(thread_arg->clients[index].socket, message);
            msgS_item(message, i, thread_arg->clients[i].item);
            send_message(thread_arg->clients[index].socket, message);
        }
    }
    pthread_mutex_unlock(&thread_arg->clients_mutex);

    // Send appliance data
    mutex_lock_both(&thread_arg->appliances_mutex, &thread_arg->clients_mutex);
    for(int i = 0; i < thread_arg->num_appliances; i++){
        int status = thread_arg->appliances[i].state;
        int time_left = thread_arg->appliances[i].time_left;
        msgS_appliance(message, i, status, time_left);
    
        send_message(thread_arg->clients[index].socket, message);
    }
    mutex_unlock_both(&thread_arg->appliances_mutex, &thread_arg->clients_mutex);

    // Send counter data
    mutex_lock_both(&thread_arg->counters_mutex, &thread_arg->clients_mutex);
    for(int i = 0; i < thread_arg->num_counters; i++){
        printf("Send counter %d\n", i);
        enum Item_type item = thread_arg->counters[i].content;
        msgS_counter(message, i, item);
        
        send_message(thread_arg->clients[index].socket, message);
    }
    mutex_unlock_both(&thread_arg->counters_mutex, &thread_arg->clients_mutex);

    // Send customer data
    
    // Send customer data
    mutex_lock_both(&thread_arg->customers_mutex, &thread_arg->clients_mutex);

    for (int i = 0; i < thread_arg->num_customers; i++) {

        // Pula customers inativos
        if (!thread_arg->customers[i].active)
            continue;

        CUSTOMER *c = &thread_arg->customers[i];

        int customer_id = c->id;
        enum Item_type *order = c->order;
        int order_size = c->order_size;

        int x = c->x;
        int y = c->y;
        int state = c->active;  
        int time_left = c->time_left;

        msgS_customer(
            message,
            customer_id,
            order,
            order_size,
            x,
            y,
            state,
            time_left
        );

        send_message(thread_arg->clients[index].socket, message);
    }

    //Send score data
    pthread_mutex_lock(&thread_arg->score_mutex);
    int s = thread_arg->score;
    pthread_mutex_unlock(&thread_arg->score_mutex);
    
    char msg_score[MESSAGE_SIZE];
    msgS_score(msg_score, s);
    send_message(thread_arg->clients[index].socket, msg_score);

    mutex_unlock_both(&thread_arg->customers_mutex, &thread_arg->clients_mutex);


}

/*
    Function to send a player connection status to all other clients
    Params:
        - THREAD_ARG_STRUCT *thread_arg: struct contating shared data
        - int index: index of client whose data will be sent
    Return:
        -
*/
void broadcast_player_connection(THREAD_ARG_STRUCT *thread_arg, int index) {
    // Access client CR
    pthread_mutex_lock(&thread_arg->clients_mutex);
    char message[MESSAGE_SIZE];
    // Create a message for the given client informing its status
    msgS_players(message, index, thread_arg->clients[index].socket != 0);
    for (int i = 0; i < MAX_PLAYERS; i++) {
        // Send it to all connected players
        if (thread_arg->clients[i].socket != 0) {
            send_message(thread_arg->clients[i].socket, message);
        }
    }
    pthread_mutex_unlock(&thread_arg->clients_mutex);
}

/*
    Function to send a player position to all other clients
    Params:
        - THREAD_ARG_STRUCT *thread_arg: struct contating shared data
        - int index: index of client whose data will be sent
    Return:
        -
*/
void broadcast_player_position(THREAD_ARG_STRUCT *thread_arg, int index) {
    // Access client CR
    pthread_mutex_lock(&thread_arg->clients_mutex);
    char message[MESSAGE_SIZE];
    // Create message for the given client informing its status
    msgS_movement(message, index, thread_arg->clients[index].x, thread_arg->clients[index].y);
    for (int i = 0; i < MAX_PLAYERS; i++) {
        // Send it to all connected players
        if (thread_arg->clients[i].socket != 0) {
            send_message(thread_arg->clients[i].socket, message);
        }
    }
    pthread_mutex_unlock(&thread_arg->clients_mutex);
}

/*
    Function to send a player current item to all other clients
    Params:
        - THREAD_ARG_STRUCT *thread_arg: struct contating shared data
        - int index: index of client whose data will be sent
    Return:
        -
*/
void broadcast_player_item(THREAD_ARG_STRUCT *thread_arg, int index) {
    // Access client CR
    pthread_mutex_lock(&thread_arg->clients_mutex);
    char message[MESSAGE_SIZE];
    // Create message for the given client informing its status
    msgS_item(message, index, thread_arg->clients[index].item);
    for (int i = 0; i < MAX_PLAYERS; i++) {
        // Send it to all connected players
        if (thread_arg->clients[i].socket != 0) {
            send_message(thread_arg->clients[i].socket, message);
        }
    }
    pthread_mutex_unlock(&thread_arg->clients_mutex);
}

/*
    Function to send an appliance status update to all clients
    Params:
        - THREAD_ARG_STRUCT *thread_arg: struct contating shared data
        - int index: index of the appliance to update
    Return:
        -
*/
void broadcast_appliance_status(THREAD_ARG_STRUCT *thread_arg, int index){
    
    pthread_mutex_lock(&thread_arg->appliances_mutex);
    int status = thread_arg->appliances[index].state;
    int time_left = thread_arg->appliances[index].time_left;

    char message[MESSAGE_SIZE];
    msgS_appliance(message, index, status, time_left);
    pthread_mutex_unlock(&thread_arg->appliances_mutex);
    
    pthread_mutex_lock(&thread_arg->clients_mutex);
    for(int i = 0; i < MAX_PLAYERS; i++){
        if(thread_arg->clients[i].socket != 0) {
            send_message(thread_arg->clients[i].socket, message);
        }
    }
    pthread_mutex_unlock(&thread_arg->clients_mutex);
}

/*
    Function to send an counter status update to all clients
    Params:
        - THREAD_ARG_STRUCT *thread_arg: struct contating shared data
        - int index: index of the counter to update
    Return:
        -
*/
void broadcast_counter_update(THREAD_ARG_STRUCT *thread_arg, int counter_index) {
    pthread_mutex_lock(&thread_arg->counters_mutex);

    char message[MESSAGE_SIZE];
    enum Item_type item = thread_arg->counters[counter_index].content;

    msgS_counter(message, counter_index, item);
    pthread_mutex_unlock(&thread_arg->counters_mutex);

    pthread_mutex_lock(&thread_arg->clients_mutex);
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (thread_arg->clients[i].socket != 0) {
            send_message(thread_arg->clients[i].socket, message);
        }
    }

    pthread_mutex_unlock(&thread_arg->clients_mutex);
}

/*
    Function to broadcast a customer update to all clients
    Params:
        - THREAD_ARG_STRUCT *thread_arg: shared game state
        - int customer_index: index of the updated customer
    Return:
        -
*/
void broadcast_customer_update(THREAD_ARG_STRUCT *thread_arg, int customer_index)
{
    char message[MESSAGE_SIZE];

    // Lock customers
    pthread_mutex_lock(&thread_arg->customers_mutex);

    CUSTOMER *c = &thread_arg->customers[customer_index];

    msgS_customer(
        message,
        c->id,
        c->order,
        c->order_size,
        c->x,
        c->y,
        c->active,
        c->time_left
    );

    pthread_mutex_unlock(&thread_arg->customers_mutex);

    // Lock clients and broadcast to all connected players
    pthread_mutex_lock(&thread_arg->clients_mutex);

    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (thread_arg->clients[i].socket != 0) {
            send_message(thread_arg->clients[i].socket, message);
        }
    }

    pthread_mutex_unlock(&thread_arg->clients_mutex);
}


void broadcast_score(THREAD_ARG_STRUCT *thread_arg) {
    char message[MESSAGE_SIZE];
    
    pthread_mutex_lock(&thread_arg->score_mutex);
    int current_score = thread_arg->score;
    pthread_mutex_unlock(&thread_arg->score_mutex);

    msgS_score(message, current_score);

    pthread_mutex_lock(&thread_arg->clients_mutex);
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (thread_arg->clients[i].socket != 0) {
            send_message(thread_arg->clients[i].socket, message);
        }
    }
    pthread_mutex_unlock(&thread_arg->clients_mutex);
}
    