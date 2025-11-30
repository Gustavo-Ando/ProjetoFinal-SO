#ifndef UTILITY_H
#define UTILITY_H

#define MAX_PLAYERS 4

#include <pthread.h>

// Enum of item types and their rendered characters
enum Item_type {
    NONE,
    BREAD,
    SALAD,
    JUICE,

    HAMBURGER,
    HAMBURGER_BURNED,
    HAMBURGER_READY,

    FRIES,
    FRIES_READY,
    FRIES_BURNED,

    BURGER_BREAD,
    SALAD_BREAD,
    SALAD_BURGER,
    FULL_BURGER,
};

enum Cook_status {
    EMPTY = 'E',   // Empty oven (player can start oven)
    COOKING = 'C', // Cooking food (cannot get item)
    READY = 'R',   // Food ready (player can get item)
    BURNED = 'B',  // Food burned (player can get burneed item)
};

enum Appliance_type {
    APP_OVEN = 1,
    APP_FRYER = 2,
};

// Function to treat errors
void fail(char *str);

// Function to send messages
void send_message(int socket, char *message);

// Function to lock multiple mutexes
void mutex_lock_both(pthread_mutex_t *mutex1, pthread_mutex_t *mutex2);

// Function to unlock multiple mutexes
void mutex_unlock_both(pthread_mutex_t *mutex1, pthread_mutex_t *mutex2);

// Function to combine itens
enum Item_type try_combine(enum Item_type a, enum Item_type b);

// Function to get seconds between times
int seconds_between(struct timespec start_time, struct timespec end_time);
#endif
