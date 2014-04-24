#include "common.h"

int serversConnected = 0;
pthread_mutex_t schainlock = PTHREAD_MUTEX_INITIALIZER;

/**
 * Thread du server
 * @param arg Socket de communication du server
 */
void *serverThread(void *arg)
{
    struct serverChain *serv=(struct serverChain*)arg;
    //int sock = ((struct serverChain*)arg)->sock;
    ((struct serverChain*)arg)->serverId = pthread_self();
    uint8_t msgType;

#if DEBUG
    printf("[Server %d] Connexion\n", pthread_self());
#endif


    /* Boucle principale */
    for (;;) {
        if (read(sock, (void *) &msgType, sizeof(msgType)) <= 0) {       /* Msg type */
            goto disconnect;
        }

        /* Switch pour les différents types de messages */
        switch (msgType) {
        case MSG_ALLOC: /* Allocation d'une variable */
            if(do_alloc(sock) == -1){
                goto disconnect;
            }
            break;
        case MSG_ACCESS_READ:   /* Demande d'accès en lecture */
            if(do_access_read(sock) == -1){
                goto disconnect;
            }
            break;
        case MSG_ACCESS_WRITE:  /* Demande d'accès en écriture */
            if(do_access_write(sock) == -1){
                goto disconnect;
            }
            break;
        case MSG_ACCESS_READ_BY_OFFSET:
            if(do_access_read_by_offset(sock) == -1){
                goto disconnect;
            }
            break;
        case MSG_ACCESS_WRITE_BY_OFFSET:
            if(do_access_write_by_offset(sock) == -1){
                goto disconnect;
            }
            break;
        case MSG_RELEASE:       /* Relachement de la variable */
            if(do_release(sock) == -1){
                goto disconnect;
            }
            break;
        case MSG_FREE:          /* Désallocation d'une variable */
            if(do_free(sock) == -1){
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
