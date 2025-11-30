#ifndef MESSAGE_H
#define MESSAGE_H

#define MESSAGE_SIZE 64

#include "map.h"

enum Message_type {
    // CLIENT MESSAGES
    MSG_INPUT = 'i',      // Input

    // SERVER MESSAGES
    MSG_CONNECTION = 'c', // Connects or leaves server
    MSG_PLAYERS = 'P',   // Player join or leave server
    MSG_SYSTEM = 'S',    // Player index
    MSG_MOVEMENT = 'M',  // Player position
    MSG_ITEM = 'I',      // Player get or lose item
    MSG_APPLIANCE = 'O', // APPLIANCE start, ready, burns or empties
    MSG_COUNTER = 'K',   // COUNTER with or without item
    MSG_TABLE = 'T',     // Item left, taken from the table
    MSG_CUSTOMER = 'C',  // Customer arrive, receive order, or leaves
    MSG_SCORE = 's'      // Total score
};

void msgC_input(char *message, char input);
void msgS_connection(char *message, int status);

void msgS_players(char *message, int player_index, int status);
void msgS_system(char *message, int player_index);
void msgS_movement(char *message, int player_index, int x, int y);
void msgS_item(char *message, int player_index, enum Item_type item_type);
void msgS_appliance(char *message, int app_index, int status, int time_left);
void msgS_counter(char *message, int counter_index, enum Item_type item_type);
void msgS_table(char *message, int table_index, enum Item_type item_type);
void msgS_customer(char *msg, int customer_id, enum Item_type *order, int order_size, int state, int time_left);
void msgS_score(char *message, int score);

enum Message_type msg_get_type(char *message);
int msg_get_size(char *message);

char msgC_input_get_input(char *message);

int msgS_connection_get_status(char *message);

int msgS_players_get_player_index(char *message);
int msgS_players_get_status(char *message);

int msgS_system_get_player_index(char *message);

int msgS_movement_get_player_index(char *message);
int msgS_movement_get_x(char *message);
int msgS_movement_get_y(char *message);

int msgS_item_get_player_index(char *message);
enum Item_type msgS_item_get_item_type(char *message);

int msgS_appliance_get_index(char *message);
int msgS_appliance_get_status(char *message);
int msgS_appliance_get_time_left(char *message);

int msgS_counter_get_counter_index(char *message);
enum Item_type msgS_counter_get_item_type(char *message);

int msgS_table_get_table_index(char *message);
enum Item_type msgS_table_get_item_type(char *message);

int msgS_customer_get_client_index(char *message);
int msgS_customer_get_order_size(char *message);
int *msgS_customer_get_order(char *message);
int msgS_customer_get_state(char *message);
int msgS_customer_get_time_left(char *message);

int msgS_score_get_value(char *message);

#endif
