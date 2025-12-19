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

volatile struct registered_users server;

//lista di tutti i messaggi tcp possibili
//REGIS, WELCO, GOBYE, CONNE, HELLO,FRIE?, FRIE>, FRIE<, MESS?, MESS>, MESS<, FLOO>, FLOO?, 
// LIST?, RLIST, LINUM,CONSU, SSEM>,OOLF>, EIRF>, OKIRF, NOKRF, ACKRF, FRIEN, NOFRI, NOCON
void REGIS_handler() { 
    printf("siamo al regis\n");
    return;
}
void CONNE_handler() { 
    printf("siamo al conne\n");
    //server.array[].isconnected=true;
    return;
}
void FRIEND_REQUEST_handler() { 
    return;
}
//ecc ecc

//gestisce il cliente a seconda di quello che ha mandato. gli args dovranno includere almeno il sockfd per comunicare in tcp col cliente. probabilmente andranno anche inclusi i campi del chiamante compilati da accept() cosi da usarli in REGIS_handler() direi.
void* TCP_message_handler(void* args) {
    struct package info=*((struct package*)args);

    int buf1_size=6;
    char* buf1=malloc(buf1_size*sizeof(char));
    while(1)
    {
        int sizeread=read(info.activefd, buf1, buf1_size-1);
        buf1[buf1_size-1]='\0';

        if(strcmp(buf1, "REGIS")==0)
        {
            REGIS_handler();
        }

        if(strcmp(buf1, "CONNE")==0 )
        {
                
        }

        if(server.array[info.pos]->isConnected)
        {
            if(strcmp(buf1, "FRIE?")==0 )
            {
                
            }
            else if(strcmp(buf1, "MESS?")==0)
            {

            }
            else if(strcmp(buf1, "FLOO?")==0)
            {
                
            }
            else if(strcmp(buf1, "LIST?")==0)
            {
                
            }
            else if(strcmp(buf1, "CONSU")==0)
            {
                
            }
            else if(strcmp(buf1, "OKIRF")==0)
            {
                
            }
            else if(strcmp(buf1, "NOKRF")==0)
            {
                
            }
            else if(strcmp(buf1, "IQUIT")==0)
            {

            }
            else 
            {
                //manda un GOBYE, dealloca tutta la memoria usata dal thread, chiude la connessione e termina il thread.
            }
        }
        else
        {
            //manda un GOBYE, dealloca tutta la memoria usata dal thread, chiude la connessione e termina il thread.
        }
        
        
    }
    return NULL;
}