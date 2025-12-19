int const maxTCPMessageLength=218;

//una struct per memorizzare le info dei clienti
struct user
{
    //potremmo usare bool per rendere piu facile la lettura del codice
    //ex: if (isConnected) ...
    bool isConnected; 

    char* id;//sarà un array di 8 char
    char* UDP_port;//<9999
    char* password;// numero compreso tra 0 e 65535
};

void TCP_message_handler(int sockfd);

void *ListenNotify(void *port);