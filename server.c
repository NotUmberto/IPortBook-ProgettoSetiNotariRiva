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

typedef struct user {
  char *name;
  char *ip;
  // aggiungere gli altri campi
} User;

typedef struct addrmex {
  char *address; // l'indirizzo del cliente a cui mandare la notifica
  char ntf[4];   // la notifica
} Package;

void *SendNotify(void *p) { // per inviare le notifiche UDP ai clienti

  // i thread richiedono parametri di tipo void facciamo quindi un cast per
  // trasformarlo in tipo Package
  Package *pkg = (Package *)p;

  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  struct addrinfo *ainfo;
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  int r = getaddrinfo(pkg->address, "5555", &hints, &ainfo);

  if (r == 0) {
    struct sockaddr *saddr = ainfo->ai_addr;

    char mex[5];
    sprintf(mex, "%s", pkg->ntf);
    sendto(sock, mex, strlen(mex), 0, saddr,
           (socklen_t)sizeof(struct sockaddr_in));
  }
  close(sock);
  return NULL;
}

int main() {

  pthread_t notifiche;
  Package *prova = malloc(sizeof(Package));
  prova->address = "localhost";
  sprintf(prova->ntf, "123");
  pthread_create(&notifiche, NULL, SendNotify, prova);

  pthread_join(notifiche, NULL);

  // inizio collegamento tcp...
}