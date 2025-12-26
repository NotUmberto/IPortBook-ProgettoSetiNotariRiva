
typedef struct notifyAndFlux
{
    char* notify;// YXX\0
    char* flux; // TYPE ARG ARG+++
} notifyAndFlux;

struct cell
{
    struct notifyAndFlux payload;
    struct cell* next;
};
typedef struct cell* Stack;

void stack_push(Stack* s, notifyAndFlux element);

notifyAndFlux* stack_pop(Stack* s);

notifyAndFlux* stack_top(Stack* s);