all : server client

server : server.c server_code.c server_code.h stack.h stack.c
	gcc -Wall -pthread server.c server_code.c stack.c -o server
    
client : client.c client_code.c client_code.h
	gcc -Wall -pthread client.c client_code.c -o client