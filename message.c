#include <stdlib.h>
#include "message.h"
// Message type
/*
Client
 - Input
 - Disconnect

Server
 - Player join, disconnect
 - Player position
 - Player get, lose item
 - Oven start, ready, burn, empty
 - Bancada put, remove
 - Client arrive, give, leave
*/

/*
    Hamburgueria:
        * Hamburger: -
        * Hamburger queimado: ~
        * Pão: =
        * Hamburguer com pão: ≡ E
        * Salada: @
        * Refri: Ú 
        * Batata frita: W
        * Batata frita queimada: M

*/

void msgC_input(char *message, char input){
    message[0] = INPUT;
    message[1] = input;
    message[2] = '\0';
}

char msgC_input_get_input(char *message){
    return message[1];
}

void msgC_connection(char *message, int status){
    message[0] = CONNECTION;
    message[1] = '0' + status;
    message[2] = '\0';
}

int msgC_connection_get_status(char *message){
    return message[1] - '0';
}

void msgS_players(char *message, int player_index, int status){
    message[0] = PLAYERS;
    message[1] = '0' + player_index;
    message[2] = '0' + status;
    message[3] = '\0';
}

int msgS_players_get_player_index(char *message){
    return message[1] - '0';
}

int msgS_players_get_status(char *message){
    return message[2] - '0';
}

void msgS_system(char *message, int player_index){
    message[0] = SYSTEM;
    message[1] = '0' + player_index;
    message[2] = '\0';
}

int msgS_system_get_player_index(char *message){
    return message[1] - '0';
}

void msgS_movement(char *message, int player_index, int x, int y){
    message[0] = MOVEMENT;
    message[1] = '0' + player_index;
    message[2] = '0' + x;
    message[3] = '0' + y;
    message[4] = '\0';
}

int msgS_movement_get_player_index(char *message){
    return message[1] - '0';
}

int msgS_movement_get_x(char *message){
    return message[2] - '0';
}

int msgS_movement_get_y(char *message){
    return message[3] - '0';
}

void msgS_item(char *message, int player_index, enum Item_type item_type){
    message[0] = ITEM;
    message[1] = '0' + player_index;
    message[2] = item_type;
    message[3] = '\0';
}

int msgS_item_get_player_index(char *message){
    return message[1] - '0';
}

enum Item_type msgS_item_get_item_type(char *message){
    return message[2];
}

void msgS_oven(char *message, int oven_index, enum Oven_status oven_status){
    message[0] = OVEN;
    message[1] = '0' + oven_index;
    message[2] = oven_status;
    message[3] = '\0';
}

int msgS_oven_get_oven_index(char *message){
    return message[1] - '0';
}

enum Oven_status msgS_oven_get_oven_status(char *message){
    return message[2];
}

void msgS_table(char *message, int table_index, enum Item_type item_type) {
    message[0] = TABLE;
    message[1] = '0' + table_index;
    message[2] = item_type;
    message[3] = '\0';
}

int msgS_table_get_table_index(char *message){
    return message[1] - '0';
}

enum Item_type msgS_table_get_item_type(char *message){
    return message[2];
}

void msgS_customer_arrival(char *message, int client_index, enum Item_type *order, int order_size){
    message[0] =  CUSTOMER;
    message[1] = '0' + client_index;
    message[2] = '0' + order_size;
    for(int i = 0; i < order_size; i++){
        message[i+3] = order[i];
    }
    message[order_size+3] = '\0';
}

int msgS_customer_arrival_get_client_index(char *message){
    return message[1] - '0';
}

int msgS_customer_arrival_get_order_size(char *message){
    return message[2] - '0';
}

int *msgS_customer_arrival_get_order(char *message){
    int order_size = msgS_customer_arrival_get_order_size(message);
    int* order = (int*) malloc(sizeof(enum Item_type)*order_size);
    for(int i = 0; i < order_size; i++){
        order[i] = message[i+3];
    }
    return order;
}

enum Message_type msg_get_type(char *message){
    return message[0];
}

int msg_get_size(char *message){
    switch(msg_get_type(message)){
        case INPUT: return 2;
        case CONNECTION: return 2;
        case PLAYERS: return 3;
        case SYSTEM: return 2;
        case MOVEMENT: return 4;
        case ITEM: return 3;
        case OVEN: return 3;
        case TABLE: return 3;
        case CUSTOMER: return 3 + msgS_customer_arrival_get_order_size(message);
        default: return 0;
    }
}
