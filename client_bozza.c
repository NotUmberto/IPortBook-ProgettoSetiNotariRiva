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

int const maxTCPMessageLength=218;

//una struct per memorizzare le info dei clienti
struct user
{
    //potremmo usare bool per rendere piu facile la lettura del codice
    //ex: if (isConnected) ...
    bool isConnected; 

    char* id;//sarà un array di 8 char
    char* UDP_port;//<9999
    char* password;// numero compreso tra 0 e 65535
};


int main()
{
    int sockfd=socket(AF_INET, SOCK_STREAM, 0);   
    struct sockaddr_in dst;
    
    //penso che andrebbe chiesto ogni volta cosi potremmo collegarci a qualunque macchina
    //stia eseguendo il server. 
    char* server_address=NULL;//molto provvisiorio
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
    int buf1_size=maxTCPMessageLength+1;
    char* buf1=malloc(buf1_size*sizeof(char));
    while(1)
    {
        fgets(buf1, buf1_size, stdin);
        //qui in mezzo non sarebbe da usare l'handler?
        write(sockfd, buf1, buf1_size);

        //read();

        
    }

    return 0;
}