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
    
    struct sockaddr_in address; // Socket incoming address
} THREAD_ARG_STRUCT;

// Struct with the shared struct + the client index
typedef struct indexed_thread_arg_struct {
    THREAD_ARG_STRUCT *thread_arg; // Shared data
    int socket_index; // Index of socket or master socket
} INDEXED_THREAD_ARG_STRUCT;

#endif