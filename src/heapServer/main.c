#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>

#include "constants.h"

#define PORTSERV 5500
#define ERRORMAXUSER 66
#define HEAPSIZE

int connectionsCount;
struct clientChain clients;

int clientThread(int sc) {
    /*** Lire le message ***/
    char *buf;
    buf = malloc(sizeof(char)*100);

    /* Ici il faut envoyer la taille du tas à allouer */

    /* if (read(sc,buf, sizeof(buf)) < 0) {
        perror("read");
        exit(EXIT_FAILURE);
    } */

    for (;;){
        sleep(4);
    }

    /* Fermer la connexion */
    shutdown(sc,2);
    close(sc);
    connectionsCount--;
    pthread_exit(0);    
}

int main(int argc, char *argv[]){
    struct sockaddr_in sin;  /* Nom de la socket de connexion */
    struct sockaddr_in exp;  /* Nom de la socket du client */
    int sc ;                 /* Socket de connexion */
    int scom;		      /* Socket de communication */
    int fromlen = sizeof(exp);
    int j;
    int tid[MAXUSER];
    int reuse = 1;

    connectionsCount = 0;
    clients.clientId = 0;
    clients.next = NULL;

    /* creation de la socket */
    if ((sc = socket(AF_INET,SOCK_STREAM,0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset((void*)&sin, 0, sizeof(sin));
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(PORTSERV);
    sin.sin_family = AF_INET;

    setsockopt(sc, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)); 


    /* nommage */
    if (bind(sc,(struct sockaddr *)&sin,sizeof(sin)) < 0){
        perror("bind");
        exit(EXIT_FAILURE);
    }
    listen(sc, 5);

    j = 0;
    for (;;) {
        /* On accepte la connexion */
        if ((scom = accept(sc, (struct sockaddr *)&exp, &fromlen))== -1){
            perror("accept");
            exit(EXIT_FAILURE);
        }

        /* Si on est au dessus de max clients, on envoie une erreur et on ferme */
        if (connectionsCount >= MAX_CLIENTS){
            write(scom, ERRORMAXUSER, sizeof(int));
            shutdown(scom, 2);
            close(scom);
        } else {
            struct clientChain newClient;
            struct clientChain *c;
            c = &clients;
            /* Creation d'un thread qui traite la requete */
            j = (j+1)%MAX_CLIENTS;
            pthread_create(&tid[j], 0, clientThread, scom);
            connectionsCount++;
            /* On ajoute dans la chaine le client */
            newClient.clientId = tid[j];
            newClient.next = NULL;
            while (c.next != NULL) {
                c = c->next;
            }
            c->next = &newClient;
        }
        sleep(1);
    }

    /* A refaire */
    for (i=0; i < MAX_CLIENTS; i++)
        pthread_join(tid[i], 0);

    close(sc);
    return EXIT_SUCCESS;
}
