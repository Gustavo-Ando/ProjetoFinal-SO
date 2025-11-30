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

/*
    Function to combine itens
    Params:
        - enum Item_type a: first item
        - enum Item_type b: second item
    Return:
        - enum Item_type: combined item (NONE if no valid combination)
*/
enum Item_type try_combine(enum Item_type a, enum Item_type b) {
    // Bread and burguer
    if ((a == BREAD && b == HAMBURGER_READY) || (b == BREAD && a == HAMBURGER_READY)) return BURGER_BREAD;

    // Bread and salad
    if ((a == BREAD && b == SALAD) || (b == BREAD && a == SALAD)) return SALAD_BREAD;

    // Salad and burguer
    if ((a == SALAD && b == HAMBURGER_READY) || (b == SALAD && a == HAMBURGER_READY)) return SALAD_BURGER;

    // Full Burguer
    if ((a == BURGER_BREAD && b == SALAD) || (a == SALAD && b == BURGER_BREAD) ||
        (a == SALAD_BREAD && b == HAMBURGER_READY) || (a == HAMBURGER_READY && b == SALAD_BREAD) ||
        (a == SALAD_BURGER && b == BREAD) || (a == BREAD && b == SALAD_BURGER))
        return FULL_BURGER;

    return NONE; // No combination
}

/*
    Function to get seconds between two times
    Params:
		- struct timespec start_time: start time
		- struct timespec end_time: end time
        - enum Item_type b: second item
    Return:
		- int elapsed_time: seconds between times
*/
int seconds_between(struct timespec start_time, struct timespec end_time){
    double elapsed_time = (double)(end_time.tv_sec - start_time.tv_sec) + (double)(end_time.tv_nsec - start_time.tv_nsec) / 1e9;
	return (int) elapsed_time;
}
