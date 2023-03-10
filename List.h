#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CLIENTS_NO 100

typedef struct stringList{
    char **strings;
    int size;
}stringList;

typedef struct Client{
	int socket;
	char *ID;
    stringList *topics;
    stringList *topicsWithFlag;
    stringList *coada;
    struct Client *next;
}Client;

typedef struct list{
    Client *head;
    Client *tail;
}list;

void initClient(Client *c, int sock){
	c->socket = sock;
    c->ID = (char *)malloc(sizeof(char)*11);
	c->ID = NULL;
    c->topics = (stringList *) malloc(sizeof(stringList));
    c->topics->size = 0;
    c->topics->strings = (char **) malloc(sizeof(char *) * 100);

    
    c->topicsWithFlag = (stringList *) malloc(sizeof(stringList));
    c->topicsWithFlag->size = 0;
    c->topicsWithFlag->strings = (char **) malloc(sizeof(char *) * 100);

    c->coada = (stringList *) malloc(sizeof(stringList));
    c->coada->size = 0;
    c->coada->strings = (char **) malloc(sizeof(char *) * 100);
}

void setID(Client *c, char *ID){
    strncpy(c->ID, ID, 11);
}

void initList(list *l){
    l = (list *)malloc(sizeof(list));
    l->head = NULL;
    l->tail = NULL;
}

void addClient(list *l, Client *c){
    if (l->head == NULL){
        l->head = c;
        l->tail = c;
    }
    else {
        l->tail->next = c;
        l->tail = c;
    }
}

Client *findClient(list *l, int sock){
    Client *aux = l->head;
    while(aux != NULL){
        if (aux->socket == sock)
            break;
        aux = aux->next;
    }
    return aux;
}

void removeLast(list *l){
    Client *aux = l->head;
    while(aux->next->next != NULL){
        aux = aux->next;
    }
    free(aux->next);
    aux->next = NULL;
    l->tail = aux;
}

Client *findClientByID(list *l, char *ID){
    Client *aux = l->head;
    //char oldID[11], newID[11];
    
    while(aux != NULL){
        if (aux->ID != NULL){
            //printf("da\n");
            //printf("%s %s : %d\n", aux->ID, ID, strncmp(aux->ID, ID, strlen(aux->ID)));
            if (strcmp(aux->ID, ID) == 0)
                return aux;
        }
        aux = aux->next;
    }
    return NULL;
}

void printList(list *l){
    Client *aux = l->head;
    while(aux != NULL){
        printf("%s : %d\n", aux->ID, aux->socket);
        aux = aux->next;
    }
}

//charList functions

void addString(stringList *l, char *s){
    l->strings[l->size] = s;
    l->size++;
}

void removeString(stringList *l, char *s, int len){
    int pos = -1;
    for(int i = 0; i < l->size; i++){
        if (strncmp(s, l->strings[i], len) == 0){
            pos = i;
            break;
        }
    }
    if (pos != -1){
        for(int i = pos; i < (l->size - 1); i++)
            l->strings[i] = l->strings[i+1];
        l->size--;
    }
}

int stringExists(stringList *l, char *s, int len){
    for(int i = 0; i < l->size; i++)
        if (strncmp(s, l->strings[i], len) == 0)
            return 1;
    return 0;
}

void printStrings(stringList *l){
    for (int i = 0; i < l->size; i++)
        printf("%s\n", l->strings[i]);
    printf("\n");
}

//Power function
int power(int x, int y){
    int res = 1;
    for (int i = y; i > 0; i--)
        res = res * x;
    return res;
}