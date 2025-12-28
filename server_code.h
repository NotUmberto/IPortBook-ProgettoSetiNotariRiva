#include <stdbool.h>
#include <sys/socket.h>
#include "stack.h"

#define MAXCLIENTS 100

typedef struct package { 
    struct sockaddr_in caller;
    socklen_t sizecaller;
    int activefd;
    int pos;
}package;
typedef struct package* Package; //per i puntatori


typedef struct client {
    bool isConnected;
    bool flooded;//per gestire i flood. come fosse visited nei grafi
    char* id;//sarà un array di 8 char
    char* UDP_port;//<9999
    unsigned password; //tra 0 e 65535

    struct sockaddr_in address;//cosi sappiamo a chi inviare flussi e notifiche. compilato al REGIS. si presuppone che non cambi

    Stack notifies; //composta da coppie notifica-flusso

    //array degli amici grande al massimo MAXCLIENTS
    int friendNumber;//per sapere il numero di amici
    struct client* friends[MAXCLIENTS];

} client;
typedef struct client* Client; //per i puntatori

struct registered_users { //una struct per i clients
    int number; //per sapere quanti ne abbiamo
    Client array[MAXCLIENTS]; //l'array dei clienti
};

extern volatile struct registered_users server;


int readline(int sock,char *b);

void* TCP_message_handler(void*);
