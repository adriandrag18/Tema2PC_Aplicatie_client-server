// struct.h
// Created by adrian on 26.04.2020.

#ifndef TEMA2_STRUCT_H
#define TEMA2_STRUCT_H

#define BUFLEN 1582

typedef struct l{
    char buffer[BUFLEN];
    struct l *next;
} LIST;

typedef struct {
    char id[11];
    char topic[51];
    int sf;
    LIST *list;
} STORE;

typedef struct {
    char id[11];
    int socket;
} CLIENT;

extern STORE *storage;
extern int capacity;
extern int size;
extern int *emptyStorage;
extern int sp;
extern char **incomplete;
extern int incLen;
extern int fdmax;

extern void printStorage();
extern void sendFromStorage(char *id, int socket);
extern void unsubscribe(char *id, char *topic);
extern void putAt(char *buffer, int socket);
extern void subscribe(char *id, char *topic, int sf);
extern void freeAll();
extern void store(char *id, char *topic, char *buffer, int where);
extern void load();
extern void saveStorage();
extern int findSocket(char *id) ;
extern void updateClient(char *id, int socket);
extern char *findID(int socket);

#endif //TEMA2_STRUCT_H
