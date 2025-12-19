//array statico di 100 clienti dato che non ce ne possono essere di piu, pare. 
//puntatori a struct perchè penso in questo modo sappiamo se un cliente è registrato se la sua
//cella non è null. dovrebbe esserci anche un risparmio di memoria. 
int const MAXCLIENTS=100;
//volatile struct Client registered_users_array[maxClients]; quello che c'era prima\

typedef struct client {
    bool isConnected;
    char* id;//sarà un array di 8 char
    char* UDP_port;//<9999

    // numero compreso tra 0 e 65535. char perchè mi sembra che in certi messaggi 
    // tcp vada mandata la password. se la memorizziamo già come string forse è piu comodo
    char* password;
} client;
typedef struct client* Client; //per i puntatori

struct registered_users { //una struct per i clients
    int number; //per sapere quanti ne abbiamo
    Client array[MAXCLIENTS]; //l'array dei clienti
};

void TCP_message_handler(void*);
void REGIS_handler();
void CONNE_handler();
void FRIEND_REQUEST_handler();
//ecc ecc