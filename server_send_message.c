#include <pthread.h>
#include <string.h>

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
void send_game_state(THREAD_ARG_STRUCT *thread_arg, int index){
    // Access clients CR
    pthread_mutex_lock(&thread_arg->clients_mutex);
    // Send system message containing player index
    char message[MESSAGE_SIZE];
    msgS_system(message, index);
    if(send(thread_arg->clients[index].socket, message, strlen(message), 0) != strlen(message)) fail("Send error");

    // For each player, get it's data and send to player
    for(int i = 0; i < MAX_PLAYERS; i++){
        // Create a message for all clients informing their status (connected or not)
        msgS_players(message, i, thread_arg->clients[i].socket != 0);
        // Send and check if succesful
        if(send(thread_arg->clients[index].socket, message, strlen(message), 0) != strlen(message)) fail("Send error");
        // Create and send a message with connected players' position and item
        if(thread_arg->clients[i].socket != 0) {
            msgS_movement(message, i, thread_arg->clients[i].x, thread_arg->clients[i].y);
            if(send(thread_arg->clients[index].socket, message, strlen(message), 0) != strlen(message)) fail("Send error");
            msgS_item(message, i, thread_arg->clients[i].item);
            if(send(thread_arg->clients[index].socket, message, strlen(message), 0) != strlen(message)) fail("Send error");
        }
    }
    pthread_mutex_unlock(&thread_arg->clients_mutex);
}

/*
    Function to send a player connection status to all other clients
    Params:
        - THREAD_ARG_STRUCT *thread_arg: struct contating shared data
        - int index: index of client whose data will be sent
    Return:
        - 
*/
void broadcast_player_connection(THREAD_ARG_STRUCT *thread_arg, int index){
    // Access client CR
    pthread_mutex_lock(&thread_arg->clients_mutex);
    char message[MESSAGE_SIZE];
    //Create a message for the given client informing its status
    msgS_players(message, index, thread_arg->clients[index].socket != 0);
    for(int i = 0; i < MAX_PLAYERS; i++){
        // Send it to all connected players
        if(thread_arg->clients[i].socket != 0) {
            if(send(thread_arg->clients[i].socket, message, strlen(message), 0) != strlen(message)) fail("Send error");
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
void broadcast_player_position(THREAD_ARG_STRUCT *thread_arg, int index){
    // Access client CR
    pthread_mutex_lock(&thread_arg->clients_mutex);
    char message[MESSAGE_SIZE];
    // Create message for the given client informing its status
    msgS_movement(message, index, thread_arg->clients[index].x, thread_arg->clients[index].y);
    for(int i = 0; i < MAX_PLAYERS; i++){
        // Send it to all connected players
        if(thread_arg->clients[i].socket != 0) {
            if(send(thread_arg->clients[i].socket, message, strlen(message), 0) != strlen(message)) fail("Send error");
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
void broadcast_player_item(THREAD_ARG_STRUCT *thread_arg, int index){
    // Access client CR
    pthread_mutex_lock(&thread_arg->clients_mutex);
    char message[MESSAGE_SIZE];
    // Create message for the given client informing its status
    msgS_item(message, index, thread_arg->clients[index].item);
    for(int i = 0; i < MAX_PLAYERS; i++){
        // Send it to all connected players
        if(thread_arg->clients[i].socket != 0) {
            if(send(thread_arg->clients[i].socket, message, strlen(message), 0) != strlen(message)) fail("Send error");
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
    // Access client CR
    pthread_mutex_lock(&thread_arg->clients_mutex);
    
    // Busca o status atual direto da memória global
    int status = appliances[index].state;

    char message[MESSAGE_SIZE];
    msgS_appliance(message, index, status);
    
    for(int i = 0; i < MAX_PLAYERS; i++){
        if(thread_arg->clients[i].socket != 0) {
            if(send(thread_arg->clients[i].socket, message, strlen(message), 0) != strlen(message)) 
                fail("Send error");
        }
    }
    pthread_mutex_unlock(&thread_arg->clients_mutex);
}