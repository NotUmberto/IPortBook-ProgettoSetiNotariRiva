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


int main() {   
    //creiamo la struct degli utenti registrati
    volatile struct registered_users server; 
    server.number=0;

    //creazione socket e bind cosi mettiamo il server ad ascoltare possibili nuove connessioni
    //su una certa porta. 
    int sockfd=socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in my_sockaddr;

    my_sockaddr.sin_family=AF_INET;
    my_sockaddr.sin_port=htons(4242);
    my_sockaddr.sin_addr.s_addr=htonl(INADDR_ANY);

    int b=bind(sockfd, (struct sockaddr*)&my_sockaddr,sizeof(struct sockaddr));
    if (b!=0) {printf("errore listen\n"); return 1;} 

    int l=listen(sockfd,0);
    if (l!=0) {printf("errore listen\n"); return 1;}

    //gestione delle connessioni. ogni connessione sarà gestita da un thread a parte.
    int active_fd=0;
    socklen_t sizeofcaller;
    struct sockaddr caller;

    while(1)
    {
        active_fd=accept(sockfd, &caller, &sizeofcaller);//op bloccante.
        //otteniamo anche le info del chiamante che potremmo mettere nella struct del cliente
        
        //creiamo anche il thread per le notifiche UDP?
        //creo un nuovo thread per gestire il cliente. al nuovo thread bisognerà passare
        //active_fd e forse anche caller

    }
    

    return 0;
}