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

void TCP_message_handler(int sockfd)
{
    int buf1_size=9; //vanno letti anche i 3+ altrimenti te li ritrovi nel prossimo messaggio
    char* buf1=malloc(buf1_size*sizeof(char));
    buf1[buf1_size-1]='\0'; 

    
        printf("sto per leggere il messaggio\n");
        int sizeread=read(sockfd, buf1, buf1_size-1);
        buf1[buf1_size-1]='\0';
        printf("messaggio ricevuto: %s\n", buf1);

        /*if (sizeread<7) { //messaggio troppo corto per essere valido
          printf("comando troppo corto, chiudo la connessione\n");
          close(sockfd);
          return;
        }*/
        
        if(strcmp(buf1, "WELCO+++")==0)
        {
            WELCO_handler();
        }
        else if(strcmp(buf1, "GOBYE+++")==0)
        {
            GOBYE_handler();
            exit(0);
        }
        else if(strcmp(buf1, "HELLO+++")==0)
        {
            HELLO_handler();
        }
        else if(strcmp(buf1, "FRIE>+++")==0)
        {
        }
        else if(strcmp(buf1, "FRIE<+++")==0)
        {
            printf("Richiesta non mandata\n");
        }
        else if(strcmp(buf1, "MESS>+++")==0)
        {
        }
        else if(strcmp(buf1, "MESS<+++")==0)
        {
        }
        else if(strcmp(buf1, "FLOO>+++")==0)
        {
        }
        else if(strcmp(buf1, "RLIST+++")==0)
        {
        }
        else if(strcmp(buf1, "LINUM+++")==0)
        {
        }
        else if(strcmp(buf1, "SSEM>+++")==0)
        {
        }
        else if(strcmp(buf1, "OOLF>+++")==0)
        {
        }
        else if(strcmp(buf1, "EIRF>+++")==0)
        {
        }
        else if(strcmp(buf1, "ACKRF+++")==0)
        {
        }
        else if(strcmp(buf1, "FRIEN+++")==0)
        {
        }
        else if(strcmp(buf1, "NOFRI+++")==0)
        {
        }
        else if(strcmp(buf1, "NOCON+++")==0)
        {
        }
        else 
        {
            printf("messaggio sconosciuto\n");
            close(sockfd);
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
      printf("Ricevuto: %s\n", mex);
    }
  }
  close(sock);
  return NULL;
}