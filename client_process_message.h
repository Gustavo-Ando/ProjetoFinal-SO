#ifndef CLIENT_PROCESS_MESSAGE_H
#define CLIENT_PROCESS_MESSAGE_H

#include "client.h"

void process_message_item(char *message, THREAD_ARG_STRUCT *thread_arg);
void process_message_movement(char *message, THREAD_ARG_STRUCT *thread_arg);
void process_message_players(char *message, THREAD_ARG_STRUCT *thread_arg);
void process_message_system(char *message, THREAD_ARG_STRUCT *thread_arg);
void process_message_appliance(char *message, THREAD_ARG_STRUCT *thread_arg);

#endif