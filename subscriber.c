// subscriber.c
// Created by adrian on 24.04.2020.

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include "helpers.h"

int sockfd;
char incomplete[2 * BUFLEN];
int len = 0;
char id[11];

void process(char *buffer);
int parseInput(char *buffer);

void usage(char *file)
{
	fprintf(stderr, "Usage: %s <ID_Client> <IP_Server> <Port_Server>n", file);
	exit(0);
}

void help(char *s) {
    printf("ERROR: Unknown command: %s"
           "\tCommands accepted:\n"
           "\tsubscribe topic SF: SF in {0, 1}\n"
           "\tunsubcribe topic\n"
           "\texit\n"
           "\tread <file>: citeste comenzile din fisierul dat ca argument\n", s);
}

void invalidSF(char *s) {
    printf("ERROR: Invalid SF\n\tSF must be 0 or 1: %s", s);
}

void invalidTopic(char *s) {
    printf("ERROR: Invalid topic: %s", s);
}

void invalidSubscription(char *s) {
    printf("ERROR: Invalid subscription: %s", s);
}

void invalidUnsubscription(char *s) {
    printf("ERROR: Invalid unsubscription: %s", s);
}
/**
 *
 * @param file
 * @description parseaza conmezile din fisierul dat
 *  pe post de input de la tastaura - folosita pentru debug
 */
void readCommandsFromFile(char *file) {
    FILE *fin = fopen(file, "r");
    if(!fin) {
        printf("ERROR: opening file");
        return;
    }
    int size = 100, l;
    char *line = (char *)malloc(size);
    l = getline(&line, (size_t *)&size, fin);
    while (l>=0) {
        printf("%s", line);
        parseInput(line);
        l = getline(&line, (size_t *)&size, fin);
    }
}

/**
 *
 * @param buffer - input-ul de la tastatura
 * @param sockfd adresa la care trebuie trimis mesajul
 * @return 0 in cazul in care input-ul a fost "exit", 1 in rest
 */
int parseInput(char *buffer) {
    int n;
    int sf = 2;
    char topic[51], s[12];
    memset(buffer, 0, BUFLEN);
    fgets(buffer, BUFLEN - 1, stdin);

    if (strncmp(buffer, "exit", 4) == 0)
        return 0;
    if (strncmp(buffer, "read", 4) == 0) {
        char file[20];
        sscanf(buffer, "%s %s", s, file);
        readCommandsFromFile(file);
    }
    if (strncmp(buffer, "subscribe", 9) == 0) {
        memset(s, 0, 11);
        memset(topic, 0, 51);
        if (strlen(buffer) < 12) {
            invalidSubscription(buffer);
            return 1;
        }
        sscanf(buffer, "%s %s", s, topic);
        if (strlen(topic) < 1 || strlen(topic) > 50) {
            invalidTopic(buffer);
            return 1;
        }
        if (strlen(buffer) < 12 + strlen(topic)) {
            invalidSubscription(buffer);
            return 1;
        }
        sscanf(buffer, "%s %s %d", s, topic, &sf);
        if (sf != 1 && sf != 0) {
            invalidSF(buffer);
            return 1;
        }

        memset(buffer, 0, BUFLEN);
        message *m = (message *) buffer;
        m->start = START;
        m->len = (short) (4 + 2 + 51 + 1 + (2 + 16) + 11 + 4);
        strcpy(m->topic, topic);
        m->type = (U8) (sf + 4);
        memset((void *) &m->senderAddr, 0, sizeof(struct sockaddr_in));
        strcpy(m->payload, id);
        *(int *) (buffer + m->len - 4) = STOP;

        n = send(sockfd, buffer, m->len, 0);
        ASSERT(n < 0, "Error in sending message", return 1)
        printf("subscribed %s\n", topic);
        return 1;
    }
    if (strncmp(buffer, "unsubscribe", 11) == 0) {
        memset(s, 0, 11);
        memset(topic, 0, 50);
        if (strlen(buffer) < 13) {
            invalidUnsubscription(buffer);
            return 1;
        }
        sscanf(buffer, "%s %s", s, topic);
        if (strlen(topic) < 1 || strlen(topic) > 50) {
            invalidTopic(buffer);
            return 1;
        }
        memset(buffer, 0, BUFLEN);
        message *m = (message *) buffer;
        m->start = START;
        m->len = (short) (4 + 2 + 51 + 1 + (2 + 16) + 11 + 4);
        strcpy(m->topic, topic);
        m->type = (U8) UNSUBSCRIBE;
        memset((void *) &m->senderAddr, 0, sizeof(struct sockaddr_in));
        strcpy(m->payload, id);
        *(int *) (buffer + m->len - 4) = STOP;

        n = send(sockfd, buffer, m->len, 0);
        ASSERT(n < 0, "Error in sending message", return 1)
        printf("unsubscribed %s\n", topic);
        return 1;;
    }
    help(buffer);
    return 1;
}

/**
 *
 * @param buffer - mesajul incomplet
 * @param socket - unde trebuie trimis mesajul in caz ca se completeaz
 */
void concatMsg(char *buffer) {
    int i = 0;
    while ( *(int *) (buffer + i) != STOP && i < BUFLEN)
        i++;
    i += 4;

    int len = *(short *) (incomplete + 4);
    if (i > BUFLEN && len < i)
        printf("Unable to concatenate fragments\n Dropping message...\n");
    else {
        memcpy(incomplete + len - i, buffer, i);
        if (isCompete(incomplete)) {
            // Message completed
            process(incomplete);
        } else
            printf("Unable to concatenate fragments\n Dropping message...\n");
    }
    memset(incomplete, 0, BUFLEN);
    len = 0;
}
/**
 *
 * @param buffer - mesajul primit
 * @param socket - unde trebuie trimis mesajul
 */
void process(char *buffer) {
    printf ("S-a primit de la server de pe socketul %d un mesaj\n", sockfd);

    if (!isCompete(buffer)){
        if (len) {
            concatMsg(buffer);
        } else if (*(int *) buffer == START) {
            // stocamm prima jumatate a mesajului
            memset(incomplete, 0, BUFLEN);
            memcpy(incomplete, buffer, BUFLEN);
        }
        return;
    }

    message *m = (message *) buffer;
    char out[PAYLOADLEN];
    ASSERT( m->type < 0 || m->type > 3, "Type not in {0, 1, 2, 3}", return)
    parsePayload(m->type, m->payload, out);
    printf("%s:%d - %s - %s - %s\n", inet_ntoa(m->senderAddr.sin_addr), ntohs(m->senderAddr.sin_port),
           m->topic, strType[m->type], out);

    if (*(int *) (buffer + m->len) == START)
        process(buffer + m->len);
}

int main(int argc, char *argv[]) {
	int n, ret;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];

	if (argc < 4) {
		usage(argv[0]);
	}
    strncpy(id, argv[1], 11);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket")

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton")

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect")

	memset(buffer, 0, BUFLEN);
    message *m = (message *) buffer;
    m->start = START;
    m->len = (short) (4 + 2 + 51 + 1 + (2 + 16) + 11 + 4);
    m->type = (U8) IDENTIFICATION;
    memset((void *) &m->senderAddr, 0, sizeof(struct sockaddr_in));
    strcpy(m->payload, id);
    *(int *) (buffer + m->len - 4) = STOP;

    n = send(sockfd, buffer, m->len, 0);
    ASSERT(n < 0, "Error in sending message", return 1)

    fd_set read_fds;	// multimea de citire folosita in select()
    fd_set tmp_fds;		// multime folosita temporar
    int fdmax;

    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);

    FD_SET(0, &read_fds);
    FD_SET(sockfd, &read_fds);
    fdmax = sockfd;

    int status = 1;
	while (status) {
        tmp_fds = read_fds;
        DIE(select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) < 0, "select")

        if (FD_ISSET(0, &tmp_fds)) {
            // se citeste de la tastatura
            status = parseInput(buffer);
            continue;
        }
        if (FD_ISSET(sockfd, &tmp_fds)) {
            memset(buffer, 0, BUFLEN);
            n = recv(sockfd, buffer, BUFLEN, 0);
            ASSERT(n == -1, "Error receiving" ,continue)

            if (n == 0) {
                printf("Server %d a inchis conexiunea\n", sockfd);
                FD_CLR(sockfd, &read_fds);
                break;
            }
            process(buffer);
        }
	}
	// se vreifica daca mai sunt mesaje de primit inainte sa se inchida conexiunea
    tmp_fds = read_fds;
    DIE(select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) < 0, "select")
    while (FD_ISSET(sockfd, &tmp_fds)) {
        memset(buffer, 0, BUFLEN);
        n = recv(sockfd, buffer, BUFLEN, 0);
        if (n > 0)
            process(buffer);
        tmp_fds = read_fds;
        DIE(select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) < 0, "select")
    }
	DIE(close(sockfd), "close")
	return 0;
}
