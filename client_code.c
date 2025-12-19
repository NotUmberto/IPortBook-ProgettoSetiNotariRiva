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

TCP_message_handler(int sockfd)
{
    int buf1_size=6;
    char* buf1=malloc(buf1_size*sizeof(char));
    while(1)
    {
        read(sockfd, buf1, buf1_size-1);
        buf1[buf1_size-1]='\0';
        
        if(strcmp(buf1, "WELCO")==0)
        {
            WELCO_handler();
        }
        else if(strcmp(buf1, "GOBYE")==0)
        {
            GOBYE_handler();
        }
        //altri casi...
        else 
        {
            //messaggio sconosciuto
        }
        
        
    }
}