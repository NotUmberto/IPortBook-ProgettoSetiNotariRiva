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

//l'idea è avere una funzione che dealloca la memoria può chiudere la connessione, può rilasciare il lock e può inviare gobye. l'ho fatta perchè è comune a moltissimi handler
void cleanup(int fd,char* infoid, char* infoport, uint16_t* infopass, char* infomess ,bool release_lock)
{
    if(fd>=0)
    {
        write(fd, "GOBYE+++", 8); 
        close(fd);
        pthread_exit(NULL);//facciamo terminare solo questo thread
    }
    if(infoid!=NULL)free(infoid);
    if(infoport!=NULL)free(infoport);
    if(infopass!=NULL)free(infopass);
    if(infomess!=NULL)free(infomess);
    if(release_lock)pthread_mutex_unlock(&lock);
}

int my_pow(int b,int e) //non ho incluso math.h perchè il linker si arrabbiava. 
{
    int ret=1;
    if(e>0)
    {
        for(int i=0;i<e;i++)
        {
            ret=ret*b;
        }
    }
    return ret;
}

//per mandare una notifica. sia se l'utente è online, sia se non lo è, immagazzino le informazioni(flusso e notifica) in una struttura dati opportuna. se l'utente target è online effettivamente gli mando la notifica, altrimenti no. inoltre alla fine del CONNE bisognerebbe mandare l'ultima notifica, se l'utente ne ha ancora da leggere. per quanto riguarda la struttura dati da usare. uno stack sarebbe perfetto perchè gestirebbe anche l'incremento e il decremento del numero delle notifiche rimanenti. magari lo implementiamo come una lista collegata semplicissima e operiamo sempre sulla testa

//flux deve terminare con \0.
void send_UDP_notify(int UDP_sock, int active_pos, int target_pos, int notify_type, char* flux)
{
    printf("A\n");
    if(notify_type<0 || notify_type>4)
    {
        printf("send_UDP_notify():notify_type illegale\n");
    }
    notifyAndFlux toSend;
    toSend.flux=NULL;
    toSend.notify=NULL;

    toSend.flux=malloc(strlen(flux)+1);
    toSend.flux[0]='\0';
    strcpy(toSend.flux, flux);

    toSend.notify=malloc(4); //[YXX]. conto anche un \0
    printf("B\n");
    //se ci ricordiamo usiamo sprintf
    toSend.notify[0]=(char)(48+notify_type); //48 in ascii è 0. 

    notifyAndFlux* top=stack_top(&(server.array[target_pos]->notifies));
    printf("C\n");

    int converted=0;
    if(top!=NULL)
    {
        for(int i=1;i<3;i++)
        {
            if(48<=top->notify[i] && top->notify[i]<=57)
            {
                converted+=(top->notify[i]-48)*my_pow(16, i-1);
            }
            else if(65<=top->notify[i] && top->notify[i]<=70)
            {
                converted+=((top->notify[i]-65)+10)*my_pow(16, i-1);
            }
        }
    }
    printf("D\n");
    if(converted<255)
    {
        converted++;
        //lo riconverto in una stringa. qua posso usare un algoritmo semplificato

        int digits[2]={converted%16 , (int)(converted/16)};
        for(int i=0;i<2;i++)
        {
            if(0<=digits[i] && digits[i]<=9)
            {
                toSend.notify[1+i]=(char)(48+digits[i]);
            }
            else
            {
                toSend.notify[1+i]=(char)((digits[i]-10)+65);
            }
        }
        toSend.notify[3]='\0';

        //ora abbiamo completato l'inizializzazione di toSend. 

        //in tutti i casi mettiamo toSend nello stack. se il cliente è online gli mandiamo una notifica udp che dovrebbe ricevere senza problemi dato che ha un thread che se ne occupa

        stack_push(&(server.array[target_pos]->notifies), toSend);

        if(server.array[target_pos]->isConnected)
        {
            //gli mando la toSend.notify
            sendto(UDP_sock, toSend.notify, strlen(toSend.notify), 0, (struct sockaddr*)&(server.array[target_pos]->address), (socklen_t)sizeof(struct sockaddr_in));

        }
    } 
    
    printf("E\n");
    
    

    
}

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

//legge +++ cosi esauriamo completamente il messaggio corrente e non rimangono rimasugli del messaggio precedente 
int read_terminator(int sock)
{
    //per terminare la lettura del comando
    char terminatore[3];
    int byte_letti=read(sock, terminatore, 3);
    if(byte_letti<3)
    {
        printf("errore read_terminator():Formato del comando non standard\n");
        
        cleanup(sock, NULL, NULL,NULL,NULL,0);
        return -1;
    }
    return 0;
}


//lista di tutti i messaggi tcp possibili
//REGIS, WELCO, GOBYE, CONNE, HELLO,FRIE?, FRIE>, FRIE<, MESS?, MESS>, MESS<, FLOO>, FLOO?, 
// LIST?, RLIST, LINUM,CONSU, SSEM>,OOLF>, EIRF>, OKIRF, NOKRF, ACKRF, FRIEN, NOFRI, NOCON
void REGIS_handler(int fd,int p, struct sockaddr_in caller) { //la sequenza di lettura e' REGIS_id_UDP-port_password
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

    if(search_user_id(infoid)!=-1)
    {
        printf("ID già in uso\n");
        cleanup(fd, infoid, NULL, NULL, NULL, true);
        return;
    }

    //printf("lunghezza di infoid -%s- %ld\n",infoid,strlen(infoid));
    newclient->id=malloc(strlen(infoid)+1); 
    strcpy(newclient->id,infoid);

    //---------lettura porta UDP--------------
    char* infoport=read_UDP_port(fd);
    if(infoport==NULL)return;//non stampo niente perchè lo fa gia la read_...(fd) di turno
    printf("port ricevuta: %s\n", infoport);

    if(atoi(infoport)<0 || atoi(infoport)>9999)
    {
        printf("Porta UDP non valida\n");
        cleanup(fd, infoid, infoport, NULL, NULL, true);
        return;
    }
    newclient->UDP_port=malloc(strlen(infoport)+1); 
    strcpy(newclient->UDP_port,infoport);
    //---------compilo address--------------
    
    newclient->address.sin_family=AF_INET;

    newclient->address=caller;

    newclient->address.sin_port=htons(atoi(infoport));//tanto caller è una copia
    
    //---------lettura password--------------
    
    uint16_t* temp=read_password(fd);

    if(temp==NULL)return;//non stampo niente perchè lo fa gia la read_...(fd) di turno
    uint16_t pass=*temp;
    free(temp);
    printf("password ricevuta: %d\n", pass);

    if (pass<0 || pass>65535) { 
        printf("password errata\n");
        cleanup(fd, infoid, infoport, NULL, NULL, true);
        free(newclient);
        return;
    }
    
    if(read_terminator(fd)==-1)return;

    newclient->password=pass; //per la password

    //---------inserimento connection--------------

    newclient->isConnected=true; //siamo sicuri di sta linea? 

    //---------aggiunta alla struct degli utenti registrati--------------
    server.array[p]=newclient;

    server.number++;

    printf("cliente aggiunto alla struct con successo!\n");

    cleanup(-1, infoid, infoport, NULL, NULL, true);

    write(fd, "WELCO+++", 8); //invio il messaggio di benvenuto
    return;
}

int CONNE_handler(int sock, int p) { 
    printf("siamo al CONNE\n");
    //CONNE ID PASS+++

    pthread_mutex_lock(&lock);

    char* infoid=read_id(sock);
    if(infoid==NULL)//non stampo niente perchè lo fa gia la read_...(fd) di turno
    {
        cleanup(sock,NULL, NULL, NULL, NULL, true);
        return -1;
    }
    printf("id ricevuto: %s\n", infoid);

    p=search_user_id(infoid);

    //caso in cui e' stato fatto il CONNE senza REGIS
    if (p==-1) { 
        printf("id non trovato\n");
        cleanup(sock,infoid, NULL, NULL, NULL, true);
        return -1;
    }

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
        cleanup(sock,infoid, NULL, infopass, NULL, true);

        return -1;
    }

    if(read_terminator(sock)==-1)
    {
        cleanup(-1, infoid, NULL, infopass, NULL,true);
        return -1;
    }

    //potremmo mandare una notifica al client se ha notifiche, ma per ora evitiamo
    

    cleanup(-1, infoid, NULL, infopass, NULL, true);
    return p;//qua p dovrebbe contenere la posizione del cliente attuale nell'array
}

void IQUIT_handler(int sock, int active_pos)
{
    pthread_mutex_lock(&lock);
    server.array[active_pos]->isConnected=false;
    cleanup(sock, NULL, NULL, NULL, NULL, true);
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

void FRIEND_REQUEST_handler(int sock, int active_pos, int UDP_sock) { 
    printf("siamo al FRIEND_REQUEST\n");
    
    
    //leggo id
    char* infoid=read_id(sock);
    if(infoid==NULL)return;//non stampo niente perchè lo fa gia la read_...(fd) di turno
    printf("id ricevuto: %s\n", infoid);

    //controlliamo se l'id esiste
    int friend_pos=search_user_id(infoid);

    //printf("A");
    char request_not_sent_msg[]="FRIE<+++";
    if(friend_pos==-1)//l'utente a cui vogliamo chiedere l'amicizia non esiste
    {
        printf("Utente inesistente. invio %s\n", request_not_sent_msg);
        write(sock, request_not_sent_msg, strlen(request_not_sent_msg)*sizeof(char));
    }
    else if(friend_pos==active_pos)//l'utente vuole fare amicizia con se stesso
    {
        printf("Impossibile fare amicizia con se stessi. invio %s\n", request_not_sent_msg);
        //send_UDP_notify(UDP_sock,active_pos, friend_pos, 0, "CIAO+++");
        write(sock, request_not_sent_msg, strlen(request_not_sent_msg)*sizeof(char));
    }
    else if(areFriends(friend_pos, active_pos))//controlliamo se i due sono già amici
    {
        //mando FRIE<+++. nulla di grave ma non mando richiesta.
        printf("I due utenti sono già amici. invio %s\n", request_not_sent_msg);
        write(sock, request_not_sent_msg, strlen(request_not_sent_msg)*sizeof(char));

    }
    else
    {
        
        //mando la richiesta di amicizia e anche la notifica UDP credo. 
        printf("Inoltro la richiesta di amicizia al destinatario. invio FRIE>+++ al mittente\n");
        write(sock, "FRIE>+++", 8);

        //EIRF> id+++
        char* flux=malloc((5+strlen(server.array[active_pos]->id)+3+1+1)*sizeof(char)); //5 per il tipo, strlen(...) dovrebbe essere sempre 8, 3 per il terminatore, 1 per lo spazio, 1 per \0

        sprintf(flux, "EIRF> %s+++", server.array[active_pos]->id);

        send_UDP_notify(UDP_sock, active_pos, friend_pos, 0, flux);

        
    }

    if(read_terminator(sock)==-1)
    {
        cleanup(-1, infoid, NULL, NULL, NULL,true);
        return;
    }
    free(infoid);
    return;
}

void MESSAGE_handler(int sock, int active_pos, int UDP_sock) { //lo fa Umberto
    printf("siamo al MESSAGE_handler\n");

    //MESS? ID MESS+++

    char* destinatario_id=read_id(sock);
    char* messaggio=read_mess(sock);
    
    printf("A\n");

    //controllo che il messaggio non superi i 200 caratteri
    /*if (strlen(messaggio)>200) { 
        printf("messaggio troppo lungo\n");
        char messaggio_non_inviato[]="MESS<+++";
        write(sock,messaggio_non_inviato,strlen(messaggio_non_inviato));
        return;
    }*/

    int destinatario_pos=search_user_id(destinatario_id); 
    
    printf("B\n");
    
    //controlliamo che non siamo noi stessi
    if (active_pos==destinatario_pos) { 
        printf("impossibile mandare messaggi a se stessi\n");
        char messaggio_non_inviato[]="MESS<+++";
        write(sock,messaggio_non_inviato,strlen(messaggio_non_inviato));
        return;
    }

    printf("C\n");

    //controlli che l'utente esista
    if (search_user_id(destinatario_id)==-1) { 
        printf("utente destinatario non esistente\n");
        char messaggio_non_inviato[]="MESS<+++";
        write(sock,messaggio_non_inviato,strlen(messaggio_non_inviato));
        return;
    }

    printf("D\n");

    //controlli che sono amici
    /*if (!areFriends(active_pos,destinatario_pos)) { 
        printf("i due utenti non sono amici\n"); 
        char messaggio_non_inviato[]="MESS<+++";
        write(sock,messaggio_non_inviato,strlen(messaggio_non_inviato));
        return;
    }*/
    //se lo sono devo aggiungere il messaggio alla struttura dati del destinatario
    //e mando la notifica 3XX

    //creo la notifica formata da: Tipo id_mittente messaggio
    char destinatario_flux[6+9+strlen(messaggio)+1]; //6 per "MESS> ", 9 per id mittente, il messaggio, 1 per \0
    
    sprintf(destinatario_flux,"MESS> %s %s\n",server.array[active_pos]->id,messaggio);
    printf("sto mandando il messaggio:\n%s\n",destinatario_flux);
    send_UDP_notify(UDP_sock, active_pos, destinatario_pos, 3, destinatario_flux);
    char messaggio_inviato[]="MESS>+++";
    write(sock,messaggio_inviato,strlen(messaggio_inviato));

    printf("E\n");

    //per debug ora stampiamo il top della pila delle notifiche del destinatario
    notifyAndFlux* temptop=stack_top(&(server.array[destinatario_pos]->notifies));
    if (temptop==NULL) { 
        printf("temptop e' NULL\n");
    } else { 
        printf("temptop->notify: %s\n",temptop->notify);
        printf("temptop->flux: %s\n",temptop->flux);
    }

    return;
}

void FLOOD_sender(int sender_pos,int friend_pos, int UDP_sock, char* messaggio) { 
    //scorro tutta la lista degli amici di friend_pos
    for (unsigned i=0;i<server.array[friend_pos]->friendNumber;i++) { 
        int dest_pos=-1;
        dest_pos=search_user_id(server.array[friend_pos]->friends[i]->id);
        if (dest_pos==-1) { 
            printf("utente amico non trovato nel server\n");
        }

        if (server.array[dest_pos]->flooded==false) { 
            server.array[dest_pos]->flooded=true;
            //creo la notifica formata da: Tipo id_mittente messaggio
            char flood_flux[6+9+strlen(messaggio)+1]; //6 per "FLOO> ", 9 per id mittente, il messaggio, 1 per \0
            sprintf(flood_flux,"FLOO> %s %s\n",server.array[sender_pos]->id,messaggio);
            printf("sto mandando il flood:\n%s\n",flood_flux);
            //richiamo la funzione su dest_pos
            if (dest_pos!=sender_pos) { 
                send_UDP_notify(UDP_sock, sender_pos, dest_pos, 4, flood_flux);
                FLOOD_sender(sender_pos, dest_pos, UDP_sock, messaggio);
            } else {
                //per DEBUG 
                printf("evito di richiamare FLOOD_sender su se stesso\n");
            }
        }
    }
}

void FLOOD_handler(int sock, int active_pos, int UDP_sock) { //lo fa Umberto
    printf("siamo al FLOOD_handler\n");
    //FLOO? mess+++

    if (server.array[active_pos]->friendNumber==0) { 
        printf("utente senza amici :(\n");
        return;
    }

    //prendiamo il messaggio
    char* messaggio=read_mess(sock);
    printf("messaggio di flood ricevuto: %s\n", messaggio);

    //chiamiamo una funzione che scorra la lista e si richiami
    FLOOD_sender(active_pos, active_pos, UDP_sock, messaggio); //alla prima chiamata il flood avviene su se stessi
    //non usiamo la stessa funzione FLOOD_handler

    //inviamo FLOO>+++ al mittente
    char flood_inviato[]="FLOO>+++";
    write(sock,flood_inviato,strlen(flood_inviato));

    //azzeriamo il bit flood di tutti
    //scorro tutto il server e li azzero tutti
    for (unsigned i=0;i<server.number;i++) { 
        server.array[i]->flooded=false;
    }
    return;
}

void LIST_handler(int sock)
{
    printf("A\n");
    pthread_mutex_lock(&lock);
    //prima mandiamo RLIST num-item+++ num-item è sempre 3 caratteri
    char* RLIST_num_item=malloc(13*sizeof(char));//5 per tipo, uno per spazio, 3 per num-item, 3 per il terminatore, uno per \0
    char* num_item=malloc(4*sizeof(char));
    char* LINUM=malloc(18*sizeof(char));//5 per il tipo, uno per lo spazio, 8 per l'id, 3 per il temrinatore, uno per \0
    if(RLIST_num_item==NULL || num_item==NULL || LINUM==NULL)
    {
        printf("LIST_handler: Problema di allocazione memoria");
        if(RLIST_num_item!=NULL)free(RLIST_num_item);
        if(num_item!=NULL)free(num_item);
        if(LINUM!=NULL)free(LINUM);
        cleanup(sock, NULL,NULL,NULL,NULL, true);
        return;
    }
    
    sprintf(num_item,"%d", server.number);
    if(strlen(num_item)==1)
    {
        sprintf(num_item, "00%d", server.number);
    }
    else if(strlen(num_item)==2)
    {
        sprintf(num_item, "0%d", server.number);
    }
    sprintf(RLIST_num_item,"RLIST %s+++", num_item);
    write(sock, RLIST_num_item, strlen(RLIST_num_item));
    
    for(int i=0;i<server.number;i++)
    {
        //invio LINUM id+++
        sprintf(LINUM,"LINUM %s+++", server.array[i]->id);
        write(sock, LINUM, strlen(LINUM));
        printf("mandato %s\n", LINUM);
    }
    free(LINUM);
    free(RLIST_num_item);
    free(num_item);
    if(read_terminator(sock)==-1)
    {
        cleanup(-1, NULL, NULL, NULL, NULL,true);
        return;
    }
    pthread_mutex_unlock(&lock);
    printf("B\n");
    return;
}

void CONSU_handler(int sock, int active_pos, int UDP_sock) { 
    printf("siamo al CONSU_handler\n");
    //CONSU+++
    
    //dobbiamo controllare se ha scritto i 3 +++ alla fine. lo facciamo con read_terminator
    if(read_terminator(sock)==-1) { 
        printf("errore nel leggere il terminatore\n");
        cleanup(-1, NULL, NULL, NULL, NULL,true);
        return;
    }

    notifyAndFlux* top=stack_pop(&(server.array[active_pos]->notifies));
    
    //se non ci sono notifiche
    if (top==NULL) { 
        printf("nessuna notifica da leggere\n");
        char no_notification_msg[]="NOCON+++";
        write(sock,no_notification_msg,strlen(no_notification_msg));
        return;
    }    
    
    printf("notifica prelevata dallo stack\n");
    printf("top->notify: %s\n",top->notify);
    printf("top->flux: %s\n",top->flux);

    //se proviene da un messaggio
    //SSEM> id mess+++
    if (top->notify[0]=='3') {
        printf("notifica di tipo MESS\n");
        //la notifica di tipo messaggio arriva come: MESS> id mess+++

        //leggiamo le varie parti del flusso
        //sposto le prime 6 posizioni da top->flux (MESS> )
        char prefix[7];
        strncpy(prefix, top->flux, 6);
        prefix[6]='\0';
        
        char consu_id[9];
        strncpy(consu_id, top->flux+6, 8);
        consu_id[8]='\0';
        printf("id del mittente del messaggio: %s\n", consu_id);

        char *consu_msg=malloc((strlen(top->flux)-6-8+1)); //tot-id-MESS> +1 per \0
        strcpy(consu_msg, top->flux+6+8);
        printf("messaggio da consegnare: %s\n", consu_msg);
        //non controllo che il messaggio superi i 200 caratteri perche' tanto lo abbiamo gia' fatto in read_mess()

        char ssem_msg[6+8+strlen(consu_msg)+3+1]; //6 per SSEM> , 8 per id, il messaggio, 3 per +++, 1 per \0

        sprintf(ssem_msg,"SSEM> %s %s+++", consu_id, consu_msg);
        printf("sto per mandare al client il messaggio:\n%s\n", ssem_msg);
        write(sock, ssem_msg, strlen(ssem_msg));
    }


    //se proviene da un flood
    //OOLF> id mess+++
    else if (top->notify[0]=='4') { 
        printf("notifica di tipo FLOOD\n");
        //la notifica di tipo flood arriva come: FLOO> id mess+++
        //leggiamo le varie parti del flusso
        //sposto le prime 6 posizioni da top->flux FLOO>
        char prefix[7];
        strncpy(prefix, top->flux, 6);
        prefix[6]='\0';

        char consu_id[9];
        strncpy(consu_id, top->flux+6, 8);
        consu_id[8]='\0';
        printf("id del mittente del flood: %s\n", consu_id);

        //puntatore perche la lunghezza del messaggio e' variabile
        char *consu_msg=malloc((strlen(top->flux)-6-8+1)); //tot-id-MESS> +1 per \0
        strcpy(consu_msg, top->flux+6+8);
        printf("messaggio di flood da consegnare: %s\n", consu_msg);
        //non controllo che il messaggio superi i 200 caratteri perche' tanto lo abbiamo gia' fatto in read_mess()

        char oolf_msg[6+8+strlen(consu_msg)+3+1];
        sprintf(oolf_msg,"OOLF> %s %s+++", consu_id, consu_msg);
        printf("sto per mandare al client il messaggio di flood:\n%s\n", oolf_msg);
        write(sock, oolf_msg, strlen(oolf_msg));
        return;
    }


    //se proviene da una richiesta di amicizia
    //EIRF> id+++
    else if (top->notify[0]=='0') { 
        printf("notifica di tipo FRIEND REQUEST\n");
        //nota: secondo me la notifica a chi dovrebbe arrivare non dovrebbe arrivare come EIRF> id+++, ma come FRIEN id+++. 
        //perche' EIRF dovrebbe mandarlo il server a chi consulta

        char prefix[7];
        strncpy(prefix, top->flux, 6);
        prefix[6]='\0';

        char consu_id[9];
        strncpy(consu_id, top->flux+6, 8);
        consu_id[8]='\0';
        printf("id del mittente della richiesta di amicizia: %s\n", consu_id);

        char eirf_flux[6+8+3+1]; //6 per EIRF> , 8 per id, 3 per +++, 1 per \0
        sprintf(eirf_flux,"EIRF> %s+++", consu_id);
        printf("sto per mandare al client la richiesta di amicizia:\n%s\n", eirf_flux);
        write(sock, eirf_flux, strlen(eirf_flux));

        //adesso leggiamo se abbiamo ricevuto OKIRF o NOKRF
        char response_buf[9]; //8 per OKIRF/NOKRF + 1 per \0
        int val=read(sock, response_buf, 9); //OKIRF+++ o NOKRF+++ e poi il terminatore
        response_buf[val]='\0';
        printf("risposta ricevuta dal client: %s\n", response_buf);
        
        if (strcmp(response_buf, "OKIRF+++")==0) { 
            //rispondiamo con ACKRF+++
            //aggiungiamo l'amico alla lista amici di entrambi
            //mandiamo la notifica UDP 1XX a chi ha inviato la richiesta di amicizia
            char ackrf_msg[]="ACKRF+++";
            write(sock, ackrf_msg, strlen(ackrf_msg));

            int friend_pos=search_user_id(consu_id);
            addfriend(active_pos, friend_pos);
            addfriend(friend_pos, active_pos);

            char friend_flux[6+8+1]; //6 per FRIEN, 8 per id, 1 per \0
            sprintf(friend_flux, "FRIEN %s", server.array[active_pos]->id);
            send_UDP_notify(UDP_sock, active_pos, friend_pos, 1, friend_flux); 
            //la consegna non dice nulla sul flusso. ci mettiamo l'id di chi ha accettato l'amicizia 
        } 
        else if (strcmp(response_buf, "NOKRF+++")==0) { 
            //rispondiamo con ACKRF+++
            //non aggiungiamo l'amico alla lista amici di entrambi
            //mandiamo la notifica UDP 2XX a chi ha inviato la richiesta di amicizia
            char ackrf_msg[]="ACKRF+++";
            write(sock, ackrf_msg, strlen(ackrf_msg));

            int friend_pos=search_user_id(consu_id);
            char nofriend_flux[6+8+1]; //6 per NOFRI, 8 per id, 1 per \0
            sprintf(nofriend_flux, "NOFRI %s", server.array[active_pos]->id);
            send_UDP_notify(UDP_sock, active_pos, friend_pos, 2, nofriend_flux);
            //la consegna non dice nulla sul flusso. ci mettiamo l'id di chi ha rifiutato l'amicizia
        } else { 
            printf("risposta sconosciuta alla richiesta di amicizia\n");
            return;
        }

        return; 
    }

    //se si tratta di una accettazione o rifiuto di una richiesta di amicizia
    else if (top->notify[0]=='1' || top->notify[0]=='2') { 
        printf("notifica di tipo FRIEND RESPONSE\n");

        //la notifica arriva come FRIEN id+++ o NOFRI id+++

        //se e' 1XX allora e' un OKIRF e mandiamo FRIEN id+++
        if (top->notify[0]=='1') { 
            char prefix[7];
            strncpy(prefix, top->flux, 6);
            prefix[6]='\0';

            char consu_id[9];
            strncpy(consu_id, top->flux+6, 8);
            consu_id[8]='\0';
            printf("id del mittente della risposta di amicizia: %s\n", consu_id);

            char frien_flux[6+8+3+1]; //6 per FRIEN , 8 per id, 3 per +++, 1 per \0
            sprintf(frien_flux,"FRIEN %s+++", consu_id);
            write(sock, frien_flux, strlen(frien_flux));
            printf("mandata FRIEN al client\n");
            return;
        } else { 
            //se e' 2XX allora e' un NOKRF e mandiamo NOFRI id+++
            char prefix[7];
            strncpy(prefix, top->flux, 6);
            prefix[6]='\0';

            char consu_id[9];
            strncpy(consu_id, top->flux+6, 8);
            consu_id[8]='\0';
            printf("id del mittente della risposta di amicizia: %s\n", consu_id);

            char nofri_flux[6+8+3+1]; //6 per NOFRI , 8 per id, 3 per +++, 1 per \0
            sprintf(nofri_flux,"NOFRI %s+++",consu_id);
            write(sock, nofri_flux, strlen(nofri_flux));
            printf("mandata NOFRI al client\n");
            return;
        }
    } else { 
        printf("notifica di tipo sconosciuto\n");
        return; 
    }
}

//gestisce il cliente a seconda di quello che ha mandato. gli args dovranno includere almeno il sockfd per comunicare in tcp col cliente. probabilmente andranno anche inclusi i campi del chiamante compilati da accept() cosi da usarli in REGIS_handler() direi.
void* TCP_message_handler(void* args) {
    struct package info=*((struct package*)args);
    free(args);

    //posizione del cliente attuale in server.array[]. -1 se non la conosciamo ancora. verrà impostata da CONNE
    int active_pos=-1;
    int UDP_sock=socket(AF_INET, SOCK_DGRAM, 0);//non faccio bind perchè qua il server non riceve in udp
    //ora possiamo mandare con sendto(). possiamo ottenere la sockaddr giusta unendo caller e la udp port contenuta nella struct del cliente


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
            REGIS_handler(info.activefd,info.pos, info.caller);
        }
        else if(strcmp(buf1, "CONNE")==0 )
        {
            //UDP_sock verrà compilato qua
            active_pos=CONNE_handler(info.activefd,info.pos);
        }
        else if(strcmp(buf1, "FRIE?")==0 )
        {
            FRIEND_REQUEST_handler(info.activefd, active_pos, UDP_sock);
        }
        else if(strcmp(buf1, "MESS?")==0)
        {
            MESSAGE_handler(info.activefd, active_pos, UDP_sock);
        }
        else if(strcmp(buf1, "FLOO?")==0)
        {
            FLOOD_handler(info.activefd, active_pos, UDP_sock);
        }
        else if(strcmp(buf1, "LIST?")==0)
        {
            LIST_handler(info.activefd);
        }
        else if(strcmp(buf1, "CONSU")==0)
        {
            CONSU_handler(info.activefd, active_pos, UDP_sock);

            //OKIRF e NOKRF penso debbano essere gestite a stretto contatto con CONSU. potremmo forse gestire questi 2 casi direttamente nel CONSU, obbligando l'utente a dire si o no ad una richiesta di amicizia, una volta che l'ha letta con CONSU. 
            //oppure si potrebbe memorizzare l'ultimo flusso letto(letteralmente salvarsi stack_pop()->flux). in questo modo si potrebbe dare all'utente la scelta di ignorare un flusso(come ad esempio una richiesta di amicizia). sarei piu propenso ad implementare quest'ultimo approccio

            //scusa ma ignorando un flusso come faccio poi a mandare OKIRF o NOKRF? non starei perdendo la possibilità di rispondere a quella richiesta? sulla consegna c'e' scritto che una volta cancellato dal consu il flusso e' perso. secondo me bisognerebbe togliere OKIRF e NOKRF e far gestire tutto a CONSU subito dopo aver letto una notifica
        }
        else if(strcmp(buf1, "OKIRF")==0)
        {
            
        }
        else if(strcmp(buf1, "NOKRF")==0)
        {
            
        }
        else if(strcmp(buf1, "IQUIT")==0)
        {
            IQUIT_handler(info.activefd, active_pos);
            return NULL;
        }
        else 
        {
            printf("comando non riconosciuto\n");
            
            cleanup(info.activefd, NULL,NULL,NULL,NULL, 0);
            return NULL;
        }
    }
    //qua e' finito il while (non dovrebbe mai finire)
    return NULL;
}