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



//legge +++ cosi esauriamo completamente il messaggio corrente e non rimangono rimasugli del messaggio precedente 
int read_terminator(int sock)
{
    //per terminare la lettura del comando
    char terminatore[3];
    int byte_letti=read(sock, terminatore, 3);
    if(byte_letti<3)
    {
        printf("errore read_terminator():Formato del comando non standard\n");
        return -1;
    }
    return 0;
}

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

//presa da server_code.c
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

void WELCO_handler() { 
    printf("Benvenuto! la connessione ha avuto successo\n");
    
    return;
}

void GOBYE_handler() { 
    printf("Hai ricevuto un GOBYE\nIl server ha chiuso la connessione\n");
    return;
}

void HELLO_handler() { 
    printf("Bentornato! La connessione ha avuto successo!\n");
    return;
}

void MESS_handler() { 
    printf("Messaggio consegnato con successo\n");
    return;
}

void NO_MESS_handler() { 
    printf("Messaggio non consegnato\n");
    return;
}

void FLOOD_handler() { 
    printf("Flood consegnato con successo\n");
    return;
}

void RLIST_handler(int sock)
{
  char* num=malloc(4*sizeof(char));
  char spazio='a';
  read(sock, &spazio,1);
  if(spazio!=' ')
  {
    printf("RLIST_handler: comando non standard\n");
  }
  read(sock, num, 3);
  printf("numero di utenti registrati nel server: %s\n", num);
  free(num);
  return;
}

void LINUM_handler(int sock)
{
  char* infoid=read_id(sock);
  printf("%s\n",infoid);
  free(infoid);
  return;
}

void NOCON_handler() { 
    printf("Nessuna notifica da leggere\n");
    return;
}

void SSEM_handler(int sock) { 
  //leggo SSEM> id mess+++
  //SSEM> e' gia' stato letto nel TCP_message_handler
    printf("Hai ricevuto un messaggio!\n");
    char* infoid=read_id(sock);
    char* messaggio=read_mess(sock);
    //i +++ sono gia' stati letti in read_mess()
    printf("Mittente: %s\nMessaggio: %s\n", infoid, messaggio);
    return;
}

void OOLF_handler(int sock) { 
  //leggo OOLF> id mess+++
  //OOLF> e' gia' stato letto nel TCP_message_handler
    printf("Hai ricevuto un messaggio di flood!\n");
    char* infoid=read_id(sock);
    char* messaggio=read_mess(sock);
    //i +++ sono gia' stati letti in read_mess()
    printf("Mittente flood: %s\nMessaggio di flood: %s\n", infoid, messaggio);
    return;
}

void EIRF_handler(int sock) { 
    char* infoid=read_id(sock);
    read_terminator(sock);
    
    printf("Hai ricevuto una richiesta di amicizia dall'utente: %s\n", infoid);
    printf("Per accettare la richiesta, invia OKIRF+++\n");
    printf("Per rifiutare la richiesta, invia NOKRF+++\n");

    //leggiamo ora se l'utente accetta o rifiuta
    char response[9]; //8 per OKIRF+++/NOKRF+++, 1 per \0
    fgets(response, 9, stdin);
    response[8]='\0';

    //inviamo la risposta al server
    write(sock, response, strlen(response));

    //ci aspettiamo ACKRF+++ come conferma
    char ack_buf[9]; //8 per ACKRF+++, 1 per \0
    read(sock, ack_buf, 9);
    ack_buf[8]='\0';
    if (strcmp(ack_buf, "ACKRF+++")==0) { 
      printf("Risposta alla richiesta di amicizia inviata con successo\n");
    } else { 
      printf("Errore nell'invio della risposta alla richiesta di amicizia\n");
    }
    return;
}

void FRIEN_handler(int sock) { 
    //FRIEN id+++
    char infoid[9];
    read(sock, infoid, 8);
    infoid[8]='\0';
    read_terminator(sock);

    printf("L'utente %s ha accettato la tua richiesta di amicizia\n", infoid);
    return;
}

void NOFRI_handler(int sock) { 
    //NOFRI id+++
    char infoid[9];
    read(sock, infoid, 8);
    infoid[8]='\0';
    read_terminator(sock);

    printf("L'utente %s ha rifiutato la tua richiesta di amicizia\n", infoid);
    return;
}

void TCP_message_handler(int sockfd)
{
    //i 3 + vanno letti a parte alla fine di ogni handler. leggerli all'inizio ha dato problemi con LIST_handler
    int buf1_size=6; //5 per il tipo di comando, 1 per \0
    char* buf1=malloc((buf1_size+1)*sizeof(char));
    buf1[0]='\0'; 

    
        printf("sto per leggere il messaggio\n");
        int sizeread=read(sockfd, buf1, buf1_size-1);
        buf1[buf1_size-1]='\0';
        printf("messaggio ricevuto: %s\n", buf1);

        /*if (sizeread<7) { //messaggio troppo corto per essere valido
          printf("comando troppo corto, chiudo la connessione\n");
          close(sockfd);
          return;
        }*/
        
        if(strcmp(buf1, "WELCO")==0)
        {
            WELCO_handler();
        }
        else if(strcmp(buf1, "GOBYE")==0)
        {
            GOBYE_handler();
            exit(0);
        }
        else if(strcmp(buf1, "HELLO")==0)
        {
            HELLO_handler();
        }
        else if(strcmp(buf1, "FRIE>")==0)
        {
          printf("Richiesta di amicizia mandata\n");
        }
        else if(strcmp(buf1, "FRIE<")==0)
        {
            printf("Richiesta di amicizia non mandata\n");
        }
        else if(strcmp(buf1, "MESS>")==0)
        {
          MESS_handler(); //purtroppo non possiamo usare > nel nome della funzione
        }
        else if(strcmp(buf1, "MESS<")==0)
        {
          NO_MESS_handler(); //purtroppo non possiamo usare < nel nome della funzione
        }
        else if(strcmp(buf1, "FLOO>")==0)
        {
          FLOOD_handler();
        }
        else if(strcmp(buf1, "RLIST")==0)
        {
          RLIST_handler(sockfd);
        }
        else if(strcmp(buf1, "LINUM")==0)
        {
          LINUM_handler(sockfd);
        }
        else if(strcmp(buf1, "SSEM>")==0)
        {
          SSEM_handler(sockfd);
          return; //questo return perche' va in conflitto con l'if alla fine
        }
        else if(strcmp(buf1, "OOLF>")==0)
        {
          OOLF_handler(sockfd);
          return;
        }
        else if(strcmp(buf1, "EIRF>")==0)
        {
          EIRF_handler(sockfd);
          return;
        }
        else if(strcmp(buf1, "ACKRF")==0)
        {
          //DA TOGLIERE (non arrivera' mai qui perche' ACKRF viene gestito dentro EIRF_handler)
        }
        else if(strcmp(buf1, "FRIEN")==0)
        {
        }
        else if(strcmp(buf1, "NOFRI")==0)
        {
        }
        else if(strcmp(buf1, "NOCON")==0)
        {
          NOCON_handler();
        }
        else 
        {
            printf("messaggio sconosciuto\n");
            close(sockfd);
            return;
        }
        

        if(read_terminator(sockfd)==-1)
        {
            printf("Comando non standard\n");
            close(sockfd);
            return;
        }

        return;
}

void *ListenNotify(void *port) {
    int udp_port=*((int*)port);
  // creazione socket UDP per ascoltare le notifiche
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  struct sockaddr_in address_sock;
  address_sock.sin_family = AF_INET;
  address_sock.sin_port = htons(udp_port);
  address_sock.sin_addr.s_addr = htonl(INADDR_ANY);
  int b = bind(sock, (struct sockaddr *)&address_sock, sizeof(struct sockaddr_in));

  if (b == 0) {
    // leggiamo le notifiche
    while (1) {
      char mex[4];
      int size = recv(sock, mex, 4, 0);
      if (size < 3)
        printf("ERRORE size=%d\n", size);
      mex[size] = '\0';
      printf("Nuova notifica: %s\n", mex);
    }
  }
  close(sock);
  return NULL;
}