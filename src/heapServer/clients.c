#include "common.h"

int clientsConnected = 0;
int retry = 0;

/**
 * Thread du client
 * @param arg Socket de communication du client
 */
void *clientThread(void *arg)
{
    struct clientChain *client,*prev;
    pthread_mutex_t mutex = ((struct clientChain*)arg)->mutex_sock;
    int sock = ((struct clientChain*)arg)->sock;
    uint8_t msgType;
    uint16_t *ids= malloc(sizeof(uint16_t));
    *ids=((struct clientChain*)arg)->clientId;
    pthread_setspecific(id, ids);
    

#if DEBUG
    printf("[Client id: %d, thread: %d] Connexion\n", pthread_getspecific(id), pthread_self());
#endif

    if(((struct clientChain*)arg)->newC){
        do_greetings(sock, ((struct clientChain*)arg)->clientId);

        if(servers!=NULL){
            snd_server_to_client(sock, ((struct clientChain*)arg));
        }
    }



    /* Boucle principale */
    for (;;) {
        pthread_mutex_lock(&mutex);

        if (read(sock, (void *) &msgType, sizeof(msgType)) <= 0) {       /* Msg type */
            goto disconnect;
        }

#if DEBUG
    printf("[msgType reçu: %d\n]", msgType);
#endif        

        if(msgType == MSG_RETRY){
            retry = 1;
            if (read(sock, (void *) &msgType, sizeof(msgType)) <= 0) {       /* Msg type */
                goto disconnect;
            }
        }

        /* Switch pour les différents types de messages */
        switch (msgType) {
        case MSG_PING:
            if(do_pong(sock) == -1){
                goto disconnect;
            }
            break;
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
        retry = 0;
        pthread_mutex_unlock(&mutex);
    }

disconnect:
    client = clients;
    prev = NULL;
#if DEBUG
    printf("[Client %d] Déconnexion\n", pthread_getspecific(id));
#endif

    while(client != NULL){
        if(client->clientId == pthread_getspecific(id)){
            if(prev!=NULL){
                prev->next = client->next;
            }
            clients = client->next;
            free(client);
        }

        prev=client;
        client=client->next;
    }



    /* Fermer la connexion */
    shutdown(sock, 2);
    close(sock);
    clientsConnected--;
    pthread_exit(NULL);
    return NULL;
}
