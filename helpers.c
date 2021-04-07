// helpers.c
// Created by adrian on 26.04.2020.
#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include "helpers.h"

char strType[7][20] = { "INT",
                        "SHORT_REAL",
                        "FLOAT",
                        "STRING",
                        "SUBSCRIBE",
                        "SUBSCRIBE_SF",
                        "UNSUBSCRIBE"       };
/**
 *
 * @param type - ce reprezinta payload-ul
 * @param payload
 * @param out - payload-ul parsat daca este de forma numerica
 */
void parsePayload(int type, const char* payload, char *out) {
    memset(out, 0, PAYLOADLEN);
    long long n;
    double d;
    switch (type) {
        case INT:
            n = ntohl(*(U32 *) (payload + 1));
            if (*(U8 *) (payload))
                n *= (-1);

            sprintf(out, "%lld", n);
            break;
        case SHORT_REAL:
            d = (double) ntohs(*(U16 *) payload) / 100;
            sprintf(out, "%.2f", d);
            break;
        case FLOAT:
            d = (double) ntohl(*(U32 *) (payload + 1));
            if (*(U8 *) (payload))
                d *= (-1);
            n = *(U8 *) (payload + 5);
            for (int i = 0; i < n; i++)
                d /= 10;
            sprintf(out, "%f", d);
            break;
        case STRING:
            memcpy(out, payload, PAYLOADLEN - 1);
            break;
        default:
            DIE(0, "Type")
    }
}
/**
 *
 * @param buffer - zona de memorie in care este mesajul
 * @return - 0 in cazul in care nu are START la inceput sau
 *  STOP la pozitian len - 4
 */
int isCompete(char *buffer) {
    message *m = (message *) buffer;
    if (m->start != START)
        return 0;
    if (*(int *) (buffer + m->len - 4) != STOP)
        return 0;
    return 1;
}
/**
 *
 * @param buffer - adresa de incrput
 * @param max - numarul maxim de octeti
 * @param alphaNumeric - nr nenul -> se printreaza sub forma de raractere daca se poate
 *                       0 -> se printeaza numai sub forma numerica
 * @description folosita la debug
 */
void examineMemory(char *buffer, int max, int alphaNumeric) {
    int j = 0;
    while (buffer[j] || j < max) {
        if (alphaNumeric && ((buffer[j] >= 48 && buffer[j] <= 126) || buffer[j] == 32))
            printf("%c", buffer[j]);
        else {
            printf(" %d", j);
            printf("\\%.3u", (U8) buffer[j]);
        }
        j++;
    }
    printf("\n");
}


