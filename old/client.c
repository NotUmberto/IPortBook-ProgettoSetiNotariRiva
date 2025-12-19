#define _POSIX_C_SOURCE 200112L // da togliere prima di presentare
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void *ListenNotify(void *) {
  // creazione socket UDP per ascoltare le notifiche
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  struct sockaddr_in address_sock;
  address_sock.sin_family = AF_INET;
  address_sock.sin_port = htons(5555);
  address_sock.sin_addr.s_addr = htonl(INADDR_ANY);
  int b =
      bind(sock, (struct sockaddr *)&address_sock, sizeof(struct sockaddr_in));

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

int main() {
  pthread_t notifiche;
  pthread_create(&notifiche, NULL, ListenNotify, NULL);
  printf("ho creato il thread\n");

  // inizio collegamento tcp...

  struct in_addr address;
  inet_aton("localhost", &address);

  int socktcp = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in sockaddr;
  sockaddr.sin_addr = address;
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_port = htons(5555);

  int c = connect(socktcp, (struct sockaddr *)&sockaddr,
                  sizeof(struct sockaddr_in));
  if (c != 0) {
    printf("ERRORE connect\n");
    exit(0);
  }

  char first[9];
  printf("sto per leggere il welcome\n");
  int s = read(socktcp, first, 8);
  first[s] = '\0';
  printf("l'ho letto\n");

  if (s < 8) {
    printf("ERRORE messaggio incompatibile\n");
    close(socktcp);
    exit(0); // se abbiamo ricevuto un messaggio strano (es. numero di caratteri
             // minore) chiudiamo tutto
  }

  if (strcmp(first, "WELCO+++") ==
      1) { // se abbiamo ricevuto il benvenuto possiamo continuare
    printf("ERRORE niente welcome\n");
    close(socktcp);
    exit(0);
  }

  if (strcmp(first, "GOBYE+++") ==
      0) { // se abbiamo ricevuto goodbye chiudiamo tutto (per il momento questo
           // if non verra mai eseguito)
    printf("ERRORE bye bye\n");
    close(socktcp);
    exit(0);
  }

  // da qui in poi si scriveranno tutti i messaggi...

  printf("Quale attivita' si desidera effettuare?\n");
  printf("CONSU - vedere tutti i comandi\n");
  printf("...");

  pthread_join(
      notifiche,
      NULL); // questo rimarra sempre alla fine perche altrimenti finche non
             // finisce il ricevimento delle notifiche il codice non inizia
}