#ifndef SERVER_GAME_LOGIC_H
#define SERVER_GAME_LOGIC_H

#include "server.h"

int app_interaction(THREAD_ARG_STRUCT *thread_arg, int px, int py, int index);
int counter_interaction(THREAD_ARG_STRUCT *thread_arg, int px, int py, int index);
int customer_interaction(THREAD_ARG_STRUCT *thread_arg, int px, int py, int index);

void treat_client_input(THREAD_ARG_STRUCT *thread_arg, char input, int index);

void create_customer(THREAD_ARG_STRUCT *thread_arg);
void tick_customers(THREAD_ARG_STRUCT *thread_arg);
void tick_appliances(THREAD_ARG_STRUCT *thread_arg);
#endif
