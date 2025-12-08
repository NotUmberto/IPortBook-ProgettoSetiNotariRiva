#define _POSIX_C_SOURCE 200112L //da togliere prima di presentare
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>

void* ListenNotify(void*) { 
    //creazione socket UDP per ascoltare le notifiche
    int sock=socket(AF_INET,SOCK_DGRAM,0);
    struct sockaddr_in address_sock;
    address_sock.sin_family=AF_INET;
    address_sock.sin_port=htons(5555);
    address_sock.sin_addr.s_addr=htonl(INADDR_ANY);
    int b=bind(sock,(struct sockaddr*)&address_sock,sizeof(struct sockaddr_in));
    
    if (b==0) { 
        //leggiamo le notifiche
        while (1) { 
            char mex[4];
            int size=recv(sock,mex,4,0);
            if (size<3) printf("ERRORE size=%d\n",size);
            mex[size]='\0';
            printf("Ricevuto: %s\n",mex);
        }
    }
    close(sock);
    return NULL;
}

int main() { 
    pthread_t notifiche;
    pthread_create(&notifiche,NULL,ListenNotify,NULL);
    printf("ho creato il thread\n");

    pthread_join(notifiche,NULL);

    //inizio collegamento tcp...

}