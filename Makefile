all:
	gcc client.c utility.c message.c map.c render.c client_process_message.c -std=c99 -Wall -o client.out -lcurses
	gcc server.c utility.c message.c map.c server_send_message.c -std=c99 -Wall -o server.out -lcurses
