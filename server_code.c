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
#include <endian.h>
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

//le seguenti funzioni devono essere usate se si è sicuri del prossimo campo che si vuole leggere. assumono che la prima cosa da leggere sia uno spazio(che verrà scartato). allocano della memoria che dovrà deallocare il chiamante. 
char* read_id(int sock)
{
    int buf1_size=9; //tengo conto anche di un \0
    char* buf1=malloc(buf1_size*sizeof(char));
    read(sock, buf1, 1);//tolgo lo spazio
    if(buf1[0]!=' ')
    {
            printf("errore read_id(): l'assunzione secondo la quale il primo carattere letto deve essere uno spazio è stata infranta\n");
            free(buf1);
            return NULL;
    }
    int byte_letti=read(sock, buf1, buf1_size-1);
    if(byte_letti<buf1_size-1)
    {
        printf("errore read_id(): campo non standard\n");
        free(buf1);
        return NULL;
    }
    buf1[byte_letti]='\0';
    return buf1;
}

char* read_UDP_port(int sock)
{
    int buf1_size=5; //tengo conto anche di un \0
    char* buf1=malloc(buf1_size*sizeof(char));

    read(sock, buf1, 1);//tolgo lo spazio
    if(buf1[0]!=' ')
    {
            printf("errore read_UDP_port(): l'assunzione secondo la quale il primo carattere letto deve essere uno spazio è stata infranta\n");
            free(buf1);
            return NULL;
    }
    int byte_letti=read(sock, buf1, buf1_size-1);
    if(byte_letti<buf1_size-1)
    {
        printf("errore read_UDP_port(): campo non standard\n");
        free(buf1);
        return NULL;
    }
    buf1[byte_letti]='\0';
    return buf1;
}
//per il campo mdp ovvero la password. la cosa brutta è che nei messaggi è codificata in little endian. è codificata su 2 byte. intende caratteri o numeri? penso numeri perchè altrimenti non ha senso il discorso little endian e avere solo 100 password è un po poco
uint16_t* read_password(int sock)
{
    int buf1_size=2;
    uint16_t* buf1=malloc(sizeof(uint16_t));
    *buf1=0;
    char spazio='a';
    read(sock, &spazio, 1);//tolgo lo spazio
    if(spazio!=' ')
    {
            printf("errore read_password(): l'assunzione secondo la quale il primo carattere letto deve essere uno spazio è stata infranta\n");
            free(buf1);
            return NULL;
    }
    int byte_letti=read(sock, buf1, buf1_size);
    if(byte_letti<buf1_size)
    {
        printf("errore read_password(): campo non standard\n");
        free(buf1);
        return NULL;
    }

    *buf1=le16toh(*buf1); //cosi funziona sia su little che su big endian

    return buf1;
}

char* read_num_item(int sock)
{
    int buf1_size=4; //tengo conto anche di un \0
    char* buf1=malloc(buf1_size*sizeof(char));
    read(sock, buf1, 1);//tolgo lo spazio
    if(buf1[0]!=' ')
    {
            printf("errore read_num_item(): l'assunzione secondo la quale il primo carattere letto deve essere uno spazio è stata infranta\n");
            free(buf1);
            return NULL;
    }

    int byte_letti=read(sock, buf1, buf1_size-1);
    if(byte_letti<buf1_size-1)
    {
        printf("errore read_num_item(): campo non standard\n");
        free(buf1);
        return NULL;
    }
    buf1[byte_letti]='\0';
    return buf1;
}


//vorrei farla simile alle precedenti ma il messaggio è di lunghezza variabile
char* read_mess(int sock)
{
    //leggo char per char perchè qui devo rilevare il terminatore "+++"
    int buf1_size=204;//200 per il messaggio, 3 per il terminatore, 1 per il nullbyte
    char* buf1=malloc(buf1_size*sizeof(char));
    char c=' '; //inizializzo cosi perchè è un carattere relativamente speciale
    int byte_letti=0;

    read(sock, &c, 1);//tolgo lo spazio iniziale
    if(c!=' ')
    {
            printf("errore read_mess(): l'assunzione secondo la quale il primo carattere letto deve essere uno spazio è stata infranta\n");
            free(buf1);
            return NULL;
    }

    int plusCount=0;
    int byte_totali=0;
    while(plusCount<3 && byte_totali<buf1_size-1)
    {
        byte_letti=read(sock, &c, 1);
        if(byte_letti<1)
        {
            printf("errore read_mess(): la lettura si è interrotta prima del terminatore\n");
            free(buf1);
            return NULL;
        }

        if(c=='+')plusCount++;
        else 
        {
            plusCount=0;
        }

        buf1[byte_totali]=c;
        byte_totali++;
    }
    buf1[byte_totali]='\0';
    //qua bisogna fare attenzione se ho un mess che è tipo 199 caratteri e poi +++. col codice attuale uscirei solo perchè ho raggiunto i caratteri massimi. ma io devo uscire in primo luogo perchè ho incontrato i 3 +++. nell'esempio che ho appena fatto, avrei un messaggio tipo "ciao...+", ma magari questo + faceva parte del terminatore
    if(plusCount==3)
    {
        //perfetto, abbiamo incontrato il terminatore
        
        buf1[byte_totali-plusCount]='\0';
        char* buf2=malloc((byte_totali-plusCount+1)*sizeof(char));
        strcpy(buf2, buf1);
        free(buf1);

        return buf2;
        
        
    }
    else if(byte_totali==buf1_size-1 && plusCount!=3)
    {
        //il messaggio era troppo lungo
        printf("errore read_mess(): campo non standard\n");
        free(buf1);
        return NULL;
    }
    else
    {
        //non dovrebbe neanche arrivarci qua
        printf("errore read_mess(): errore sconosciuto\n");
        free(buf1);
        return NULL;
    }
    return NULL;//risparmio warning
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
    char* infoid=read_id(fd);
    if(infoid==NULL)return;//non stampo niente perchè lo fa gia la read_...(fd) di turno
    printf("id ricevuto: %s\n", infoid);

    //printf("lunghezza di infoid -%s- %ld\n",infoid,strlen(infoid));
    newclient->id=malloc(strlen(infoid)+1); 
    strcpy(newclient->id,infoid);

    free(infoid);

    //---------lettura porta UDP--------------
    char* infoport=read_UDP_port(fd);
    if(infoport==NULL)return;//non stampo niente perchè lo fa gia la read_...(fd) di turno
    printf("port ricevuta: %s\n", infoport);

    newclient->UDP_port=malloc(strlen(infoport)+1); 
    strcpy(newclient->UDP_port,infoport);

    free(infoport);
    //---------lettura password--------------
    
    uint16_t* temp=read_password(fd);

    if(temp==NULL)return;//non stampo niente perchè lo fa gia la read_...(fd) di turno
    uint16_t pass=*temp;
    free(temp);
    printf("password ricevuta: %d\n", pass);

    if (pass<0 || pass>65535) { 
        printf("password errata\n");
        write(fd, "GOBYE+++", 8); 
        close(fd);
        return;
    }
    //per terminare la lettura del comando
    char terminatore[3];
    int byte_letti=read(fd, terminatore, 3);
    if(byte_letti<3)
    {
        printf("errore REGIS_handler():Formato del comando non standard\n");
        write(fd, "GOBYE+++", 8); 
        close(fd);
        return;
    }
    //

    newclient->password=pass; //per la password

    //---------inserimento connection--------------

    newclient->isConnected=true; //siamo sicuri di sta linea? 

    //---------aggiunta alla struct degli utenti registrati--------------
    server.array[p]=newclient;

    server.number++;

    pthread_mutex_unlock(&lock);
    printf("cliente aggiunto alla struct con successo!\n");

    //printf("stampo i dati del nuovo cliente\n");
    //printf("id %s, udp %s, pass %d\n",server.array[p]->id,server.array[p]->UDP_port,server.array[p]->password);
    

    write(fd, "WELCO+++", 8); //invio il messaggio di benvenuto
    return;
}

int CONNE_handler(int sock, int p) { 
    printf("siamo al CONNE\n");
    //CONNE ID PASS+++

    char* infoid=read_id(sock);
    if(infoid==NULL)return -1;//non stampo niente perchè lo fa gia la read_...(fd) di turno
    printf("id ricevuto: %s\n", infoid);

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
    /*
    if(strcmp(infoid,server.array[p]->id)==0) {//forse superfluo perchè lo fa search_user_id
        printf("id corretto\n");
    } else {
        printf("id errato\n");

        printf("invio GOBYE+++\n");
        char goodbye_msg[]="GOBYE+++";
        write(sock, goodbye_msg, strlen(goodbye_msg));

        close(sock);
        return -1;
    }
    */
    printf("sto per leggere la password\n");
    uint16_t* infopass=read_password(sock);
    if(infopass==NULL)return -1;//non stampo niente perchè lo fa gia la read_...(fd) di turno
    printf("password ricevuta: %u\n", *infopass);

    if(*infopass==server.array[p]->password)
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

    //per terminare la lettura del comando
    char terminatore[3];
    int byte_letti=read(sock, terminatore, 3);
    if(byte_letti<3)
    {
        printf("errore REGIS_handler():Formato del comando non standard\n");
        write(sock, "GOBYE+++", 8); 
        close(sock);
        return-1;
    }
    //

    return p;//qua p dovrebbe contenere la posizione del cliente attuale nell'array
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
    
    //leggo id
    char* infoid=read_id(sock);
    if(infoid==NULL)return;//non stampo niente perchè lo fa gia la read_...(fd) di turno
    printf("id ricevuto: %s\n", infoid);

    //controlliamo se l'id esiste
    int friend_pos=search_user_id(infoid);

    char request_not_sent_msg[]="FRIE<+++";
    if(friend_pos==-1)//l'utente a cui vogliamo chiedere l'amicizia non esiste
    {
        printf("Utente inesistente. invio %s\n", request_not_sent_msg);
        write(sock, request_not_sent_msg, strlen(request_not_sent_msg)*sizeof(char));
        return;
    }
    else if(friend_pos==active_pos)//l'utente vuole fare amicizia con se stesso
    {
        printf("Impossibile fare amicizia con se stessi. invio %s\n", request_not_sent_msg);
        write(sock, request_not_sent_msg, strlen(request_not_sent_msg)*sizeof(char));
        return;
    }
    else if(areFriends(friend_pos, active_pos))//controlliamo se i due sono già amici
    {
        //mando FRIE<+++. nulla di grave ma non mando richiesta.
        printf("I due utenti sono già amici. invio %s\n", request_not_sent_msg);
        write(sock, request_not_sent_msg, strlen(request_not_sent_msg)*sizeof(char));
        return;

    }
    else
    {
        //mando la richiesta di amicizia e anche la notifica UDP credo
        
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
    
    free(args);
    
    //posizione del cliente attuale in server.array[]. -1 se non la conosciamo ancora. verrà impostata da CONNE
    int active_pos=-1;
    
    int buf1_size=6;
    char* buf1=malloc(buf1_size*sizeof(char));
    while(1) {
        printf("sto per leggere il comando\n");
        int val=read(info.activefd, buf1, buf1_size-1);
        printf("valore di read: %d\n", val);
        printf("ho appena letto il comando\n");
        buf1[val]='\0';
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