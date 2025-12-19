#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#include "client_code.h"

int main() {
    int sockfd=socket(AF_INET, SOCK_STREAM, 0);   
    struct sockaddr_in dst;
    
    //penso che andrebbe chiesto ogni volta cosi potremmo collegarci a qualunque macchina
    //stia eseguendo il server. 
    char* server_address="localhost";//molto provvisiorio
    struct in_addr address;
    inet_aton(server_address, &address);

    dst.sin_family=AF_INET;
    dst.sin_addr=address;
    dst.sin_port=htons(4242);//molto provvisorio
    int r=connect(sockfd, (struct sockaddr*)&address, sizeof(struct sockaddr_in));
    if(r!=0)
    {
        printf("Problema di connessione\n");
        close(sockfd);
        return 1;
    }
    printf("Connesso.\ninviare un messaggio al server...\n");

    //qui penso sarebbe da iniziare il thread per le notifiche UDP

    int buf1_size=maxTCPMessageLength+1;
    char* buf1=malloc(buf1_size*sizeof(char));
    while(1)
    {
        fgets(buf1, buf1_size, stdin);
        //qui in mezzo non sarebbe da usare l'handler?
        TCP_message_handler(sockfd);

        write(sockfd, buf1, buf1_size);

        //read();
        
    }

    return 0;
}