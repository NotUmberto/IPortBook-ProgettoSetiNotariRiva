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
#include "server_code.h"

extern volatile struct registered_users server;

int main() {   
    //creiamo la struct degli utenti registrati
    server.number=0;

    //creazione socket e bind cosi mettiamo il server ad ascoltare possibili nuove connessioni
    //su una certa porta. 
    int sockfd=socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in my_sockaddr;

    my_sockaddr.sin_family=AF_INET;
    my_sockaddr.sin_port=htons(4242);
    my_sockaddr.sin_addr.s_addr=htonl(INADDR_ANY);

    int b=bind(sockfd, (struct sockaddr*)&my_sockaddr,sizeof(struct sockaddr));
    if (b!=0) {printf("errore bind\n"); return 1;} 

    int l=listen(sockfd,0);
    if (l!=0) {printf("errore listen\n"); return 1;}

    //gestione delle connessioni. ogni connessione sarà gestita da un thread a parte.
    struct sockaddr_in caller;    
    socklen_t sizeofcaller=sizeof(caller);
    printf("sto per iniziare il while\n");
    while(1) {
        int active_fd=accept(sockfd, (struct sockaddr*)&caller, &sizeofcaller);//op bloccante.
        printf("ho accettato la connessione\n");
        struct package user_info;
        user_info.activefd=active_fd;
        user_info.caller=caller;
        user_info.sizecaller=sizeofcaller;
        user_info.pos=server.number;

        server.number++;
        pthread_t tcp_thread;
        pthread_create(&tcp_thread,NULL,TCP_message_handler,&user_info);
    }
    

    return 0;
}