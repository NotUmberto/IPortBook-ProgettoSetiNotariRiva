all : server client

server : server_bozza.c server_code.c server_code.h stack.h stack.c
	gcc -Wall -pthread server_bozza.c server_code.c stack.c -o server
    
client : client_bozza.c client_code.c client_code.h
	gcc -Wall -pthread client_bozza.c client_code.c -o client