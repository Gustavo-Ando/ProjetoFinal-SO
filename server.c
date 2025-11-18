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

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 4

typedef struct _client {
    int socket;
    int x, y;
} CLIENT;

void fail(char *str){
    perror(str);
    exit(EXIT_FAILURE);
}

CLIENT clients[MAX_CLIENTS];
int connected; // Number of connected clients
pthread_mutex_t clients_mutex;

struct sockaddr_in address;

// Thread to send updates to the players when they move
static void send_player_update(int index){
    pthread_mutex_lock(&clients_mutex);
    char message[BUFFER_SIZE + 1] = { 0 };
    message[0] = '0' + index;
    message[1] = '0' + clients[index].x;
    message[2] = '0' + clients[index].y;
    if(clients[index].socket == 0) {
        message[1] = '0' - 1;
        message[2] = '\0';
    }
    for(int i = 0; i < MAX_CLIENTS; i++){
        if(clients[i].socket != 0) {
            if(send(clients[i].socket, message, strlen(message), 0) != strlen(message)) fail("Send error");
        }
    }
    pthread_mutex_unlock(&clients_mutex);

}

// Thread to deal with master_socket (accept new connections)
static void *read_master_socket(void *args){
    int master_socket = *(int*)args;
    int addr_len = sizeof(address);
    
    while(1){
        // Accepts the connection
        int new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t *)&addr_len);
        if(new_socket < 0) fail("Accept failed");
        printf("New connection!\n");
        printf(" - Socket FD: %d\n", new_socket);
        printf(" - IP: %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
        
        // Access CR to read number of connected clients and check if a connection is possible
        int is_full = 0;
        pthread_mutex_lock(&clients_mutex);
        if(connected == MAX_CLIENTS) is_full = 1;
        pthread_mutex_unlock(&clients_mutex);

        // If reached the limit, reject the connection and continue
        if(is_full){
            printf(" - Rejected: Server full.\n\n");
            char *message = "Connection rejected";
            send(new_socket, message, strlen(message), 0);
            continue;
        }
        
        // Otherwise, send a confirmation message
        char *message = "Connection established";
        if(send(new_socket, message, strlen(message), 0) != strlen(message)) fail("Send error");

        // Access CR to increase number of connected clients and add client to the list
        pthread_mutex_lock(&clients_mutex);
        int new_index;
        for(int i = 0; i < MAX_CLIENTS; i++){
            if(clients[i].socket != 0) continue;
            clients[i].socket = new_socket;
            clients[i].x = 6;
            clients[i].y = 5;
            new_index = i;
            connected++;
            printf(" - Adding socket as %d\n\n", i);
            break;
        }
        pthread_mutex_unlock(&clients_mutex);
        send_player_update(new_index);
    }

    return NULL;
}

// Thread to deal with messages received from the clients
static void *read_client_socket(void *args){
    int index = *(int *)args;
    int addr_len = sizeof(address);
    // Repeat to check if socket exists 
    while(1){
        // Access CR to read the socket
        pthread_mutex_lock(&clients_mutex);
        int sd = clients[index].socket;
        pthread_mutex_unlock(&clients_mutex);
        if(sd == 0) continue;
        // If exists, treat client until it disconnects
        while(1){
            char buffer[BUFFER_SIZE + 1];
            int valread = read(sd, buffer, BUFFER_SIZE);
    
            if(valread == 0) {
                // Disconnected
                getpeername(sd, (struct sockaddr *)&address, (socklen_t *)&addr_len);
                printf("Host disconnected.\n");
                printf(" - IP: %s:%d\n\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
    
                close(sd);
                // Access CR to disconnect the client
                pthread_mutex_lock(&clients_mutex);
                clients[index].socket = 0;
                connected--;
                pthread_mutex_unlock(&clients_mutex);
                send_player_update(index);
                break;
            } else {
                // Game map with blocked spaces
                int game_map[10][19] = {
                    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
                    {1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1},
                    {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
                    {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
                    {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
                    {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
                    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
                    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
                    {1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1},
                    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
                };
                // Treat Received message
                pthread_mutex_lock(&clients_mutex);
                // Deals with the movements
                int try_moved = 1;
                switch(buffer[0]) {
                    case 'w':
                        if(!(game_map[clients[index].y - 1][clients[index].x])) 
                            clients[index].y--;
                        break;
                    case 'a':
                        if(!(game_map[clients[index].y][clients[index].x - 2])) 
                            clients[index].x -= 2;
                        break;
                    case 's':
                        if(!(game_map[clients[index].y + 1][clients[index].x])) 
                            clients[index].y++;
                        break;
                    case 'd':
                        if(!(game_map[clients[index].y][clients[index].x + 2])) 
                            clients[index].x += 2;
                        break;
                    default:
                        try_moved = 0;
                        break;
                }
                printf("Received message from client %d: %s\n", index, buffer);
                printf(" - %d, %d\n", clients[index].x, clients[index].y);
                pthread_mutex_unlock(&clients_mutex);
                if(try_moved) send_player_update(index);
                
            }
        }
    }

    return NULL;
}

int main(int argc, char** argv){
    // Initialize clients
    connected = 0;
    for(int i = 0; i < MAX_CLIENTS; i++){
        clients[i].socket = 0;
    }

    // Initialize server socket
    int master_socket, opt = 1;
    if(!(master_socket = socket(AF_INET, SOCK_STREAM, 0))) fail("Socket failed");
    if(setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) fail("Setsockopt failed");

    // Set address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind and listen socket
    if(bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0) fail("Bind failed");
    if(listen(master_socket, 3) < 0) fail("Listen failed");

    printf("Waiting for connections...\n");

    // Create a thread for the server
    pthread_t master_thr;
    pthread_create(&master_thr, NULL, read_master_socket, &master_socket);

    // Create a thread for each of the clients 
    pthread_t client_thr[MAX_CLIENTS];
    // Keeps a list of indexes (passing by reference)
    int client_index[MAX_CLIENTS];
    for(int i = 0; i < MAX_CLIENTS; i++){
        client_index[i] = i;
        pthread_create(&client_thr[i], NULL, read_client_socket, &client_index[i]);
    }

    // Join threads
    pthread_join(master_thr, NULL);
    for(int i = 0; i < MAX_CLIENTS; i++) pthread_join(client_thr[i], NULL);
    
    return 0;
}


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

/*

                   (Batata Frita)
                ########5#5########
         (Lixo) ##X|  \_O_W_/  (@)# (Salada)
                ##_|      d       #
                ##_|a            [] o  EEW 3
      (Bancada) ##_|            c[] o/ @ 2    (Clientes)
                ##_|             [] o  @W 5
                #                [] o/ ÚÚ 1
                #       b         #
         (Suco) #(Ú)  /¨-¨0¨\  (=)# (Pão) 
                ########5#5########
                    (Hambúrguer)    

*/
