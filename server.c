// server.c
// Created by adrian on 24.04.2020.

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include "helpers.h"
#include "struct.h"

void usage(char *file) {
    fprintf(stderr, "Usage: %s server_port\n", file);
    exit(0);
}

void help(char *s) {
    printf("ERROR:Unknown command: %s"
           "Only commands accepted:\n\texit\n"
           "\twrite: salveaza listele cu abonatii si mesajele salvate "
           "intr-un fisier binar\n"
           "\tload: incarca listele cu abonatii si mesajele salvate\n"
           "\tprint\n", s);
}

int parseInput (char *buffer) {
    memset(buffer, 0, BUFLEN);
    fgets(buffer, BUFLEN - 1, stdin);
    if (strncmp(buffer, "exit", 4) == 0)
        return 0;
    if (strncmp(buffer, "print", 5) == 0) {
        printStorage();
        return 1;
    }if (strncmp(buffer, "write", 5) == 0) {
        saveStorage();
        return 1;
    }if (strncmp(buffer, "load", 4) == 0) {
        load();
        return 1;
    }
    help(buffer);
    return 1;
}

void process(char *buffer, int socket);

/**
 *
 * @param buffer - mesajul incomplet
 * @param socket - unde trebuie trimis mesajul in caz ca se completeaz
 */
void concatMsg(char *buffer, int socket) {
    int i = 0;
    while ( *(int *) (buffer + i) != STOP && i < BUFLEN)
        i++;
    i += 4;
    examineMemory(buffer, 40, 0);
    printf("Find STOP at %d\n", i);
    int len = *(short *) (incomplete[socket] + 4);
    if (!(i > BUFLEN && len < i)) {
        memcpy(incomplete[socket] + len - i, buffer, i);
        if (isCompete(incomplete[socket])) {
            examineMemory(incomplete[socket], 70, 0);
            printf("Message completed\n");
            process(incomplete[socket], socket);
        }
    }
    free(incomplete[socket]);
    incomplete[socket] = NULL;
}
/**
 *
 * @param buffer - mesajul primit
 * @param socket - unde trebuie trimis mesajul
 */
void process(char *buffer, int socket) {
    if (!isCompete(buffer)){
        examineMemory(buffer, 40, 0);
        if (incLen > socket && incomplete[socket]) {
            concatMsg(buffer, socket);
        } else if (*(int *) buffer == START) {
            putAt(buffer, socket);
        }
        return;
    }
    message *m = (message *)buffer;
    ASSERT(m->type < 4 || m->type > 7, "Invalid operation", return);
    if (m->type == UNSUBSCRIBE) {
        unsubscribe(findID(socket), m->topic);
    } else if (m->type == IDENTIFICATION) {
        printf("New client %s connected from %s:%d\n", m->payload, inet_ntoa(m->senderAddr.sin_addr),
                ntohs(m->senderAddr.sin_port));
        updateClient(m->payload, socket);
        sendFromStorage(m->payload, socket);
    } else {
        subscribe(findID(socket), m->topic , m->type - 4);
    }
    if (*(int *) (buffer + m->len) == START)
        process(buffer + m->len, socket);
}
/**
 *
 * @param buffer - mesajul primit de la clientul UDP
 * @param from - adresa de unde a venit
 */
void processUDP(char *buffer, struct sockaddr_in *from){
    int n;
    char new[BUFLEN];
    memset(new, 0 , BUFLEN);
    message *m = (message *)new;
    m->start = START;
    m->type = *(U8 *) (buffer + 50);
    *(U8 *) (buffer + 50) = 0;
    strcpy(m->topic, buffer);
    memcpy((void *)&m->senderAddr, (void *)from, sizeof(*from));

    switch (m->type) {
        case INT:
            // size of START, len, topic, type, sender address,payload + \0, STOP
            m->len = 4 + 2 + 51 + 1 + (2 + 16) + (5 + 1) + 4;
            memcpy(m->payload, buffer + 51, 5);
            *(int *) (new + m->len - 4) = STOP;
            break;
        case SHORT_REAL:
            m->len = 4 + 2 + 51 + 1 + (2 + 16) + (2 + 1) + 4;
            printf("%d", m->len);
            memcpy(m->payload, buffer + 51, 2);
            *(int *) (new + m->len - 4) = STOP;
            break;
        case FLOAT:
            m->len = 4 + 2 + 51 + 1 + (2 + 16) + (6 + 1) + 4;
            memcpy(m->payload, buffer + 51, 6);
            *(int *) (new + m->len - 4) = STOP;
            break;
        case STRING:
            *(U8 *) (buffer + 1551) = 0;
            strcpy(m->payload, buffer + 51);
            m->len = 4 + 2 + 51 + 1 + (2 + 16) + strlen(m->payload) + 1 + 4;
            *(int *) (new + m->len - 4) = STOP;
            break;
        default:
            ASSERT(1, "Type in switch", return)
            break;
    }

    int socket;
    for (int j = 0; j < size; ++j) {
        if (strcmp(storage[j].topic, m->topic) == 0) {
            ASSERT((socket = findSocket(storage[j].id)) == -2,
                   "Did not find client", continue)
            if (socket != -1) {
                n = send(socket, new, BUFLEN, 0);
                ASSERT(n < 0, "send", continue)
            } else {
                if (storage[j].sf)
                    store(storage[j].id, m->topic, new, j);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        usage(argv[0]);
    }

	int sockUDP, sockTCP, newsockfd, port, len;
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr, cli_addr, from_station;
	int n, i, ret;
	socklen_t clilen;
	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar

	// se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

    sockUDP = socket(AF_INET, SOCK_DGRAM, 0);
    sockTCP = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockTCP < 0, "socket TCP");
    DIE(sockUDP < 0, "socket UDP");

	port = atoi(argv[1]);
	DIE(port == 0, "atoi");

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

    ret = bind(sockUDP, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
    DIE(ret < 0, "bind UDP");
    ret = bind(sockTCP, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
    DIE(ret < 0, "bind TCP");

	ret = listen(sockTCP, MAX_CLIENTS);
	DIE(ret < 0, "listen");

	// se adauga noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
    FD_SET(sockTCP, &read_fds);
    FD_SET(sockUDP, &read_fds);
    FD_SET(0, &read_fds); // se adauga stdin
	if(sockUDP > sockTCP)
        fdmax = sockUDP;
	else
        fdmax = sockTCP;

	int status = 1;
	while (status) {
		tmp_fds = read_fds; 
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (i = 0; i <= fdmax; i++) {
            if (!(FD_ISSET(i, &tmp_fds)))
                continue;
            if (i == 0)
                status = parseInput(buffer);
            else if (i == sockUDP) {
                ret = recvfrom(sockUDP, buffer, BUFLEN, 0, (struct sockaddr*)&from_station, (socklen_t*) &len);
                ASSERT(ret < 0, "recvfrom", continue);

                processUDP(buffer, &from_station);
            } else if(i == sockTCP) {
                // a venit o cerere de conexiune pe socketul inactiv (cel cu listen), pe care serverul o accepta
                clilen = sizeof(cli_addr);
                newsockfd = accept(sockTCP, (struct sockaddr *) &cli_addr, &clilen);
                ASSERT(newsockfd < 0, "accept", continue);
                char flag = 1;
                setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
                // se adauga noul socket intors de accept() la multimea descriptorilor de citire
                FD_SET(newsockfd, &read_fds);
                if (newsockfd > fdmax)
                    fdmax = newsockfd;

            } else {
                // s-au primit date pe unul din socketii de client, asa ca serverul trebuie sa le receptioneze
                memset(buffer, 0, BUFLEN);
                n = recv(i, buffer, sizeof(buffer), 0);
                ASSERT(n < 0, "recv", continue);

                if (n == 0) {
                    printf("Client %s disconnected.\n", findID(i));
                    ASSERT(close(i), "close", continue)
                    updateClient(findID(i), -1);
                    FD_CLR(i, &read_fds);
                } else {
                    process(buffer, i);
                }
            }
		}
	}
	// inchidere fortata
    for (i = 0; i < fdmax; ++i) {
        if (FD_ISSET(i, &read_fds))
            ASSERT(close(i), "close", continue)

    }
    freeAll();
	return 0;
}

