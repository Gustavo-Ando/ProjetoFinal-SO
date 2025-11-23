#include <pthread.h>
#include <stdio.h>

#include "message.h"
#include "client_process_message.h"

/*
    Function to process item message from server
    Responsible for updating the player's item
    Params:
        - char *message: message received from server
        - THREAD_ARG_STRUCT *thread_arg: struct containing shared data
    Return:
        -
*/
void process_message_item(char *message, THREAD_ARG_STRUCT *thread_arg)
{
    // Access player's CR
    pthread_mutex_lock(&thread_arg->players_mutex);
    // Get player index and item type
    int index = msgS_item_get_player_index(message);
    enum Item_type item = msgS_item_get_item_type(message);
    thread_arg->players[index].item = item; // Update item
    pthread_mutex_unlock(&thread_arg->players_mutex);

    // Access debug CR
    pthread_mutex_lock(&thread_arg->debug_mutex);
    sprintf(thread_arg->debug[thread_arg->current_debug_line], "ITEM %d:%c", index, item); // Add Debug message
    thread_arg->current_debug_line = (thread_arg->current_debug_line + 1) % 10;            // Update current line
    pthread_mutex_unlock(&thread_arg->debug_mutex);
}

/*
    Function to process movement message from server
    Responsible for updating the player's position (and last_position, where the item will be rendered)
    Params:
        - char *message: message received from server
        - THREAD_ARG_STRUCT *thread_arg: struct containing shared data
    Return:
        -
*/
void process_message_movement(char *message, THREAD_ARG_STRUCT *thread_arg)
{
    // Access player's CR and update the given player's item
    pthread_mutex_lock(&thread_arg->players_mutex);
    // Get player index and position
    int index = msgS_movement_get_player_index(message), x = msgS_movement_get_x(message), y = msgS_movement_get_y(message);
    // Check if valid player and position is changed
    if ((index >= 0 && index < MAX_PLAYERS) && (x != thread_arg->players[index].x || y != thread_arg->players[index].y))
    {
        // Update last position and position
        thread_arg->players[index].last_x = thread_arg->players[index].x;
        thread_arg->players[index].last_y = thread_arg->players[index].y;
        thread_arg->players[index].x = x;
        thread_arg->players[index].y = y;
        // If last position is invalid, set to current position
        if (thread_arg->players[index].last_x == -1 || thread_arg->players[index].last_y == -1)
        {
            thread_arg->players[index].last_x = x;
            thread_arg->players[index].last_y = y;
        }
    }
    pthread_mutex_unlock(&thread_arg->players_mutex);
}

/*
    Function to process player connection message from server
    Responsible for updating the player's connection
    Params:
        - char *message: message received from server
        - THREAD_ARG_STRUCT *thread_arg: struct containing shared data
    Return:
        -
*/
void process_message_players(char *message, THREAD_ARG_STRUCT *thread_arg)
{
    // Access Debug CR
    pthread_mutex_lock(&thread_arg->debug_mutex);
    sprintf(thread_arg->debug[thread_arg->current_debug_line], "CON %d:%d.", msgS_players_get_player_index(message), msgS_players_get_status(message)); // Add debug message
    thread_arg->current_debug_line = (thread_arg->current_debug_line + 1) % 10;                                                                         // Update current debug line
    pthread_mutex_unlock(&thread_arg->debug_mutex);

    // Access Player CR and update the given player's status, updating number of players in the game
    pthread_mutex_lock(&thread_arg->players_mutex);
    // Get player index and status
    int index = msgS_players_get_player_index(message);
    int status = msgS_players_get_status(message);
    thread_arg->players[index].is_active = status; // Update status
    // Update player count
    if (status == 1)
        thread_arg->num_players++;
    else
        thread_arg->num_players--;
    pthread_mutex_unlock(&thread_arg->players_mutex);
}

/*
    Function to process system message from server
    Responsible for receiving player index of this client
    Params:
        - char *message: message received from server
        - THREAD_ARG_STRUCT *thread_arg: struct containing shared data
    Return:
        -
*/
void process_message_system(char *message, THREAD_ARG_STRUCT *thread_arg)
{
    // Access debug CR
    pthread_mutex_lock(&thread_arg->debug_mutex);
    sprintf(thread_arg->debug[thread_arg->current_debug_line], "INDEX %d", msgS_system_get_player_index(message)); // Add debug message
    thread_arg->current_debug_line = (thread_arg->current_debug_line + 1) % 10;                                    // Update current debug line
    pthread_mutex_unlock(&thread_arg->debug_mutex);

    // Access player CR and for each player check if it's index corresponds to this client's index
    pthread_mutex_lock(&thread_arg->players_mutex);
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        thread_arg->players[i].is_me = (i == msgS_system_get_player_index(message));
    }
    pthread_mutex_unlock(&thread_arg->players_mutex);
}

/*
    Function to process appliance status message from server
    Responsible for updating the appliance state and content for rendering
    Params:
        - char *message: message received from server
        - THREAD_ARG_STRUCT *thread_arg: struct containing shared data
*/
void process_message_appliance(char *message, THREAD_ARG_STRUCT *thread_arg)
{

    // Recupera dados da mensagem usando as novas funções
    int index = msgS_appliance_get_index(message);
    int status = msgS_appliance_get_status(message);

    pthread_mutex_lock(&thread_arg->players_mutex);

    if (index >= 0 && index < num_appliances)
    {
        appliances[index].state = status;

        // Lógica para atualizar o Conteúdo visual baseado no Status e Tipo
        if (status == COOK_OFF)
        {
            appliances[index].content = NONE;
        }
        else
        {
            if (appliances[index].type == APP_OVEN)
            {
                if (status == COOK_COOKING)
                    appliances[index].content = HAMBURGER;
                else if (status == COOK_READY)
                    appliances[index].content = HAMBURGER_READY;
                else if (status == COOK_BURNT)
                    appliances[index].content = HAMBURGER_BURNED;
            }
            else if (appliances[index].type == APP_FRYER)
            {
                if (status == COOK_COOKING)
                    appliances[index].content = FRIES;
                else if (status == COOK_READY)
                    appliances[index].content = FRIES_READY;
                else if (status == COOK_BURNT)
                    appliances[index].content = FRIES_BURNED;
            }
        }
    }

    pthread_mutex_unlock(&thread_arg->players_mutex);
}

/*
    Function to process counter status message from server
    Responsible for updating the counter state and content for rendering
    Params:
        - char *message: message received from server
        - THREAD_ARG_STRUCT *thread_arg: struct containing shared data
*/
void process_message_counter(char *message, THREAD_ARG_STRUCT *thread_arg)
{

    // Recupera dados da mensagem usando as novas funções
    int index = msgS_counter_get_counter_index(message);
    int item = msgS_counter_get_item_type(message);

    pthread_mutex_lock(&thread_arg->players_mutex);

    if (index >= 0 && index < num_counters)
    {

        // Lógica para atualizar o Conteúdo visual baseado no Status e Tipo
        if (item == NONE)
        {
            counters[index].content = NONE;
        }
        else
        {
            counters[index].content = item;
        }
    }

    pthread_mutex_unlock(&thread_arg->players_mutex);
}