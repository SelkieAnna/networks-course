#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

typedef struct Node
{
    struct Node *next;
    int data;
} Node;

typedef struct Stack {
    int size;
    Node *upper;
} Stack;

int fd[2];
Stack *stack;

int peek() {
    // return the upper element in stack (without poping it)
    printf("Server: %d\n", stack->upper->data);
    return 0;
}

void push(int data) {
    // push new element into the stack
    Node *new = malloc(sizeof(Node));
    new->data = data;
    new->next = stack->upper;
    stack->upper = new;
    stack->size = stack->size + 1;
    printf("Server: %d was pushed onto the stack\n", data);
}

int pop() {
    // pop upper element from the stack
    if (stack->size == 0) {
        printf("Server: The stack is already empty\n");
        return 0;
    }
    printf("Server: %d\n", stack->upper->data);
    Node *upp = stack->upper;
    stack->upper = stack->upper->next;
    stack->size = stack->size - 1;
    free(upp);
    return 0;
}

int empty() {
    // check if the stack is empty
    if (stack->size == 0) {
        printf("Server: The stack is empty\n");
    } else {
        printf("Server: The stack is not empty\n");
    }
    return 0;
}

void display() {
    // print the stack
    if (stack->size == 0) {
        printf("Server: The stack is empty\n");
        return;
    }
    int i;
    Node *cur = stack->upper;
    printf("Server: ");
    for (i = 1; i < stack->size; i++) {
        printf("%d ", cur->data);
        cur = cur->next;
    }
    printf("%d\n", cur->data);
}

void create() {
    // create a new empty stack
    stack->upper =  NULL;
    stack->size = 0;
    printf("Server: A new stack was created\n");
}

void stack_size() {
    // print stack size
    printf("Server: The stack size is %d\n", stack->size);
}

void signal_handler(int sig) {
    char command[50];
    char *arg;
    // read pipe
    read(fd[0], command, sizeof(char) * 50);
    // parse command and arguments
    int i;
    for (i = 0; i < 50; i++) {
        if (command[i] == ' ') {
            command[i] = '\0';
            arg = command + i + 1;
        }
    }
    // execute function
    if (strcmp(command, "peek") == 0) {
        peek();
    }
    if (strcmp(command, "push") == 0) {
        push(atoi(arg));
    }
    if (strcmp(command, "pop") == 0) {
        pop();
    }
    if (strcmp(command, "empty") == 0) {
        empty();
    }
    if (strcmp(command, "display") == 0) {
        display();
    }
    if (strcmp(command, "create") == 0) {
        create();
    }
    if (strcmp(command, "stack_size") == 0) {
        stack_size();
    }
}

void server() {
    // declare handler
    stack = malloc(sizeof(Stack));
    signal(SIGUSR1, signal_handler);
    for(;;){}
}

int client(pid_t serv_pid) {
    char command[50];
    for(;;) {
        // read command
        scanf(" %[^\t\n]s", command);
        // send to pipe
        write(fd[1], command, sizeof(char) * 50);
        // send signal
        kill(serv_pid, SIGUSR1);
    }
}

int main() {
    if (pipe(fd) == 0) {
		pid_t f;
        f = fork();
        if (f == 0) {       // if child (server)
            server(); 
        } else {            // if parent (client)
            client(f);
        }
    } else return 1;
}