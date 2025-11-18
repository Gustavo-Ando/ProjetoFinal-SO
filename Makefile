all:
	gcc client.c -std=c99 -Wall -o client.out -lcurses
	gcc server.c -std=c99 -Wall -o server.out
