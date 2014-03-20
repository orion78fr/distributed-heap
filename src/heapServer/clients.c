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

    void *content = NULL;
    int msgType;
    int temp;

#if DEBUG
    printf("[Client %d] Connexion\n", pthread_self());
#endif

    /* Envoi de la taille du stack */

    msgType = MSG_HEAP_SIZE;
    if (write(sock, (void *) &msgType, sizeof(msgType)) < 0) {  /* Msg type */
        goto disconnect;
    }
    if (write(sock, &(parameters.heapSize), sizeof(parameters.heapSize)) <
        0) {
        /* Heap Size */
        goto disconnect;
    }

    /* Boucle principale */
    for (;;) {
        if (read(sock, (void *) &msgType, sizeof(msgType)) < 0) {       /* Msg type */
            goto disconnect;
        }

        /* Switch pour les différents types de messages */
        switch (msgType) {
        case MSG_HEAP_SIZE:     /* Clients should not send that */
            goto disconnect;
        case MSG_ALLOC: /* Allocation d'une variable */
            if (read(sock, (void *) &temp, sizeof(temp)) < 0) { /* Name size */
                goto disconnect;
            }
            content = malloc(temp * sizeof(char) + 1);
            ((char *) content)[temp] = '\0';
            if (read(sock, content, temp) < 0) {        /* Name */
                goto disconnect;
            }
            if (read(sock, (void *) &temp, sizeof(temp)) < 0) { /* Var size */
                goto disconnect;
            }
#if DEBUG
            printf("[Client %d] Allocation de %s de taille %d\n",
                   pthread_self(), content, temp);
#endif

            if (add_data(content, temp) != 0) {
                /* TODO Traitement erreur */
            }

            break;

        case MSG_ACCESS_READ:   /* Demande d'accès en lecture */
            if (read(sock, (void *) &temp, sizeof(temp)) < 0) { /* Name size */
                goto disconnect;
            }
            content = malloc(temp * sizeof(char) + 1);
            ((char *) content)[temp] = '\0';
            if (read(sock, content, temp) < 0) {        /* Name */
                goto disconnect;
            }
#if DEBUG
            printf("[Client %d] Demande d'accès en lecture de %s\n",
                   pthread_self(), content);
#endif

            /* TODO Demande d'accès */

            break;
        case MSG_ACCESS_WRITE:  /* Demande d'accès en écriture */
            if (read(sock, (void *) &temp, sizeof(temp)) < 0) { /* Name size */
                goto disconnect;
            }
            content = malloc(temp * sizeof(char) + 1);
            ((char *) content)[temp] = '\0';
            if (read(sock, content, temp) < 0) {        /* Name */
                goto disconnect;
            }
#if DEBUG
            printf("[Client %d] Demande d'accès en écriture de %s\n",
                   pthread_self(), content);
#endif

            /* TODO Demande d'accès */

            break;

        case MSG_RELEASE:       /* Relachement de la variable */
            if (read(sock, (void *) &temp, sizeof(temp)) < 0) { /* Name size */
                goto disconnect;
            }
#if DEBUG
            printf("[Client %d] Libération de %d\n", pthread_self(),
                   temp);
#endif

            /* TODO Libération */

            break;

        case MSG_FREE:          /* Désallocation d'une variable */
            if (read(sock, (void *) &temp, sizeof(temp)) < 0) { /* Name size */
                goto disconnect;
            }
            content = malloc(temp * sizeof(char) + 1);
            ((char *) content)[temp] = '\0';
            if (read(sock, content, temp) < 0) {        /* Name */
                goto disconnect;
            }
#if DEBUG
            printf("[Client %d] Désallocation de %s\n", pthread_self(),
                   content);
#endif

            /* TODO Désalloc */

            break;
        case MSG_ERROR: /* Message d'erreur */
            goto disconnect;    /* Pour l'instant, aucun traitement d'erreur */
        case MSG_DISCONNECT:    /* Message de déconnection */
            goto disconnect;
        default:                /* Unknown message code, version problem? */
            goto disconnect;
        }

        if (content != NULL) {
            free(content);
            content = NULL;
        }
    }

disconnect:

#if DEBUG
    printf("[Client %d] Déconnexion\n", pthread_self());
#endif

    if (content != NULL) {
        free(content);
    }
    /* Fermer la connexion */
    shutdown(sock, 2);
    close(sock);
    clientsConnected--;
    pthread_exit(NULL);
    return NULL;
}
