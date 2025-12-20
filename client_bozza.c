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
    struct sockaddr_in dst;
    int sockfd=socket(AF_INET, SOCK_STREAM, 0);   

    inet_aton("localhost", &dst.sin_addr);

    dst.sin_family=AF_INET;
    dst.sin_port=htons(4242);//molto provvisorio
    int r=connect(sockfd, (struct sockaddr*)&dst, sizeof(struct sockaddr_in));
    if(r!=0)
    {
        printf("Problema di connessione\n");
        close(sockfd);
        return 1;
    }
    printf("Connesso.\ninviare un messaggio al server...\n");

    int udp_port=5555;
    pthread_t udp_thread;
    pthread_create(&udp_thread,NULL,ListenNotify,&udp_port);

    int buf1_size=maxTCPMessageLength+1;
    char* buf1=malloc(buf1_size*sizeof(char));
    while(1)
    {
        fgets(buf1, buf1_size, stdin); //mandiamo il segnale
        write(sockfd, buf1, buf1_size);

        TCP_message_handler(sockfd); //gestisce cosa riceve   
    }

    pthread_join(udp_thread,NULL);

    return 0;
}