#ifndef SERVER_SEND_MESSAGE_H
#define SERVER_SEND_MESSAGE_H

#include "server.h"

void send_game_state(THREAD_ARG_STRUCT *thread_arg, int index);
void broadcast_player_connection(THREAD_ARG_STRUCT *thread_arg, int index);
void broadcast_player_position(THREAD_ARG_STRUCT *thread_arg, int index);
void broadcast_player_item(THREAD_ARG_STRUCT *thread_arg, int index);
void broadcast_appliance_status(THREAD_ARG_STRUCT *thread_arg, int index);
void broadcast_counter_update(THREAD_ARG_STRUCT *thread_arg, int counter_index);

#endif