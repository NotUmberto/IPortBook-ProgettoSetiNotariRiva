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

    int ok=1;
    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&ok,sizeof(ok));

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

    printf("Scegli la tua porta UDP:\n");
    int udp_port=5555;
    
    scanf("%d", &udp_port);
    
    char trash;
    fread(&trash, 1, sizeof(char), stdin); //'\n'

    printf("Connesso.\ninviare un messaggio al server...\n");

    
    pthread_t udp_thread;
    pthread_create(&udp_thread,NULL,ListenNotify,&udp_port);

    int buf1_size=maxTCPMessageLength+1;
    char* buf1=malloc(buf1_size*sizeof(char));
    while(1)
    {
        printf("puoi inviare un messaggio\n");
        fgets(buf1, buf1_size, stdin); //mandiamo il segnale
        printf("mandato messaggio:%s\n", buf1);

        
        if (strcmp(buf1,"\n")==0) { 
            printf("stai attento!!!! (1 tentativo rimasto)\n");
            //riproviamo come se niente fosse
            fgets(buf1, buf1_size, stdin); //mandiamo il segnale
        }

        
        //togliamo il \n dopo i +++. se non lo facciamo il server legge male.
        if(buf1[strlen(buf1)-1]=='\n')buf1[strlen(buf1)-1]='\0';


        write(sockfd, buf1, strlen(buf1)); //SI MANDA QUANTO LETTO NON SEMPRE LA STESSA GRANDEZZA

        TCP_message_handler(sockfd); //gestisce cosa riceve   
    }

    pthread_join(udp_thread,NULL);

    return 0;
}