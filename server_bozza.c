#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "server_code.h"


//una struct per memorizzare le info dei clienti
struct client
{
    //int perchè il c non ha un bool vero e proprio
    int isConnected;

    char* id;//sarà un array di 8 char
    char* UDP_port;//<9999

    // numero compreso tra 0 e 65535. char perchè mi sembra che in certi messaggi 
    // tcp vada mandata la password. se la memorizziamo già come string forse è piu comodo
    char* password;
};
//array statico di 100 clienti dato che non ce ne possono essere di piu, pare. 
//puntatori a struct perchè penso in questo modo sappiamo se un cliente è registrato se la sua
//cella non è null. dovrebbe esserci anche un risparmio di memoria. 
int const maxClients=100;
volatile struct client* registered_users_array[maxClients];

//forse un grafo con matrice di adiacenza per gestire le richieste di flood potrebbe essere furbo.


int main()
{
    //creazione socket e bind cosi mettiamo il server ad ascoltare possibili nuove connessioni
    //su una certa porta. 
    int sockfd=socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in my_sockaddr;
    
    my_sockaddr.sin_family=AF_INET;
    my_sockaddr.sin_port=htons(4242);
    my_sockaddr.sin_addr.s_addr=htonl(INADDR_ANY);

    bind(sockfd, (struct sockaddr*)&my_sockaddr,sizeof(struct sockaddr));
    listen(sockfd,0);

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