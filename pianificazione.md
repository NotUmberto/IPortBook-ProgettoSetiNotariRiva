**Come potremmo dividere il lavoro**
penso sarebbe bello creare 2 main iniziali(uno per il server, l'altro per il client), un 
impalcatura su cui costruire. ogni main si servirà di un numero di funzioni ausiliarie(handler).L'idea sarebbe che ognuno avrà delle funzioni da programmare. sarebbe bello dividere equamente le
funzioni del server e quelle del client. in questo modo ognuno si occupa di entrambe le
entità. idealmente questo approccio dovrebbe favorirci all'orale perche il rischio di non
saper spiegare una parte che non abbiamo programmato in prima persona si riduce. 


**Spiegazione approccio**

**Rappresentare le informazioni di un cliente nel server**

propongo questa struct 

struct client
{
    //int perchè il c non ha un bool vero e proprio
    int isConnected;

    char* id;//sarà un array di 8 char
    char* UDP_port;//<9999

    // numero compreso tra 0 e 65535. char perchè mi sembra che in certi messaggi 
    // tcp vada mandata la password. se la memorizziamo già come string forse è piu comodo
    char* password;

    //gestione delle notifiche, magari con delle queue implementate nel modo che ci sembra piu semplice(liste )
    queue TCP_fluxes;
    queue UDP_notifies;

}

**Commento sugli handler**

sia il server che il ciente avranno un loro gestore di messaggi tcp, formato da dei sottogestori(specifici per singolo comando). il gestore principale legge i primi 5 caratteri dal socket tcp cosi capisce di che comando si tratta e quale sottogestore chiamare. ogni sottogestore farà quacosa di diverso a seconda del comando per cui è fatto, ad esempio il sottogestore di CONNE farà cose tipo: guarderà se un cliente è già registrato, se lo è "lo attiva" così potrà interagire con gli altri clienti, se non è registrato allora il server termina tutto. possiamo spartirci questi handler per dividere il lavoro.

**gestire i messaggi di flood**

per gestire il flood penso sia utile usare un grafo, magari implementato con una matrice di adiacenza(che dovrebbe essere relativamente facile). il flood sarà una semplice visita del grafo. per evitare di congestionare la rete con messaggi infiniti, è bene che ogni nodo abbia un bit visited. 

