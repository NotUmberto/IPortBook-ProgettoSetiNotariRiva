
typedef struct notifyAndFlux
{
    char* notify;
    char* flux;
} notifyAndFlux;

struct cell
{
    struct notifyAndFlux payload;
    struct cell* next;
};
typedef struct cell* Stack;

void stack_push(Stack* s, notifyAndFlux element);

notifyAndFlux* stack_pop(Stack* s);