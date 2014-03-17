#include "common.h"

int clientsConnected = 0;
struct clientChain *clients = NULL;

/**
 * Thread du client
 * @param arg Socket de communication du client
 */
void *clientThread(void *arg)
{
    int sock = (int) arg;
    struct message msg;

    /* Envoi de la taille du stack */
    msg.msgType = MSG_HEAP_SIZE;
    msg.content.asInteger = parameters.heapSize;
    if (write(sock, (void *) &msg, sizeof(msg)) < 0) {
	/* GTU : Comment traite-t-on les erreurs? On déconnecte le client? 
	 * Utiliser un goto pour gérer les erreurs? */
    }

    for (;;) {
	sleep(4);
    }

    /* Fermer la connexion */
    shutdown(sock, 2);
    close(sock);
    clientsConnected--;
    pthread_exit(NULL);
    return NULL;
}
