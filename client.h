#ifndef CLIENT_H
#define CLIENT_H

#include <pthread.h>

#include "map.h"

// Struct with players' info
typedef struct _player {
    int x, y; // Current position
    int last_x, last_y; // Last position (on which itens will be rendered)
    enum Item_type item; // Current item
    int is_active, is_me; // 1 if active, 0 if not; 1 if index is client, 0 if not
} PLAYER;

// Struct with shared data between threads
typedef struct __thread_arg_struct {
    char buffer_send[64]; // Buffer with inputed character
    int buffer_send_size; // Keeps track of the current size of the buffer
    pthread_mutex_t buffer_send_mutex; // Mutex to access buffer
    
    PLAYER players[MAX_PLAYERS]; // Array of players
    int num_players; // Keeps track of number of players in the game
    pthread_mutex_t players_mutex; // Mutex to access players' shared info
    
    char debug[10][20]; // Array of strings containing debug information
    int current_debug_line; // Index of current debug line
    pthread_mutex_t debug_mutex; // Mutex to access debug lines

    APPLIANCE appliances[MAX_APPLIANCES]; // Array of appliances (Oven/Frier)
    int num_appliances; // Number of appliances
    pthread_mutex_t appliances_mutex; // Mutex to access appliances' shared data

    COUNTER counters[MAX_COUNTERS]; // Array of counters
    int num_counters; // Number of counters
    pthread_mutex_t counters_mutex; // Mutex to access counters' shared data

    CUSTOMER customers[MAX_CUSTOMERS]; // Array of customers
    int num_customers; // Number of customers
    pthread_mutex_t customers_mutex; // Mutex to access customers' shared data

    int score; // Total score
    pthread_mutex_t score_mutex; // Mutex to acess score

    int client_fd; // Client socket
} THREAD_ARG_STRUCT;

#endif