all:
	gcc client.c utility.c message.c map.c render.c client_process_message.c -Wall -o client.out -lcurses -lrt
	gcc server.c utility.c message.c map.c server_send_message.c server_game_logic.c -Wall -o server.out -lcurses -lrt
