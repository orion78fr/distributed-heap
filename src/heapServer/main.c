#include "common.h"

int main(int argc, char *argv[])
{
    struct sockaddr_in sin;	/* Nom de la socket de connexion */
    int sock;			/* Socket de connexion */
    int sclient;		/* Socket du client */

    /* Parsing des arguments 
     * Voir 
     * http://www.gnu.org/software/libc/manual/html_node/Getopt.html#Getopt
     */
    if (parse_args(argc, argv)) {
	perror("Wrong args\n");
	exit(EXIT_FAILURE);
    }
#if DEBUG
    printf("Port : %d\n", parameters.port);
    printf("Max Clients : %d\n", parameters.maxClients);
    printf("Heap Size : %d\n", parameters.heapSize);
#endif

    /* Creation de la socket */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	perror("socket");
	exit(EXIT_FAILURE);
    }

    memset((void *) &sin, 0, sizeof(sin));
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(parameters.port);
    sin.sin_family = AF_INET;

    /* setsockopt(sc, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
     * GTU : Est-ce nécessaire? */

    /* Lien et écoute de la socket */
    if (bind(sock, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
	perror("bind");
	exit(EXIT_FAILURE);
    }
    listen(sock, parameters.maxClients);

    for (;;) {
	struct clientChain *newClient;

#if DEBUG
	printf("It's ok! I'm waiting!\n");
#endif

	/* On accepte la connexion */
	if ((sclient = accept(sock, NULL, NULL)) == -1) {
	    perror("accept");
	    exit(EXIT_FAILURE);
	}
#if DEBUG
	printf("New Client...\n");
	/* GTU : Ajouter des getname etc... pour le debug (savoir 
	 * qui se connecte) */
#endif

	if (clientsConnected > parameters.maxClients) {
	    struct message msgError;
	    msgError.msgType = MSG_ERROR;
	    msgError.content.asInteger = ERROR_SERVER_FULL;
	    write(sclient, (void *) &msgError, sizeof(msgError));
	    continue;
	}
	/* Ajout du client dans la chaîne de socket (ajout au début pour
	 * éviter le parcours) */
	newClient = malloc(sizeof(struct clientChain));
	newClient->sock = sclient;
	newClient->next = clients;
	clients = newClient;

	/* Création d'un thread pour traiter les requêtes */
	pthread_create((pthread_t *) & (newClient->clientId), NULL,
		       clientThread, (void *) sclient);
	clientsConnected++;
    }
    /* GTU : Et comment on sort de là? Signaux puis envoi d'un END 
     * a tout les clients */

    while (clients != NULL) {
	pthread_join((pthread_t) clients->clientId, 0);
	clients = clients->next;
    }

    shutdown(sock, 2);
    close(sock);
    return EXIT_SUCCESS;
}
