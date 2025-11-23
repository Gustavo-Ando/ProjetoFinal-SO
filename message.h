#ifndef MESSAGE_H
#define MESSAGE_H

#define MESSAGE_SIZE 128

#include "map.h"

enum Message_type {
    // CLIENT MESSAGES
    INPUT = 'i', // Input
    CONNECTION = 'c', // Connects or leaves server

    // SERVER MESSAGES
    PLAYERS = 'P', // Player join or leave server
    SYSTEM = 'S', // Player index
    MOVEMENT = 'M', // Player position
    ITEM = 'I', // Player get or lose item
    APPLIANCE = 'O', // APPLIANCE start, ready, burns or empties
    TABLE = 'T', // Item left, taken from the table
    CUSTOMER = 'C', // Customer arrive, receive order, or leaves
};

enum Cook_status {
    EMPTY = 'E', // Empty oven (player can start oven)
    COOKING = 'C', // Cooking food (cannot get item)
    READY = 'R', // Food ready (player can get item)
    BURNED = 'B', // Food burned (player can get burneed item)
};

void msgC_input(char *message, char input);
void msgC_connection(char *message, int status);

void msgS_players(char *message, int player_index, int status);
void msgS_system(char *message, int player_index);
void msgS_movement(char *message, int player_index, int x, int y);
void msgS_item(char *message, int player_index, enum Item_type item_type);
void msgS_appliance(char *message, int app_index, int status, int time_left);
void msgS_table(char *message, int table_index, enum Item_type item_type);
void msgS_customer_arrival(char *message, int client_index, enum Item_type *order, int order_size);

enum Message_type msg_get_type(char *message);
int msg_get_size(char *message);


char msgC_input_get_input(char *message);

int msgC_connection_get_status(char *message);


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

int msgS_table_get_table_index(char *message);
enum Item_type msgS_table_get_item_type(char *message);

int msgS_customer_arrival_get_client_index(char *message);
int msgS_customer_arrival_get_order_size(char *message);
int *msgS_customer_arrival_get_order(char *message);
#endif