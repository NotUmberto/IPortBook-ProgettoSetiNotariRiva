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
    bool flood;//per gestire i flood. come fosse visited nei grafi
    char* id;//sarà un array di 8 char
    char* UDP_port;//<9999
    unsigned password; //tra 0 e 65535

    //servirebbe un campo ip.(serve a quanto pare per mandare notifiche udp anche se non si ha fatto conne) questo perchè il prof sul forum ha detto che si presuppone che il cliente si colleghi sempre dallo stesso ip. questo comporta cambiamenti di REGIS e CONNE. oppure potremmo conservare le notifiche udp da mandare e mandarle quando il cliente si connette in tcp, senza bisogno di salvare l'ip. in quest'ultimo caso mandiamo le notifiche indipendentemente dall'ip del client. se si è connesso correttamente tramite conne assumo che sia lui. userei quest'ultima opzione.

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
void REGIS_handler(int fd,int p);
int CONNE_handler(int sock, int p);
void FRIEND_REQUEST_handler();
void MESSAGE_HANDLER();
//ecc ecc