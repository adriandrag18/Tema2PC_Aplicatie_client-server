Draghicescu Adrian
321CB
                                                TEMA 2 PC

    Pentru a trimite mesajele intre server si clientii TCP am folosit urmatoarea structura
typedef struct {
    int start;
    short len;
    char topic[51];
    U8 type;
    struct sockaddr_in senderAddr;
    char payload[PAYLOADLEN];
    // int stop
} message;

    Campurile start, len si sirul de terminare stop(care este pus in payload sau la finalul sau) sunt folosite la reconstruiea
mesajelor si detectarea mesajelor incomplete
    Campul type este folosit de server pentru a tine transmite ce este in payload, iar clintul il foloseste petru a transmite
operatia efectuata (SUBSCRIBE, SUBSCRIBE_SF, UNSUBSCRIBE, IDENTIFICATION).
    In campul payload este pus payload ul de la clientii UDP sau id de la clienul TCP

    Pentru a stoca abonatii am folosit o structura pentru care alocam memorie dinamic (daca capacitatea este depasita se dubleaza
memoria alocata) si o stiva pentr a tine indicii pozitiilor goale(de unde s au dezabonat).

    Am acoperit doar cazul in care mesajele sunt fragmentate in 2, dar din teste mai mari de peste  1000 de mesaje am constat
ca nu este suficient, de la un punct majoritatea mesajelor sunt fragmentae si pierdute. Din pacate nu am gasit o solutuie in
timp util sa ca am lasat acesta impenetare neacoperitoare petru cazuri cu multe mesaje.

    In server pentru situatii de eroare am folosit un macro ASSERT similar cu DIE din laboratoare, dar in loc de exit(1)
foloseste alta comanda pentru a trece mai departe in program (continue sau return).

P.S. Am lasat si niste functii folosite pentru debug (impreuna cu comentariile pentru ele) sper ca nu este o problema.

P.S.S. Am experimentat cu comentarii de forma urmatoare. Sper ca nu am exagerat si ca nu o sa deranjeze la corectare.
/**
 *
 * @param buffer - mesajul primit de la clientul UDP
 * @param from - adresa de unde a venit
 */
