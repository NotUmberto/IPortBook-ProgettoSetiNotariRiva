#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stack.h"

//mette element nello stack
void stack_push(Stack* s, notifyAndFlux element)
{

    if(*s==NULL)//lista vuota
    {
        *s=malloc(sizeof(struct cell));
        (*s)->payload=element;
        (*s)->next=NULL;
    }
    else //lista con almeno un elemento
    {
        Stack temp=malloc(sizeof(struct cell));
        temp->payload=element;
        temp->next=*s;
        *s=temp;
    }
}
//ritorna NULL se lo stack è vuoto, altrimenti alloca un notify 
notifyAndFlux* stack_pop(Stack* s)
{
    notifyAndFlux* ret;
    ret=NULL;
    if(*s!=NULL)
    {
        ret=malloc(sizeof(notifyAndFlux));
        *ret=(*s)->payload;
        Stack temp=*s;
        *s=(*s)->next;
        free(temp);
    }
    return ret;
}

