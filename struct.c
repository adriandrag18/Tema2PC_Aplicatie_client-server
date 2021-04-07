// struct.c
// Created by adrian on 26.04.2020.

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "helpers.h"
#include "struct.h"

int fdmax = 0;
char **incomplete = NULL;
int incLen = 0;

STORE *storage;
int capacity = 0;   // cata memorie este alocata
int size = 0;       // cata din memorie este folosita

// indici unde se poate suprascrie rezultati in urma dezabonarii
int *emptyStorage;
int sp = -1;

CLIENT *list = NULL;
int nrClients;
int listCapacity;
/**
 *
 * @param id
 * @param socket
 * @description cauta daca clientul exista deja in lista si updateaza socket ul
 *  sau daca nu il gaseste il pune la coada
 *  mai aloca memorie daca este necesar
 */
void updateClient(char *id, int socket) {
    for(int i = 0; i < nrClients; i++) {
        if (strcmp(list[i].id, id) == 0) {
            list[i].socket = socket;
            return;
        }
    }
    if (list == NULL) {
        list = (CLIENT *) malloc(8 * sizeof(CLIENT));
        listCapacity = 8;
    }
    while (nrClients == listCapacity -1) {
        CLIENT *new;
        new = (CLIENT  *) realloc(incomplete, 2 * listCapacity * sizeof(char *));
        list = new;
        listCapacity *= 2;
    }
    list[nrClients].socket = socket;
    strcpy(list[nrClients].id, id);
    nrClients++;
}
/**
 *
 * @param id
 * @return socketul pe care are conexiunea clintul cu id ul dat
 *  -1 daca nu este conectat
 *  -2 daca clientul nu este gasit
 */
int findSocket(char *id) {
    for(int i = 0; i < nrClients; i++) {
        if (strcmp(list[i].id, id) == 0)
            return list[i].socket;
    }
    return -2;
}
/**
 *
 * @param socket
 * @return id-ul clientului conectat pe socket ul dat
 *  NULL in cazul in care nu exista nici o potrivire
 */
char *findID(int socket) {
    for(int i = 0; i < nrClients; i++) {
        if (list[i].socket == socket)
            return list[i].id;
    }
    return NULL;
}

/**
 *
 * @param buffer - mesajul incomplet
 * @param socket - socket ul de pe care a venit
 * @description daca este nevoie mareste capacitatea lui incomplete
 *  alooca memoreie pentru a stoca prima jumatae a mesajului incomplet
 */
void putAt(char *buffer, int socket) {
    if (incomplete == NULL) {
        incomplete = (char **) malloc(8 * sizeof(char *));
        incLen = 8;
        for (int i = 0; i < 8; ++i)
            incomplete[i] = NULL;
    }
    while (fdmax > incLen) {
        char **new;
        new = (char **) realloc(incomplete, 2 * incLen * sizeof(char *));
        incomplete = new;

        for (int i = incLen; i < 2 * incLen; ++i)
            incomplete[i] = NULL;

        incLen *= 2;
    }

    if (incomplete[socket]) {
        free(incomplete[socket]);
    }
    incomplete[socket] = (char *) malloc(2 * BUFLEN);
    memcpy(incomplete[socket], buffer, BUFLEN);
}
/**
 *
 * @param id - id ul client care se aboneaza
 * @param topic - topica la care se aboneaza
 * @param sf - cu sau fara stocare a mesajelor
 * @description verifica daca clientul este deja abonat la topic,
 *  caz in care updateaza sf-ul, daca nu pune la o pozitie din emptyStorage
 *  sau la finalul vextorului si incrementeaza size
 *  daca capacitatea este depasita se realoca memori (se dubleaaza capacitatea)
 */
void subscribe(char *id,  char *topic, int sf) {
    for(int i = 0; i < size; i++) {
        if (strcmp(storage[i].id, id) == 0 && strcmp(storage[i].topic, topic) == 0) {
            storage[i].sf = sf;
            printf("Overwriting old subscription\n");
            if (storage[i].sf == 0) {
                while (storage[i].list) {
                    LIST *tmp = storage[i].list;
                    storage[i].list = storage[i].list->next;
                    free(tmp);
                }
            }
            return;
        }
    }
    if (capacity == 0) {
        storage = (STORE *) malloc(128 * sizeof(STORE));
        emptyStorage = (int *) malloc(128 * 4);
        memset(emptyStorage, 0, 128 * 4);
        capacity = 128;
    }
    if (size == capacity - 1) {
        STORE *new;
        int *n;
        new = (STORE *) realloc(storage, 2 * capacity * sizeof(STORE));
        n = (int *) realloc(emptyStorage, 2 * capacity * 4);
        storage = new;
        emptyStorage = n;
        memset(emptyStorage + capacity, 0, capacity);
        capacity *= 2;
    }
    int i = size;
    if (sp != -1) {
        i = emptyStorage[sp];
        sp--;
    } else {
        size++;
    }
    strcpy(storage[i].id, id);
    storage[i].sf = sf;
    memset(storage[i].topic, 0, 51);
    strcpy(storage[i].topic, topic);
    storage[i].list = NULL;
}

/**
 *
 * @param socket - socket ul client care se dezaboneaza
 * @param topic - topica de la care se dezaboneaz
 * @description daca clientul avea sf = 1 si mesaje ctocate acestea se sterg
 *  si memoria este eliberata
 */
void unsubscribe(char *id, char *topic) {
    for(int i = 0; i < size; i++) {
        if (strcmp(storage[i].id, id) == 0 && strcmp(storage[i].topic, topic) == 0) {
            sp++;
            // se suprascriu datele pt ca functia sendFromStorage
            // sa nu trebuiasca sa verifice in emptyStorage pentru fiecare pozitie
            // care corespunde cu id si topic
            memset(storage[i].id, 0 , 11);
            memset(storage[i].topic, 0 , 51);
            storage[i].sf = -1;
            emptyStorage[sp] = i;
            if (storage[i].sf == 1) {
                while (storage[i].list) {
                    LIST *tmp = storage[i].list;
                    storage[i].list = storage[i].list->next;
                    free(tmp);
                }
            }
            break;
        }
    }
}
/**
 *
 * @param id - id ul client
 * @param topic - topica la care este abonat
 * @param buffer - mesajul de stocat
 * @param where - la ce pozitie in lista
 * @description verifica daca pozitia data este corecta si daca intradevar
 *  clientul are sf = 1 si stoceaza la pozitia data daca aceasta este corecta
 *  sau la o pozitie gasita in caz contrat
 */
void store(char *id, char *topic, char *buffer, int where) {
    int i;
    if (strcmp(storage[where].id, id) == 0 && strcmp(storage[where].topic, topic) == 0) {
        i = where;
    } else {
        for (i = 0; i < size; i++) {
            if (strcmp(storage[i].id, id) == 0 && strcmp(storage[i].topic, topic) == 0)
                break;
        }
    }
    if (i == size) {
        printf("ERROR: Could not store the message\n");
        return;
    }
    if (storage[i].sf == 1) {
        if (!storage[i].list) {
            storage[i].list = (LIST *) malloc(sizeof(LIST));
            storage[i].list->next = NULL;
            memcpy(storage[i].list, buffer, BUFLEN);
            return;
        }
        LIST *p;
        for (p = storage[i].list; p->next; p = p->next);
        p->next = (LIST *) malloc(sizeof(LIST));
        p->next->next = NULL;
        memcpy(p->next->buffer, buffer, BUFLEN);
    } else {
        printf("ERROR: Try cu store for a client with sf = 0");
    }
}
/**
 *
 * @param id - id ul clientului cu sf = 1 care s-a reconectat
 * @param soket - socket ul actual al clientului cu sf = 1 care s-a reconectat
 * @description couta prin toate pozitiile
 */
void sendFromStorage(char *id, int socket) {
    for(int i = 0; i < size; i++) {
        if (strcmp(storage[i].id, id) == 0 ) {
            if (storage[i].sf == 1) {
                LIST *p = storage[i].list;

                while (p) {
                    send(socket, p->buffer, BUFLEN, 0);
                    LIST *tmp = p;
                    p = p->next;
                    free(tmp);
                }
                storage[i].list = NULL;
            }
        }
    }
}
/**
 * @description printeaza datele din Storage - folosita a debug
 */
void printStorage() {
    printf("Printing storage: size: %d, capacity: %d\n", size, capacity);
    for (int i = 0; i < size; ++i) {
        int empty = 0;
        for (int j = 0; j <= sp; ++j) {
            if (emptyStorage[j] == i) {
                empty = 1;
            }
        }
        if (empty)
            continue;
        printf("Client: %s at topic: %s with SF = %d\n",
               storage[i].id, storage[i].topic, storage[i].sf);
        LIST *p = storage[i].list;
        while (p) {
            message *m = (message *) p->buffer;
            char out[PAYLOADLEN];
            parsePayload(m->type, m->payload, out);
            printf("%s - %s - %s\n", m->topic, strType[m->type], out);
            p = p->next;
        }
        printf("\n");
    }
}

/**
 * @description salveaza intr-un fisier binar datele din storage si emptyStorage
 *  folosita a debug
 */
void saveStorage() {
    FILE *fptr;
    ASSERT((fptr = fopen("storage.bin", "wb")) == NULL, "Error! opening file", return)

    fwrite(&capacity, 4, 1, fptr);
    fwrite(&size, 4, 1, fptr);
    fwrite(&sp, 4, 1, fptr);
    fwrite(emptyStorage, 4, size, fptr);
    fwrite(storage, sizeof(STORE), size, fptr);

    fclose(fptr);
}
/**
 * @description citeste datele savate anterior dintr-un fisier binar
 * si le suprascrie storage si empptyStorage- folosita a debug
 */
void load() {
    FILE *fptr;
    ASSERT((fptr = fopen("storage.bin", "rb")) == NULL, "Error! opening file", return)
    if(fptr == NULL) {
        printf("ERROR: opening file");
        return;
    }
    fseek(fptr, 0, SEEK_SET);
    fread(&capacity, 4, 1, fptr);
    fread(&size, 4, 1, fptr);
    fread(&sp, 4, 1, fptr);

    storage = (STORE *)malloc(capacity * sizeof(STORE));
    emptyStorage = (int *)malloc(capacity * 4);

    fread(emptyStorage, 4, size, fptr);
    fread(storage, sizeof(STORE), size, fptr);

    fclose(fptr);
}
/**
 * @description eliberaza toat memoria alocata pentru list, incomplete, emptyStorage,
 *  storage si listele din storage folosite pentru a stoca mesajele
 */
void freeAll() {
    free(list);
    nrClients = 0;
    listCapacity = 0;

    for (int i = 0; i < incLen; ++i)
        free(incomplete[i]);
    free(incomplete);
    incLen = 0;

    for(int i = 0; i < size; i++) {
        while (storage[i].list) {
            LIST *tmp = storage[i].list;
            storage[i].list = storage[i].list->next;
            free(tmp);
        }
    }
    free(storage);
    capacity = 0;
    size = 0;

    free(emptyStorage);
    sp = -1;
}