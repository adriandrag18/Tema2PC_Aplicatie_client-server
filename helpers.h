// helper.h
// Created by adrian on 24.04.2020.

#ifndef TEMA2_HELPERS_H
#define TEMA2_HELPERS_H

#include <stdlib.h>

#define BUFLEN 1582

#define PAYLOADLEN 1501

#define MAX_CLIENTS 1000

#define START 0xffffffff
#define STOP 0x99995555

enum {INT, SHORT_REAL, FLOAT, STRING, SUBSCRIBE, SUBSCRIBE_SF, UNSUBSCRIBE, IDENTIFICATION};

typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned long U32;
typedef unsigned long long U64;

typedef struct m {
    int start;                          // at 0
    short len;                          // at 4
    char topic[51];                     // at 6
    U8 type;                            // at 57
    struct sockaddr_in senderAddr;      // at 60
    char payload[PAYLOADLEN];           // at 76
} message;

extern char strType[7][20];

extern void parsePayload(int type, const char* payload, char *out);
extern int isCompete(char *buffer);
extern void examineMemory(char *buffer, int max, int alphaNumeric);

#define DIE(assertion, call_description)	\
    if (assertion) {					    \
        fprintf(stderr, "(%s, %d): ",	    \
                __FILE__, __LINE__);	    \
        perror(call_description);		    \
        exit(EXIT_FAILURE);				    \
    }

#define ASSERT(assertion, call_description, otherwise)	\
    if (assertion) {					                \
        fprintf(stderr, "(%s, %d): ",	                \
                __FILE__, __LINE__);	                \
        perror(call_description);			            \
	    otherwise;		                                \
    }

#endif //TEMA2_HELPERS_H
