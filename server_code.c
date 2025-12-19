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

//lista di tutti i messaggi tcp possibili
//REGIS, WELCO, GOBYE, CONNE, HELLO,FRIE?, FRIE>, FRIE<, MESS?, MESS>, MESS<, FLOO>, FLOO?, 
// LIST?, RLIST, LINUM,CONSU, SSEM>,OOLF>, EIRF>, OKIRF, NOKRF, ACKRF, FRIEN, NOFRI, NOCON
void REGIS_handler() { 
    return;
}
void CONNE_handler() { 
    return;
}
void FRIEND_REQUEST_handler() { 
    return;
}
//ecc ecc

//gestisce il cliente a seconda di quello che ha mandato. gli args dovranno includere almeno il sockfd per comunicare in tcp col cliente. probabilmente andranno anche inclusi i campi del chiamante compilati da accept() cosi da usarli in REGIS_handler() direi.
void TCP_message_handler(void* args)
{
    int active_fd=*((int*)args);
    int buf1_size=6;
    char* buf1=malloc(buf1_size*sizeof(char));
    while(1)
    {
        read(active_fd, buf1, buf1_size-1);
        buf1[buf1_size-1]='\0';

        if(strcmp(buf1, "REGIS")==0 && /*l'user NON è connesso*/)
        {
            REGIS_handler();
        }
        else if(strcmp(buf1, "CONNE")==0 && /*l'user è connesso*/)
        {
            CONNE_handler();
        }
        else if(strcmp(buf1, "FRIE?")==0)
        {
            FRIEND_REQUEST_handler();
        }
        //altri casi...
        else 
        {
            //manda un GOBYE, dealloca tutta la memoria usata dal thread, chiude la connessione e termina il thread. questa cosa la puo fare anche un handler se si accorge che qualcosa non va.
        }
        
        
    }
}