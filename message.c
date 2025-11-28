#include <stdlib.h>
#include "message.h"

// CLIENT MESSAGES

/*
    Function to create message containing input from the client
    message[0] is the message code, message[1] contains the input
    Params:
        - char *message: buffer containing the message
        - char input: player's input
    Return:
        -
*/
void msgC_input(char *message, char input) {
    message[0] = MSG_INPUT;
    message[1] = input;
    message[2] = '\0';
}

/*
    Function to get the input from an input-type message
    message[1] contains the input
    Params:
        - char *message: buffer containing the message
    Return:
        - char input
*/
char msgC_input_get_input(char *message) {
    return message[1];
}

/*
    Function to create message containing the client's connection status
    message[0] is the message code, message[1] contains the status
    Params:
        - char *message: buffer containing the message
        - int status: player's status
    Return:
        -
*/
void msgC_connection(char *message, int status) {
    message[0] = MSG_CONNECTION;
    message[1] = '0' + status;
    message[2] = '\0';
}

/*
    Function to get the player's status from a connection-type message
    message[1] contains the status
    Params:
        - char *message: buffer containing the message
    Return:
        - int status
*/
int msgC_connection_get_status(char *message) {
    return message[1] - '0';
}

// SERVER MESSAGES

/*
    Function to create message containing the given player status
    message[0] is the message code, message[1] contains the player index, message[3] contains their status
    Params:
        - char *message: buffer containing the message
        - int player_index: player's index
        - int status: player's status
    Return:
        -
*/
void msgS_players(char *message, int player_index, int status) {
    message[0] = MSG_PLAYERS;
    message[1] = '0' + player_index;
    message[2] = '0' + status;
    message[3] = '\0';
}

/*
    Function to get the player's index from a player-type message
    message[1] contains the player's index
    Params:
        - char *message: buffer containing the message
    Return:
        - int player index
*/
int msgS_players_get_player_index(char *message) {
    return message[1] - '0';
}

/*
    Function to get the player's status from a player-type message
    message[2] contains the player's status
    Params:
        - char *message: buffer containing the message
    Return:
        - int player status
*/
int msgS_players_get_status(char *message) {
    return message[2] - '0';
}

/*
    Function to create message containing the client's index
    message[0] is the message code, message[1] contains the client index
    Params:
        - char *message: buffer containing the message
        - int player_index: client's index
    Return:
        -
*/
void msgS_system(char *message, int player_index) {
    message[0] = MSG_SYSTEM;
    message[1] = '0' + player_index;
    message[2] = '\0';
}

/*
    Function to get the client's index from a system-type message
    message[1] contains the client's index
    Params:
        - char *message: buffer containing the message
    Return:
        - int client index
*/
int msgS_system_get_player_index(char *message) {
    return message[1] - '0';
}

/*
    Function to create message containing the given player's position
    message[0] is the message code, message[1] contains the player index, message[2] contains the x coord., message[3] contains the y coord.
    Params:
        - char *message: buffer containing the message
        - int player_index: player's index
        - int x: player's x coord.
        - int y: player's y coord.
    Return:
        -
*/
void msgS_movement(char *message, int player_index, int x, int y) {
    message[0] = MSG_MOVEMENT;
    message[1] = '0' + player_index;
    message[2] = '0' + x;
    message[3] = '0' + y;
    message[4] = '\0';
}

/*
    Function to get the player's index from a movement-type message
    message[1] contains the player's index
    Params:
        - char *message: buffer containing the message
    Return:
        - int player index
*/
int msgS_movement_get_player_index(char *message) {
    return message[1] - '0';
}

/*
    Function to get the player's x coord. from a movement-type message
    message[2] contains the player's x coord.
    Params:
        - char *message: buffer containing the message
    Return:
        - int player's x coord.
*/
int msgS_movement_get_x(char *message) {
    return message[2] - '0';
}

/*
    Function to get the player's x coord. from a movement-type message
    message[3] contains the player's y coord.
    Params:
        - char *message: buffer containing the message
    Return:
        - int player's y coord.
*/
int msgS_movement_get_y(char *message) {
    return message[3] - '0';
}

/*
    Function to create message containing the given player's item
    message[0] is the message code, message[1] contains the player index, message[2] contains the item type (enum)
    Params:
        - char *message: buffer containing the message
        - int player_index: player's index
        - enum Item_type item_type: enum with the item type
    Return:
        -
*/
void msgS_item(char *message, int player_index, enum Item_type item_type) {
    message[0] = MSG_ITEM;
    message[1] = '0' + player_index;
    message[2] = '0' + item_type;
    message[3] = '\0';
}

/*
    Function to get the player's index from an item-type message
    message[1] contains the player's index
    Params:
        - char *message: buffer containing the message
    Return:
        - int player index
*/
int msgS_item_get_player_index(char *message) {
    return message[1] - '0';
}

/*
    Function to get the player's item type from an item-type message
    message[2] contains the player's item type.
    Params:
        - char *message: buffer containing the message
    Return:
        - enum Item_type player's item
*/
enum Item_type msgS_item_get_item_type(char *message) {
    return message[2] - '0';
}

/*
    Function to create message containing the given appliance's status
    message[0] is the message code, message[1] contains the appliance index, message[2] contains the status
*/
void msgS_appliance(char *message, int app_index, int status, int time_left) {
    message[0] = MSG_APPLIANCE;
    message[1] = '0' + app_index;
    message[2] = status;
    message[3] = '0' + time_left;
    message[4] = '\0';
}

/*
    Function to get the appliance's index from an appliance-type message
*/
int msgS_appliance_get_index(char *message) {
    return message[1] - '0';
}

/*
    Function to get the appliance's status from an appliance-type message
*/
int msgS_appliance_get_status(char *message) {
    return message[2];
}

/*
    Function to create message containing the given counter's item
    message[0] is the message code, message[1] contains the counter index, message[2] contains the item type (enum)
    Params:
        - char *message: buffer containing the message
        - int counter_index: counter's index
        - enum Item_type item_tye: enum with the item type
    Return:
        -
*/
void msgS_counter(char *message, int counter_index, enum Item_type item_type) {
    message[0] = MSG_COUNTER;
    message[1] = '0' + counter_index;
    message[2] = item_type + '0';
    message[3] = '\0';
}

/*
    Function to get the counter's index from a counter-type message
    message[1] contains the counter's index
    Params:
        - char *message: buffer containing the message
    Return:
        - int counter index
*/
int msgS_counter_get_counter_index(char *message) {
    return message[1] - '0';
}

/*
    Function to get the counter's item from a counter-type message
    message[2] contains the counter's item
    Params:
        - char *message: buffer containing the message
    Return:
        - enum Item_type item_type
*/
enum Item_type msgS_counter_get_item_type(char *message) {
    return message[2] - '0';
}

/*
    Function to get the appliance's time from an appliance-type message
*/
int msgS_appliance_get_time_left(char *message) {
    return message[3] - '0';
}

/*
    Function to create message containing the given table's item
    message[0] is the message code, message[1] contains the table index, message[2] contains the item type (enum)
    Params:
        - char *message: buffer containing the message
        - int table_index: table's index
        - enum Item_type item_tye: enum with the item type
    Return:
        -
*/
void msgS_table(char *message, int table_index, enum Item_type item_type) {
    message[0] = MSG_TABLE;
    message[1] = '0' + table_index;
    message[2] = item_type;
    message[3] = '\0';
}

/*
    Function to get the table's index from a table-type message
    message[1] contains the table's index
    Params:
        - char *message: buffer containing the message
    Return:
        - int table index
*/
int msgS_table_get_table_index(char *message) {
    return message[1] - '0';
}

/*
    Function to get the table's item from a table-type message
    message[2] contains the table's item
    Params:
        - char *message: buffer containing the message
    Return:
        - enum Item_type item_type
*/
enum Item_type msgS_table_get_item_type(char *message) {
    return message[2];
}

/*
    Function to create message containing the given customer's order
    message[0] is the message code, message[1] contains the customer index, message[2] contains the order size, message [3+order size] contains the order
    Params:
        - char *message: buffer containing the message
        - int client_index: customer's index
        - enum Item_type *order: array containing the itens ordered
        - int order_size: number of itens in the order
    Return:
        -
*/
void msgS_customer_arrival(char *message, int client_index, enum Item_type *order, int order_size) {
    message[0] = MSG_CUSTOMER;
    message[1] = '0' + client_index;
    message[2] = '0' + order_size;
    for (int i = 0; i < order_size; i++) {
        message[i + 3] = order[i];
    }
    message[order_size + 3] = '\0';
}

/*
    Function to get the customer's index from a customer-type message
    message[1] contains the customer's index
    Params:
        - char *message: buffer containing the message
    Return:
        - int customer index
*/
int msgS_customer_arrival_get_client_index(char *message) {
    return message[1] - '0';
}

/*
    Function to get the customer's order size from a customer-type message
    message[2] contains the customer's order size
    Params:
        - char *message: buffer containing the message
    Return:
        - int order size
*/
int msgS_customer_arrival_get_order_size(char *message) {
    return message[2] - '0';
}

/*
    Function to get the customer's order from a customer-type message
    message[3+order_size] contains the customer's order
    Params:
        - char *message: buffer containing the message
    Return:
        - int *order
*/
int *msgS_customer_arrival_get_order(char *message) {
    int order_size = msgS_customer_arrival_get_order_size(message);
    int *order = (int *)malloc(sizeof(enum Item_type) * order_size);
    for (int i = 0; i < order_size; i++) {
        order[i] = message[i + 3];
    }
    return order;
}

/*
    Function to create message containing the given customer's full status
    message[0] is the message code, message[1] contains the customer index, message[2] contains the order size, message [3..3+order_size-1] contains the order,
    message[3+order_size] contains the x coord., message[4+order_size] contains the y coord., message[5+order_size] contains the state, message[6+order_size] contains time left
    Params:
        - char *msg: buffer containing the message
        - int customer_id: customer's index
        - enum Item_type *order: array containing the itens ordered
        - int order_size: number of itens in the order
        - int x: customer's x coord.
        - int y: customer's y coord.
        - int state: customer's state
        - int time_left: time left for the customer to leave
    Return:
        -
*/

void msgS_customer(
    char *msg,
    int customer_id,
    enum Item_type *order,
    int order_size,
    int x,
    int y,
    int state,
    int time_left
) {
    msg[0] = MSG_CUSTOMER;
    msg[1] = (char) ('0' + customer_id);
    msg[2] = (char) ('0' + order_size);

    for (int i = 0; i < order_size && i < MAX_ORDER; i++) {
        msg[3 + i] = (char) ((unsigned char) order[i]); // escreva o valor diretamente como byte
    }

    msg[3 + order_size] = (char) ((unsigned char) x);
    msg[4 + order_size] = (char) ((unsigned char) y);
    msg[5 + order_size] = (char) ((unsigned char) state);
    msg[6 + order_size] = (char) ((unsigned char) time_left);
    msg[7 + order_size] = '\0';
}



/*
    Function to get the given message's type
    message[0] is the message type
    Params:
        - char *message: buffer containing the message
    Return:
        - enum Message_type message type
*/
enum Message_type msg_get_type(char *message) {
    return message[0];
}

/*
    Function to get the given message's size
    message[0] is the message type
    Params:
        - char *message: buffer containing the message
    Return:
        - int message size given it's type
*/
int msg_get_size(char *message) {
    switch (msg_get_type(message)) {
        case MSG_INPUT: return 2;
        case MSG_CONNECTION: return 2;
        case MSG_PLAYERS: return 3;
        case MSG_SYSTEM: return 2;
        case MSG_MOVEMENT: return 4;
        case MSG_ITEM: return 3;
        case MSG_APPLIANCE: return 4;
        case MSG_COUNTER: return 3;
        case MSG_TABLE: return 3;
        case MSG_CUSTOMER: return 7 + msgS_customer_arrival_get_order_size(message);
        default: return 0;
    }
}