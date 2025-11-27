#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include "map.h"

// Struct with data for a client
typedef struct _client {
    int socket; // Socket index (0 if no connection)
    int x, y; // Position of client's player
    enum Item_type item; // Item of client's player
} CLIENT;

// Struct with shared information
typedef struct thread_arg_struct {
    CLIENT clients[MAX_PLAYERS]; // List of clients
    int connected; // Number of connected clients
    pthread_mutex_t clients_mutex; // Mutex to access client list
    
    APPLIANCE appliances[MAX_APPLIANCES]; // Array of appliances (Oven/Frier)
    int num_appliances; // Number of appliances
    pthread_mutex_t appliances_mutex; // Mutex to access appliances' shared data

    COUNTER counters[MAX_COUNTERS]; // Array of counters
    int num_counters; // Number of counters
    pthread_mutex_t counters_mutex; // Mutex to access counters' shared data

    CUSTOMER customers[MAX_CUSTOMERS]; // Array of customers
    int num_customers; // Number of customers
    pthread_mutex_t customers_mutex; // Mutex to access customers' shared data

    struct sockaddr_in address; // Socket incoming address
} THREAD_ARG_STRUCT;

// Struct with the shared struct + the client index
typedef struct indexed_thread_arg_struct {
    THREAD_ARG_STRUCT *thread_arg; // Shared data
    int socket_index; // Index of socket or master socket
} INDEXED_THREAD_ARG_STRUCT;

#endif