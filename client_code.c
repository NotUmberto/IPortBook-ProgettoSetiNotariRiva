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
        }
        else if(strcmp(buf1, "OOLF>")==0)
        {
        }
        else if(strcmp(buf1, "EIRF>")==0)
        {
        }
        else if(strcmp(buf1, "ACKRF")==0)
        {
        }
        else if(strcmp(buf1, "FRIEN")==0)
        {
        }
        else if(strcmp(buf1, "NOFRI")==0)
        {
        }
        else if(strcmp(buf1, "NOCON")==0)
        {
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