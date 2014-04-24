#include "common.h"

int serversConnected = 1;
struct pollfd *poll_list;

/**
 * Thread du server
 * @param arg Socket de communication du server
 */
void *serverThread(void *arg)
{
    struct serverChain *firstServer=(struct serverChain*)arg;
    //myself->serverId = pthread_self();
    uint8_t msgType;


    //int sock = ((struct clientChain*)arg)->sock;
    //((struct clientChain*)arg)->clientId = pthread_self();
    //uint8_t msgType;

#if DEBUG
    printf("[Server %d] Connexion\n", pthread_self());
#endif


    /* Boucle principale */
    for (;;) {
        int retval, i;

        retval = poll(poll_list, serversConnected, -1);
        if (retval < 0){
            perror("poll");
            return(EXIT_FAILURE);
        }
        if (retval > 0){
            struct serverChain *server = firstServer;
            for (i=0; i<serversConnected)














        }

        if (read(sock, (void *) &msgType, sizeof(msgType)) <= 0) {       /* Msg type */
            goto disconnect;
        }

        /* Switch pour les différents types de messages */
        switch (msgType) {
        case MSG_TOTAL_REPLICATION: /* Demande de replication totale */
            if(do_replication(sock) == -1){
                goto disconnect;
            }
            break;
        case MSG_ERROR: /* Message d'erreur */
        default:                /* Unknown message code, version problem? */
            goto disconnect;
        }
    }

disconnect:

#if DEBUG
    printf("[Server %d] Déconnexion\n", pthread_self());
#endif

    /* Fermer la connexion */
    shutdown(sock, 2);
    close(sock);
    clientsConnected--;
    pthread_exit(NULL);
    return NULL;
}
