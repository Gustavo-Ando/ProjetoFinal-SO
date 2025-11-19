all:
	gcc client.c message.c -std=c99 -Wall -o client.out -lcurses
	gcc server.c message.c -std=c99 -Wall -o server.out
