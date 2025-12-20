#include <stdbool.h>
#include <sys/socket.h>

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
    char* id;//sarà un array di 8 char
    char* UDP_port;//<9999
    int password; //tra 0 e 65535
} client;
typedef struct client* Client; //per i puntatori

struct registered_users { //una struct per i clients
    int number; //per sapere quanti ne abbiamo
    Client array[MAXCLIENTS]; //l'array dei clienti
};

extern volatile struct registered_users server;


int readline(int sock,char *b);

void* TCP_message_handler(void*);
void REGIS_handler(int fd,int p);
void CONNE_handler(int sock, int p);
void FRIEND_REQUEST_handler();
void MESSAGE_HANDLER();
//ecc ecc