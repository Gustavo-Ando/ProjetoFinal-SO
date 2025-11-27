#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>

#include "utility.h"

/*
    Function to send an error message and terminate execution
    Params:
        - char *str: string to print
    Return:
        - 
*/
void fail(char *str) {
    perror(str);
    exit(EXIT_FAILURE);
}


/*
    Function to send a message and check if it was successful. If not, send an error message and terminate execution
    Params:
        - int socket: socket fd
        - char *message: message to send
    Return:
        - 
*/
void send_message(int socket, char *message){
    if (send(socket, message, strlen(message), 0) != strlen(message)) fail("Send error");
}

/*
    Function to aquire two mutexes, prevents deadlocks
    Params:
        - pthread_mutex_t *mutex1: Reference to first mutex
        - pthread_mutex_t *mutex2: Reference to second mutex
    Return:
        -
*/
void mutex_lock_both(pthread_mutex_t *mutex1, pthread_mutex_t *mutex2){
    while(1){
        pthread_mutex_lock(mutex1);
        if(pthread_mutex_trylock(mutex2) != 0) pthread_mutex_unlock(mutex1);
        else break;
    }
}

/*
    Function to unlock two mutexes previously locked to prevent forgetting one of them
    Params:
        - pthread_mutex_t *mutex1: Reference to first mutex
        - pthread_mutex_t *mutex2: Reference to second mutex
    Return:
        -
*/
void mutex_unlock_both(pthread_mutex_t *mutex1, pthread_mutex_t *mutex2){
    pthread_mutex_unlock(mutex1);
    pthread_mutex_unlock(mutex2);
}