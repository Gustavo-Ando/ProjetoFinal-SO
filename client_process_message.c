#include <pthread.h>
#include <stdio.h>

#include <stdlib.h>

#include "message.h"
#include "client_process_message.h"

#define CUSTOMER_SPAWN_X  20

/*
    Function to process item message from server
    Responsible for updating the player's item
    Params:
        - char *message: message received from server
        - THREAD_ARG_STRUCT *thread_arg: struct containing shared data
    Return:
        -
*/
void process_message_item(char *message, THREAD_ARG_STRUCT *thread_arg) {
    // Access player's CR
    pthread_mutex_lock(&thread_arg->players_mutex);
    // Get player index and item type
    int index = msgS_item_get_player_index(message);
    enum Item_type item = msgS_item_get_item_type(message);
    thread_arg->players[index].item = item; // Update item
    pthread_mutex_unlock(&thread_arg->players_mutex);

    // Access debug CR
    pthread_mutex_lock(&thread_arg->debug_mutex);
    sprintf(thread_arg->debug[thread_arg->current_debug_line], "ITEM %d:%c", index, item); // Add Debug message
    thread_arg->current_debug_line = (thread_arg->current_debug_line + 1) % 10;            // Update current line
    pthread_mutex_unlock(&thread_arg->debug_mutex);
}

/*
    Function to process movement message from server
    Responsible for updating the player's position (and last_position, where the item will be rendered)
    Params:
        - char *message: message received from server
        - THREAD_ARG_STRUCT *thread_arg: struct containing shared data
    Return:
        -
*/
void process_message_movement(char *message, THREAD_ARG_STRUCT *thread_arg) {
    // Access player's CR and update the given player's item
    pthread_mutex_lock(&thread_arg->players_mutex);
    // Get player index and position
    int index = msgS_movement_get_player_index(message), x = msgS_movement_get_x(message), y = msgS_movement_get_y(message);
    // Check if valid player and position is changed
    if ((index >= 0 && index < MAX_PLAYERS) && (x != thread_arg->players[index].x || y != thread_arg->players[index].y)) {
        // Update last position and position
        thread_arg->players[index].last_x = thread_arg->players[index].x;
        thread_arg->players[index].last_y = thread_arg->players[index].y;
        thread_arg->players[index].x = x;
        thread_arg->players[index].y = y;
        // If last position is invalid, set to current position
        if (thread_arg->players[index].last_x == -1 || thread_arg->players[index].last_y == -1) {
            thread_arg->players[index].last_x = x;
            thread_arg->players[index].last_y = y;
        }
    }
    pthread_mutex_unlock(&thread_arg->players_mutex);
}

/*
    Function to process player connection message from server
    Responsible for updating the player's connection
    Params:
        - char *message: message received from server
        - THREAD_ARG_STRUCT *thread_arg: struct containing shared data
    Return:
        -
*/
void process_message_players(char *message, THREAD_ARG_STRUCT *thread_arg) {
    // Access Debug CR
    pthread_mutex_lock(&thread_arg->debug_mutex);
    sprintf(thread_arg->debug[thread_arg->current_debug_line], "CON %d:%d.", msgS_players_get_player_index(message), msgS_players_get_status(message)); // Add debug message
    thread_arg->current_debug_line = (thread_arg->current_debug_line + 1) % 10;                                                                         // Update current debug line
    pthread_mutex_unlock(&thread_arg->debug_mutex);

    // Access Player CR and update the given player's status, updating number of players in the game
    pthread_mutex_lock(&thread_arg->players_mutex);
    // Get player index and status
    int index = msgS_players_get_player_index(message);
    int status = msgS_players_get_status(message);
    thread_arg->players[index].is_active = status; // Update status
    // Update player count
    if (status == 1) thread_arg->num_players++;
    else thread_arg->num_players--;
    pthread_mutex_unlock(&thread_arg->players_mutex);
}

/*
    Function to process system message from server
    Responsible for receiving player index of this client
    Params:
        - char *message: message received from server
        - THREAD_ARG_STRUCT *thread_arg: struct containing shared data
    Return:
        -
*/
void process_message_system(char *message, THREAD_ARG_STRUCT *thread_arg) {
    // Access debug CR
    pthread_mutex_lock(&thread_arg->debug_mutex);
    sprintf(thread_arg->debug[thread_arg->current_debug_line], "INDEX %d", msgS_system_get_player_index(message)); // Add debug message
    thread_arg->current_debug_line = (thread_arg->current_debug_line + 1) % 10;                                    // Update current debug line
    pthread_mutex_unlock(&thread_arg->debug_mutex);

    // Access player CR and for each player check if it's index corresponds to this client's index
    pthread_mutex_lock(&thread_arg->players_mutex);
    for (int i = 0; i < MAX_PLAYERS; i++) {
        thread_arg->players[i].is_me = (i == msgS_system_get_player_index(message));
    }
    pthread_mutex_unlock(&thread_arg->players_mutex);
}

/*
    Function to process appliance status message from server
    Responsible for updating the appliance state and content for rendering
    Params:
        - char *message: message received from server
        - THREAD_ARG_STRUCT *thread_arg: struct containing shared data
*/
void process_message_appliance(char *message, THREAD_ARG_STRUCT *thread_arg) {

    // Get data
    int index = msgS_appliance_get_index(message);
    int status = msgS_appliance_get_status(message);
    int time_left = msgS_appliance_get_time_left(message);
    
    pthread_mutex_lock(&thread_arg->appliances_mutex);

    if (index >= 0 && index < thread_arg->num_appliances) {
        thread_arg->appliances[index].state = status;
        thread_arg->appliances[index].time_left = time_left;

        // Update appliance's content
        int is_oven = thread_arg->appliances[index].type == APP_OVEN;
        switch (status) {
            case EMPTY:
                thread_arg->appliances[index].content = NONE;
                break;
            case COOKING:
                thread_arg->appliances[index].content = is_oven ? HAMBURGER : FRIES;
                break;
            case READY:
                thread_arg->appliances[index].content = is_oven ? HAMBURGER_READY : FRIES_READY;
                break;
            case BURNED:
                thread_arg->appliances[index].content = is_oven ? HAMBURGER_BURNED : FRIES_BURNED;
                break;
        }
    }

    pthread_mutex_unlock(&thread_arg->appliances_mutex);
}

/*
    Function to process counter status message from server
    Responsible for updating the counter state and content for rendering
    Params:
        - char *message: message received from server
        - THREAD_ARG_STRUCT *thread_arg: struct containing shared data
*/
void process_message_counter(char *message, THREAD_ARG_STRUCT *thread_arg) {

    // Get data
    int index = msgS_counter_get_counter_index(message);
    int item = msgS_counter_get_item_type(message);

    pthread_mutex_lock(&thread_arg->counters_mutex);

    if (index >= 0 && index < thread_arg->num_counters) {
        thread_arg->counters[index].content = item;
    }

    pthread_mutex_unlock(&thread_arg->counters_mutex);
}

/*

    Function to process customer arrival message from server
    Responsible for updating the customer data
    Params:
        - char *message: message received from server
        - THREAD_ARG_STRUCT *thread_arg: struct containing shared data
*/

void process_message_customer(char *message, THREAD_ARG_STRUCT *thread_arg)
{

    int id = msgS_customer_arrival_get_client_index(message);
    int order_size = msgS_customer_arrival_get_order_size(message);

    int x = message[3 + order_size] - '0';
    int y = message[4 + order_size] - '0';
    int state = message[5 + order_size] - '0';
    int time_left = message[6 + order_size] - '0';

    int *order = msgS_customer_arrival_get_order(message); 

    pthread_mutex_lock(&thread_arg->customers_mutex);

    CUSTOMER *c = &thread_arg->customers[id];
    c->id = id;

    // sets the position
    c->x = x;
    c->y = y;

    // copies the order
    c->order_size = order_size;
    for (int i = 0; i < order_size && i < MAX_ORDER; i++) {
        c->order[i] = (enum Item_type) order[i];
    }
    // if order_size < MAX_ORDER, fill the rest with NONE
    for (int i = order_size; i < MAX_ORDER; i++) {
        c->order[i] = NONE;
    }

    // applies state and time left
    c->active = state;
    c->time_left = time_left;

    // if the customer is not active, reset order
    if (!c->active) {
        c->order_size = 0;
        for (int i = 0; i < MAX_ORDER; i++) 
            c->order[i] = NONE;
    }

    pthread_mutex_unlock(&thread_arg->customers_mutex);

    free(order);
}

/*
    Function to process score arrival message from server
    Responsible for updating the score data
    Params:
        - char *message: message received from server
        - THREAD_ARG_STRUCT *thread_arg: struct containing shared data
*/
void process_message_score(char *message, THREAD_ARG_STRUCT *thread_arg) {
    int new_score = msgS_score_get_value(message);

    pthread_mutex_lock(&thread_arg->score_mutex);
    thread_arg->score = new_score;
    pthread_mutex_unlock(&thread_arg->score_mutex);

    // Debug
    pthread_mutex_lock(&thread_arg->debug_mutex);
    sprintf(thread_arg->debug[thread_arg->current_debug_line], "SCORE: %d", new_score);
    thread_arg->current_debug_line = (thread_arg->current_debug_line + 1) % 10;
    pthread_mutex_unlock(&thread_arg->debug_mutex);
}