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

pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;

int readline(int sock,char *b) {//se troviamo uno spazio abbiamo trovato una parola
    //ATTENZIONE A NON SUPERARE LA LUNGHEZZA DI B
    char *bpos=b;
    char c;
    int r=read(sock,&c,sizeof(char));

    if (c!=' '){ //se il primo char è uno spazio non lo mettiamo nel buffer
        *bpos=c;
    }

    while(r==1 && *bpos!=' ' && *bpos!='\n'){
        ++bpos;
        read(sock,bpos,sizeof(char));}
    if(r==1){
        ++bpos;
        *bpos='\0';
        return 0;
    } else {
        return 1;
    }

}

//lista di tutti i messaggi tcp possibili
//REGIS, WELCO, GOBYE, CONNE, HELLO,FRIE?, FRIE>, FRIE<, MESS?, MESS>, MESS<, FLOO>, FLOO?, 
// LIST?, RLIST, LINUM,CONSU, SSEM>,OOLF>, EIRF>, OKIRF, NOKRF, ACKRF, FRIEN, NOFRI, NOCON
void REGIS_handler(int fd,int p) { //la sequenza di lettura e' REGIS_id_UDP-port_password
    printf("siamo al REGIS\n");

    if (p>MAXCLIENTS) { 
        printf("il server e' pieno!\n");
        close(fd);
        return;
    }

    char infoid[9];
    readline(fd, infoid);
    printf("id ricevuto: %s\n", infoid);

    char infoport[5];
    readline(fd, infoport);
    printf("port ricevuta: %s\n", infoport);

    char infopass[8];
    readline(fd, infopass);
    printf("password ricevuta: %s\n", infopass);

    int pass=atoi(infopass);
    if (pass<0 || pass>65535) { 
        printf("password errata\n");
        close(fd);
        return;
    }
    printf("password corretta\n");

    //creiamo il nuovo cliente 
    
    pthread_mutex_lock(&lock);

    Client newclient;
    newclient=malloc(sizeof(struct client));

    newclient->id=malloc(strlen(infoid)+1); //per l'id
    strcpy(newclient->id,infoid);

    newclient->UDP_port=malloc(strlen(infoport)+1); //per la porta UDP
    strcpy(newclient->UDP_port,infoport);

    newclient->password=pass; //per la password

    newclient->isConnected=false;

    //lo aggiungiamo alla struct
    server.array[p]=newclient;
    pthread_mutex_unlock(&lock);
    printf("cliente aggiunto alla struct con successo!\n");
    server.number++;
    return;
}

void CONNE_handler(int sock, int p) { 
    printf("siamo al CONNE\n");
    //CONNE ID PASS+++
    char infoid[9];
    readline(sock, infoid);

    p=0;
    for (unsigned i=0;i<server.number;i++) { 
        //cerca il cliente
        if (strcmp(server.array[i]->id,infoid)==0) { 
            printf("l'ho trovato\n");
            p=i;
            break;
        }
    }

    printf("p e' in posizione %d\n",p);
    printf("id da confrontare %s\n", server.array[p]->id);    

    printf("id ricevuto: %s\n", infoid);

    if(strcmp(infoid,server.array[p]->id)==0) {
        printf("id corretto\n");
    } else {
        printf("id errato\n");

        printf("invio GOBYE+++\n");
        char goodbye_msg[]="GOBYE+++";
        write(sock, goodbye_msg, strlen(goodbye_msg));

        close(sock);
        return;
    }

    printf("sto per leggere la password\n");
    char infopass[6];
    readline(sock, infopass);
    printf("password ricevuta: %s\n", infopass);
    if(atoi(infopass)==server.array[p]->password)
    {
        printf("password corretta\n");

        printf("invio HELLO+++\n");
        char hello_msg[]="HELLO+++";
        write(sock, hello_msg, strlen(hello_msg));

        server.array[p]->isConnected=true;
    }
    else
    {
        printf("password errata\n");

        printf("invio GOBYE+++\n");
        char goodbye_msg[]="GOBYE+++";
        write(sock, goodbye_msg, strlen(goodbye_msg));

        close(sock);
        return;
    }

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
            char s;
            read(info.activefd,&s,1);
            REGIS_handler(info.activefd,info.pos);
        }

        if(strcmp(buf1, "CONNE")==0 )
        {
            char s;
            read(info.activefd,&s,1);
            CONNE_handler(info.activefd,info.pos);
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