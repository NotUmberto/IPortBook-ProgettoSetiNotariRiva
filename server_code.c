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

//ritorna la posizione di id nell'array dei clienti. se non esiste ritorna -1
int search_user_id(char* infoid)
{
    int p=-1;
    for (unsigned i=0;i<server.number;i++) { 
        //cerca il cliente
        if (strcmp(server.array[i]->id,infoid)==0) { 
            //printf("l'ho trovato\n");
            p=i;
            break;
        }
    } 
    return p;
}

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

    pthread_mutex_lock(&lock);

    Client newclient;
    newclient=malloc(sizeof(struct client));

    //---------lettura id--------------
    char infoid[9];
    readline(fd, infoid);
    printf("id ricevuto: %s\n", infoid);

    //printf("lunghezza di infoid -%s- %ld\n",infoid,strlen(infoid));
    newclient->id=malloc(strlen(infoid)+1); 
    strcpy(newclient->id,infoid);

    //---------lettura porta UDP--------------
    char infoport[5];
    readline(fd, infoport);
    printf("port ricevuta: %s\n", infoport);

    newclient->UDP_port=malloc(strlen(infoport)+1); 
    strcpy(newclient->UDP_port,infoport);

    //---------lettura password--------------
    char infopass[9];
    readline(fd, infopass);
    int pass=atoi(infopass);

    printf("password ricevuta: %d\n", pass);

    if (pass<0 || pass>65535) { 
        printf("password errata\n");
        write(fd, "GOBYE+++", 8); //invio il messaggio di errore
        close(fd);
        return;
    }
    newclient->password=pass; //per la password

    //---------inserimento connection--------------
    newclient->isConnected=true;

    //---------aggiunta alla struct degli utenti registrati--------------
    server.array[p]=newclient;
    pthread_mutex_unlock(&lock);
    printf("cliente aggiunto alla struct con successo!\n");

    //printf("stampo i dati del nuovo cliente\n");
    //printf("id %s, udp %s, pass %d\n",server.array[p]->id,server.array[p]->UDP_port,server.array[p]->password);
    server.number++;

    write(fd, "WELCO+++", 8); //invio il messaggio di benvenuto
    return;
}

int CONNE_handler(int sock, int p) { 
    printf("siamo al CONNE\n");
    //CONNE ID PASS+++
    char infoid[9];
    readline(sock, infoid);

    p=search_user_id(infoid);

    //caso in cui e' stato fatto il CONNE senza REGIS
    if (p==-1) { 
        printf("id non trovato\n");

        printf("invio GOBYE+++\n");
        char goodbye_msg[]="GOBYE+++";
        write(sock, goodbye_msg, strlen(goodbye_msg));

        close(sock);
        return -1;
    }

    printf("id ricevuto: %s\n", infoid);

    if(strcmp(infoid,server.array[p]->id)==0) {
        printf("id corretto\n");
    } else {
        printf("id errato\n");

        printf("invio GOBYE+++\n");
        char goodbye_msg[]="GOBYE+++";
        write(sock, goodbye_msg, strlen(goodbye_msg));

        close(sock);
        return -1;
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
        return -1;
    }

    return p;
}
//controlla se 2 persone sono amiche.
bool areFriends(int pos1, int pos2)
{
    if(pos1>=server.number || pos2>=server.number)
    {
        printf("errore areFriends(): uno (o piu) dei due utenti non è registrato nel server\n");
        return false;
    }
    else
    {
        int minFriendNumber=server.array[pos1]->friendNumber;
        if(server.array[pos2]->friendNumber<minFriendNumber)
        {
            minFriendNumber=server.array[pos2]->friendNumber;
        }

        for(int i=0;i<minFriendNumber;i++)
        {
            if(strcmp(server.array[pos1]->friends[i]->id,server.array[pos2]->id)==0)
            {
                //assumo che la relazione di amicizia sia biunivoca. quindi non controllo anche gli amici dell'altro utente
                return true; 
            }
        }
    }
    return false;
}

void FRIEND_REQUEST_handler(int sock, int active_pos) { 
    printf("siamo al FRIEND_REQUEST\n");
    
    char infoid[9];
    readline(sock, infoid);
    printf("id ricevuto: %s\n", infoid);

    //controlliamo se l'id esiste
    int friend_pos=search_user_id(infoid);

    char unknown_id_msg[]="FRIE<+++";
    if(friend_pos==-1)//l'utente a cui vogliamo chiedere l'amicizia non esiste
    {
        printf("Utente inesistente. invio %s\n", unknown_id_msg);
        write(sock, unknown_id_msg, strlen(unknown_id_msg)*sizeof(char));
        return;
    }
    else if(friend_pos==active_pos)//l'utente vuole fare amicizia con se stesso
    {
        printf("Impossibile fare amicizia con se stessi. invio %s\n", unknown_id_msg);
        write(sock, unknown_id_msg, strlen(unknown_id_msg)*sizeof(char));
        return;
    }
    else if(areFriends(friend_pos, active_pos))//controlliamo se i due sono già amici
    {
        
    }
    return;
}

void MESSAGE_HANDLER() { 
    printf("siamo al MESSAGE_HANDLER\n");
    return;
}
//ecc ecc

//gestisce il cliente a seconda di quello che ha mandato. gli args dovranno includere almeno il sockfd per comunicare in tcp col cliente. probabilmente andranno anche inclusi i campi del chiamante compilati da accept() cosi da usarli in REGIS_handler() direi.
void* TCP_message_handler(void* args) {
    struct package info=*((struct package*)args);

    //posizione del cliente attuale in server.array[]. -1 se non la conosciamo ancora. verrà impostata da CONNE
    int active_pos=-1;

    int buf1_size=6;
    char* buf1=malloc(buf1_size*sizeof(char));
    while(1) {
        printf("sto per leggere il comando\n");
        int val=read(info.activefd, buf1, buf1_size);
        printf("valore di read: %d\n", val);
        printf("ho appena letto il comando\n");
        buf1[val-1]='\0';
        printf("strlen=%ld\n", strlen(buf1));
        printf("comando ricevuto: %s\n", buf1);

        if(strcmp(buf1, "REGIS")==0)
        {
            //char s;
            //read(info.activefd,&s,1);
            REGIS_handler(info.activefd,info.pos);
        }
        else if(strcmp(buf1, "CONNE")==0 )
        {
            //char s;
            //read(info.activefd,&s,1);
            active_pos=CONNE_handler(info.activefd,info.pos);
        }
        else if(strcmp(buf1, "FRIE?")==0 )
        {
            FRIEND_REQUEST_handler(info.activefd, active_pos);
        }
        else if(strcmp(buf1, "MESS?")==0)
        {
            MESSAGE_HANDLER();
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
            printf("comando non riconosciuto\n");
            char goodbye_msg[]="GOBYE+++";
            write(info.activefd, goodbye_msg, strlen(goodbye_msg));
            close(info.activefd);
            pthread_exit(NULL);
            return NULL;
        }
    }
    //qua e' finito il while (non dovrebbe mai finire)
    return NULL;
}