#ifndef UTILITY_H
#define UTILITY_H

#define MAX_PLAYERS 4

// Function to treat errors
void fail(char *str);

// Function to send messages
void send_message(int socket, char *message);

// Function to lock multiple mutexes
void mutex_lock_both(pthread_mutex_t *mutex1, pthread_mutex_t *mutex2);

// Function to unlock multiple mutexes
void mutex_unlock_both(pthread_mutex_t *mutex1, pthread_mutex_t *mutex2);
#endif