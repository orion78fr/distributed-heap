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

/*
#define PORTSERV 5500
#define HEAPSIZE 1024
#define MAX_CLIENTS 20
*/

struct heapData{
  pthread_mutex_t mutex;
  char *name;
  int offset;
  struct clientChain *readAccess;
  struct clientChain *writeAccess;
  struct clientChain *readWait;
  struct clientChain *writeWait;
  struct heapData *next;
};

struct clientChain{
  int clientId;
  int sock;
  struct clientChain *next;
};

struct parameters{
	int port;
	int maxClients;
	int heapSize;
} parameters;

// GTU : 256 char pour un message ça me paraît suffisant non?
#define MESSAGE_SIZE 256
enum msgTypes{
	MSG_HEAP_SIZE,
	MSG_ALLOC,
	MSG_ACCESS_READ,
	MSG_ACCESS_WRITE,
	MSG_RELEASE,
	MSG_FREE,
	MSG_ERROR
};

struct message{
	int msgType;
	union{
		int asInteger;
		char asString[MESSAGE_SIZE];
	} content;
};

int clientsConnected = 0;
//pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
struct clientChain *clients = NULL;

void *clientThread(void *arg) {
	int sock = (int) arg;
	struct message msg;

	// Envoi de la taille du stack
	msg.msgType = MSG_HEAP_SIZE;
	msg.content.asInteger = parameters.heapSize;
	if(write(sock, (void*) &msg, sizeof(msg)) < 0){
		// GTU : Comment traite-t-on les erreurs? On déconnecte le client?
	}

    for (;;){
        sleep(4);
    }

    // Fermer la connexion
    shutdown(sock,2);
    close(sock);
	clientsConnected--;
    pthread_exit(NULL);
	return NULL;
}

int main(int argc, char *argv[]){
    struct sockaddr_in sin; // Nom de la socket de connexion
    int sock; // Socket de connexion
    int sclient; // Socket du client

	parameters.port = PORTSERV;
	parameters.maxClients = MAX_CLIENTS;
	parameters.heapSize = HEAPSIZE;
	
	if(argc < 4){
		perror("Usage: server [Port] [Max client] [Heap Size]");
		exit(EXIT_FAILURE);
	}
	
	// Lecture des paramètres
	// TODO: Mettre des valeurs par défaut et remplir de manière dynamique ie quelque chose genre -p Port -n nbClient -s Size ?
	parameters.port = atoi(argv[1]);
	parameters.maxClients = atoi(argv[2]);
	parameters.heapSize = atoi(argv[3]);
	
	printf("Port : %d\n", parameters.port);
	printf("Max Clients : %d\n", parameters.maxClients);
	printf("Heap Size : %d\n", parameters.heapSize);

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


    // Lien et écoute de la socket
    if(bind(sock,(struct sockaddr *)&sin,sizeof(sin)) < 0){
        perror("bind");
        exit(EXIT_FAILURE);
    }
    listen(sock, parameters.maxClients);

    for (;;) {
		struct clientChain *newClient;
		
		printf("It's ok! I'm waiting!\n");
		
        // On accepte la connexion
        if((sclient = accept(sock, NULL, NULL))== -1){
            perror("accept");
            exit(EXIT_FAILURE);
        }

		// Ajout du client dans la chaîne de socket (ajout au début pour éviter le parcours)
        newClient = malloc(sizeof(struct clientChain));
		newClient->sock = sclient;
		newClient->next = clients;
		clients = newClient;
			
        // Création d'un thread pour traiter les requêtes
        pthread_create((pthread_t *) &(newClient->clientId), NULL, clientThread, (void *) sclient);
        clientsConnected++;
    } // GTU : Et comment on sort de là?

    while(clients != NULL){
        pthread_join((pthread_t) clients->clientId, 0);
		clients = clients->next;
	}
	
	shutdown(sock,2);
    close(sock);
    return EXIT_SUCCESS;
}
