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

#define PORTSERV 5500
#define ERRORMAXUSER 66
#define HEAPSIZE 1024
#define MAX_CLIENTS 20

struct parameters{
	int port;
	int maxClients;
} parameters;

int clientsConnected = 0;
struct clientChain *clients = NULL;

int clientThread(int sc) {
    /* Lire le message */
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
    struct sockaddr_in sin;  // Nom de la socket de connexion
    struct sockaddr_in exp;  // Nom de la socket du client
    int sock ;               // Socket de connexion
    int sclient;		     // Socket du client
    int fromlen = sizeof(exp);
    int j;
    int tid[MAXUSER];

	
	if(argc < 3){
		perror("Usage: server [Port] [Max client]");
		exit(EXIT_FAILURE);
	}
	
	// Lecture des paramètres
	parameters.port = itoa(argv[1]);
	parameters.maxClients = itoa(argv[2]);

    // Creation de la socket
    if ((sock = socket(AF_INET,SOCK_STREAM,0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset((void*)&sin, 0, sizeof(sin));
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(parameters.port);
    sin.sin_family = AF_INET;

    // setsockopt(sc, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
	// Est-ce nécessaire?


    /* nommage */
    if(bind(sock,(struct sockaddr *)&sin,sizeof(sin)) < 0){
        perror("bind");
        exit(EXIT_FAILURE);
    }
	
    listen(sock, parameters.maxClients);

    j = 0;
    for (;;) {
        // On accepte la connexion
        if((sclient = accept(sock, (struct sockaddr *)&exp, &fromlen))== -1){
            perror("accept");
            exit(EXIT_FAILURE);
        }

		// Ajout du client dans la chaîne de socket (ajout au début pour éviter le parcours)
        struct clientChain *newClient = malloc(sizeof(struct clientChain));
		newClient->sock = sclient;
		newClient->next = clients;
		clients = newClient;
			
        // Création d'un thread qui traite la requête
        j = (j+1)%MAX_CLIENTS;
        pthread_create(&tid[j], 0, clientThread, sclient);
        connectionsCount++;
        }
    } /* GTU : Et comment on sort de là? */

    for (i=0; i < MAX_CLIENTS; i++){
        pthread_join(tid[i], 0);
	}
	
	shutdown(sc,2);
    close(sc);
    return EXIT_SUCCESS;
}
