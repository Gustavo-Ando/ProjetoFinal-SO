#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>

#include "server_send_message.h"
#include "message.h"
#include "map.h"
#include "utility.h"
#include "server.h"

#define PORT 8080

#define TIME_TO_COOK 5
#define TIME_TO_BURN 14

/*
    Thread to read data in master_socket
    Responsible for dealing with incoming connections, adding it to client list
    Params:
        - void *args (INDEXED_THREAD_ARG_STRUCT): struct containing shared data and master_socket index
    Return:
        -
*/
static void *read_master_socket(void *args)
{
    INDEXED_THREAD_ARG_STRUCT indexed_struct = *(INDEXED_THREAD_ARG_STRUCT *)args; // Cast
    THREAD_ARG_STRUCT *thread_arg = indexed_struct.thread_arg;
    // In this case, socket_index is actually the fd to the master_socket
    int master_socket = indexed_struct.socket_index;
    int addr_len = sizeof(thread_arg->address);

    while (1)
    {
        // Accepts the connection
        int new_socket = accept(master_socket, (struct sockaddr *)&thread_arg->address, (socklen_t *)&addr_len);
        if (new_socket < 0)
            fail("Accept failed");
        printf("New connection!\n");
        printf(" - Socket FD: %d\n", new_socket);
        printf(" - IP: %s:%d\n", inet_ntoa(thread_arg->address.sin_addr), ntohs(thread_arg->address.sin_port));

        // Access client CR to read number of connected clients and check if a connection is possible
        int is_full = 0;
        pthread_mutex_lock(&thread_arg->clients_mutex);
        if (thread_arg->connected == MAX_PLAYERS)
            is_full = 1;
        pthread_mutex_unlock(&thread_arg->clients_mutex);

        // If reached the limit, reject the connection and continue
        if (is_full)
        {
            printf(" - Rejected: Server full.\n\n");
            char *message = "Connection rejected";
            send(new_socket, message, strlen(message), 0);
            continue;
        }

        // Access client CR to increase number of connected clients and add client to the list
        pthread_mutex_lock(&thread_arg->clients_mutex);
        int new_index;
        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            // If the index is already in use, tries the next one
            if (thread_arg->clients[i].socket != 0)
                continue;
            thread_arg->clients[i].socket = new_socket;
            // Defines initial position and item
            thread_arg->clients[i].x = 6;
            thread_arg->clients[i].y = 5;
            thread_arg->clients[i].item = NONE;
            new_index = i;
            // Increases the number of connected players
            thread_arg->connected++;
            printf(" - Adding socket as %d\n\n", i);
            break;
        }
        pthread_mutex_unlock(&thread_arg->clients_mutex);

        // Send all data to new client
        send_game_state(thread_arg, new_index);
        // Update other clients of the new player
        broadcast_player_connection(thread_arg, new_index);
        broadcast_player_position(thread_arg, new_index);
        broadcast_player_item(thread_arg, new_index);
    }

    return NULL;
}

/*
    Function to treat input from a client
    Responsible for checking if a move or action is valid, processing it's effects and sending result to all clients
    Params:
        - THREAD_ARG_STRUCT *args: struct containing shared data
        - char input: character inputed from client
        - int index: index of client responsible for input
    Return:
        -
*/
static void treat_client_input(THREAD_ARG_STRUCT *thread_arg, char input, int index)
{
    pthread_mutex_lock(&thread_arg->clients_mutex);

    int try_moved = 0, try_action = 0;
    int app_updated_index = -1;
    int counter_updated_index = -1;

    switch (input)
    {
    // Movimentacao (W, A, S, D)
    case 'w':
        try_moved = 1;
        if (!(game_map[thread_arg->clients[index].y - 1][thread_arg->clients[index].x]))
            thread_arg->clients[index].y--;
        break;
    case 'a':
        try_moved = 1;
        if (!(game_map[thread_arg->clients[index].y][thread_arg->clients[index].x - 2]))
            thread_arg->clients[index].x -= 2;
        break;
    case 's':
        try_moved = 1;
        if (!(game_map[thread_arg->clients[index].y + 1][thread_arg->clients[index].x]))
            thread_arg->clients[index].y++;
        break;
    case 'd':
        try_moved = 1;
        if (!(game_map[thread_arg->clients[index].y][thread_arg->clients[index].x + 2]))
            thread_arg->clients[index].x += 2;
        break;

    // Interacao
    case ' ':
        try_action = 1;
        int px = thread_arg->clients[index].x;
        int py = thread_arg->clients[index].y;

        // Pegar itens do chão
        if (item_map[py][px] != NONE && thread_arg->clients[index].item == NONE)
        {
            thread_arg->clients[index].item = item_map[py][px];
        }
        else if (item_map[py][px] != NONE && thread_arg->clients[index].item != NONE)
        {
            enum Item_type p_item = thread_arg->clients[index].item;
            enum Item_type g_item = item_map[py][px];

            enum Item_type combined = try_combine(p_item, g_item);

            if (combined != NONE)
            {
                thread_arg->clients[index].item = combined;
            }
        }
        // Jogar no Lixo
        else if (trash_map[py][px])
        {
            thread_arg->clients[index].item = NONE;
        }
        // Interação com Objetos (Máquinas e Bancadas)
        else
        {
            int app_id = get_appliance_id_at(px, py);
            int c_id = get_counter_id_at(px, py);

            // Fogao/Fritadeira
            if (app_id != -1)
            {
                Appliance *app = &appliances[app_id];
                enum Item_type p_item = thread_arg->clients[index].item;

                // Se appliance tem item e jogador tem item → TENTAR COMBINAR
                if (p_item != NONE && app->content != NONE)
                {
                    enum Item_type combined = try_combine(p_item, app->content);

                    if (combined != NONE)
                    {
                        // Monta os itens combinados na mão do jogador
                        thread_arg->clients[index].item = combined;
                        app->state = COOK_OFF;
                        app->content = NONE;
                        app->time_left = 0;
                        app_updated_index = app_id;
                        break;
                    }
                }

                // Jogador coloca item CRU no appliance (regra original)
                if (app->state == COOK_OFF)
                {
                    int success = 0;

                    // Regras originais
                    if (app->type == APP_OVEN && p_item == HAMBURGER)
                    {
                        app->content = HAMBURGER;
                        success = 1;
                    }
                    else if (app->type == APP_FRYER && p_item == FRIES)
                    {
                        app->content = FRIES;
                        success = 1;
                    }

                    if (success)
                    {
                        thread_arg->clients[index].item = NONE;
                        app->state = COOK_COOKING;
                        app->start_time = time(NULL);
                        app->time_left = TIME_TO_COOK;
                        app_updated_index = app_id;
                        break;
                    }
                }

                // Jogador pega o item do appliance (somente se não está cozinhando)
                if ((app->state != COOK_COOKING) && p_item == NONE)
                {
                    thread_arg->clients[index].item = app->content;
                    app->state = COOK_OFF;
                    app->content = NONE;
                    app->time_left = 0;
                    app_updated_index = app_id;
                    break;
                }
            }

            // Bancada
            else if (c_id != -1)
            {
                Counter *c = &counters[c_id];
                enum Item_type p_item = thread_arg->clients[index].item;
                enum Item_type c_item = c->content;

                // Caso: Jogador segurando algo e bancada com algo → tentar combinar
                if (p_item != NONE && c_item != NONE)
                {
                    enum Item_type combined = try_combine(p_item, c_item);

                    if (combined != NONE)
                    {
                        // combinação fica na bancada
                        // (põe na bancada a combinação e limpa mão)
                        thread_arg->clients[index].item = NONE;
                        c->content = combined;
                        counter_updated_index = c_id;
                    }
                    // Caso contrário, nada acontece
                }

                // Jogador segurando item e bancada vazia → deposita
                else if (p_item != NONE && c_item == NONE)
                {
                    c->content = p_item;
                    thread_arg->clients[index].item = NONE;
                    counter_updated_index = c_id;
                }

                // Jogador sem item e bancada com item → pega
                else if (p_item == NONE && c_item != NONE)
                {
                    thread_arg->clients[index].item = c_item;
                    c->content = NONE;
                    counter_updated_index = c_id;
                }
            }
        }
        break;

    default:
        break;
    }

    printf("Received input from client %d: %c\n", index, input);
    printf(" - %d, %d\n", thread_arg->clients[index].x, thread_arg->clients[index].y);

    pthread_mutex_unlock(&thread_arg->clients_mutex);

    if (try_moved)
        broadcast_player_position(thread_arg, index);

    if (try_action)
        broadcast_player_item(thread_arg, index);

    if (app_updated_index != -1)
        broadcast_appliance_status(thread_arg, app_updated_index);

    if (counter_updated_index != -1)
        broadcast_counter_update(thread_arg, counter_updated_index);
}

/*
    Thread to read data from a client
    Responsible for dealing with disconnection (remove from client list), and receiving input
    Params:
        - void *args (INDEXED_THREAD_ARG_STRUCT): struct containing shared data and client index
    Return:
        -
*/
static void *read_client_socket(void *args)
{
    INDEXED_THREAD_ARG_STRUCT indexed_struct = *(INDEXED_THREAD_ARG_STRUCT *)args; // Cast
    THREAD_ARG_STRUCT *thread_arg = indexed_struct.thread_arg;
    int index = indexed_struct.socket_index;
    int addr_len = sizeof(thread_arg->address);
    // Repeat to check if socket exists
    while (1)
    {
        // Access client CR to read the socket
        pthread_mutex_lock(&thread_arg->clients_mutex);
        int sd = thread_arg->clients[index].socket;
        pthread_mutex_unlock(&thread_arg->clients_mutex);
        if (sd == 0)
            continue;
        // If exists, treat client until it disconnects
        while (1)
        {
            char buffer[MESSAGE_SIZE + 1];
            // Keeps track of the size of the message received
            int read_length = read(sd, buffer, MESSAGE_SIZE);
            buffer[read_length] = '\0'; // Null terminate message

            // If there is no message, the client disconnected
            if (read_length == 0)
            {
                // Disconnected
                getpeername(sd, (struct sockaddr *)&thread_arg->address, (socklen_t *)&addr_len);
                printf("Host disconnected.\n");
                printf(" - IP: %s:%d\n\n", inet_ntoa(thread_arg->address.sin_addr), ntohs(thread_arg->address.sin_port));

                // close the socket
                close(sd);

                // Access CR to disconnect the client
                pthread_mutex_lock(&thread_arg->clients_mutex);
                thread_arg->clients[index].socket = 0;
                thread_arg->connected--;
                pthread_mutex_unlock(&thread_arg->clients_mutex);
                // Update player's connection
                broadcast_player_connection(thread_arg, index);
                break;
            }
            else
            {
                // Index to read message
                int current_index = 0;
                // While is not the end of the message
                while (current_index < MESSAGE_SIZE && current_index < read_length && buffer[current_index] != '\0')
                {
                    // Treat it based on its type
                    switch (msg_get_type(buffer + current_index))
                    {
                    case INPUT:
                        treat_client_input(thread_arg, msgC_input_get_input(buffer + current_index), index);
                        break;
                    default:
                        buffer[current_index] = '\0';
                        break;
                    }
                    // Updates index given the message size to deal with concatenated messages
                    current_index += msg_get_size(buffer + current_index);
                }
            }
        }
    }

    return NULL;
}

/*
    Thread to handle game physics and timers (Ovens/Fryers)
*/
static void *game_loop_thread(void *arg)
{
    THREAD_ARG_STRUCT *thread_arg = (THREAD_ARG_STRUCT *)arg;

    while (1)
    {
        sleep(1); // Verifica a cada 1 segundo

        pthread_mutex_lock(&thread_arg->clients_mutex);

        time_t now = time(NULL);

        for (int i = 0; i < num_appliances; i++)
        {
            int changed = 0;
            int current_time_elapsed = (int)difftime(now, appliances[i].start_time);
            int new_time_left = 0;

            // Estado: COZINHANDO -> PRONTO
            if (appliances[i].state == COOK_COOKING)
            {
                new_time_left = TIME_TO_COOK - current_time_elapsed;
                if (new_time_left < 0)
                    new_time_left = 0;

                appliances[i].time_left = new_time_left;
                changed = 1;

                if (current_time_elapsed >= TIME_TO_COOK)
                {
                    appliances[i].state = COOK_READY;
                    appliances[i].content = (appliances[i].type == APP_OVEN) ? HAMBURGER_READY : FRIES_READY;
                    appliances[i].time_left = TIME_TO_BURN - current_time_elapsed;
                }
            }
            // Estado: PRONTO -> QUEIMADO
            else if (appliances[i].state == COOK_READY)
            {
                new_time_left = TIME_TO_BURN - current_time_elapsed;
                if (new_time_left < 0)
                    new_time_left = 0;

                appliances[i].time_left = new_time_left;
                changed = 1;

                if (current_time_elapsed >= TIME_TO_BURN)
                {
                    appliances[i].state = COOK_BURNT;
                    appliances[i].content = (appliances[i].type == APP_OVEN) ? HAMBURGER_BURNED : FRIES_BURNED;
                    appliances[i].time_left = 0;
                }
            }

            // Se mudou o estado, avisa todos os clientes
            if (changed)
            {
                pthread_mutex_unlock(&thread_arg->clients_mutex); // Destrava antes de chamar broadcast para não travar
                broadcast_appliance_status(thread_arg, i);
                pthread_mutex_lock(&thread_arg->clients_mutex); // Trava de novo para continuar o loop
            }
        }

        pthread_mutex_unlock(&thread_arg->clients_mutex);
    }
    return NULL;
}

int main(int argc, char **argv)
{
    THREAD_ARG_STRUCT *thread_arg = malloc(sizeof(THREAD_ARG_STRUCT));
    // Initialize clients
    thread_arg->connected = 0;
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        thread_arg->clients[i].socket = 0;
    }

    // Inicialize appliances
    init_appliances();

    // Inicialize counters
    init_counters();

    // Initialize server socket
    int master_socket, opt = 1;
    if (!(master_socket = socket(AF_INET, SOCK_STREAM, 0)))
        fail("Socket failed");
    if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
        fail("Setsockopt failed");

    // Set address
    thread_arg->address.sin_family = AF_INET;
    thread_arg->address.sin_addr.s_addr = INADDR_ANY;
    thread_arg->address.sin_port = htons(PORT);

    // Bind and listen socket
    if (bind(master_socket, (struct sockaddr *)&thread_arg->address, sizeof(thread_arg->address)) < 0)
        fail("Bind failed");
    if (listen(master_socket, 3) < 0)
        fail("Listen failed");

    printf("Waiting for connections...\n");

    // Create a thread for the server
    INDEXED_THREAD_ARG_STRUCT *master_arg_struct = malloc(sizeof(INDEXED_THREAD_ARG_STRUCT));
    master_arg_struct->socket_index = master_socket;
    master_arg_struct->thread_arg = thread_arg;
    pthread_t master_thr;
    pthread_create(&master_thr, NULL, read_master_socket, master_arg_struct);

    // Create a thread for each of the clients
    pthread_t client_thr[MAX_PLAYERS];
    // Keeps a list of indexed structs (passing by reference)
    INDEXED_THREAD_ARG_STRUCT *indexed_arg_struct = malloc(4 * sizeof(INDEXED_THREAD_ARG_STRUCT));
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        indexed_arg_struct[i].socket_index = i;
        indexed_arg_struct[i].thread_arg = thread_arg;
        pthread_create(&client_thr[i], NULL, read_client_socket, &indexed_arg_struct[i]);
    }

    // Create Game Loop Thread
    pthread_t game_thr;
    pthread_create(&game_thr, NULL, game_loop_thread, thread_arg);

    // Join threads
    pthread_join(master_thr, NULL);
    for (int i = 0; i < MAX_PLAYERS; i++)
        pthread_join(client_thr[i], NULL);

    // Free space
    free(indexed_arg_struct);
    free(master_arg_struct);
    free(thread_arg);

    return 0;
}