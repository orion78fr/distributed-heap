#include "common.h"

int serversConnected = 1;
struct pollfd *poll_list;

/**
 * Thread du server
 * @param arg Socket de communication du server
 */
void *serverThread(void *arg)
{
    uint8_t msgType;


    //int sock = ((struct clientChain*)arg)->sock;
    //((struct clientChain*)arg)->clientId = pthread_self();
    //uint8_t msgType;

#if DEBUG
    printf("[Server id: %d] Connexion\n", ((struct serverChain*)arg)->serverId);
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
            struct serverChain *server = servers;
            for (i=0; i<serversConnected)
                if((poll_list[i].revents&POLLNVAL)==POLLNVAL){
#if DEBUG
    printf("POLLNVAL, server id = %d\n", server->serverId);
#endif 
                    /* traiter la deco server */
                    continue;
                }
                if ((poll_list[i].revents&POLLHUP) == POLLHUP){
#if DEBUG
    printf("POLLHUP, server id = %d\n", server->serverId);
#endif 
                    /* traiter la deco server */
                    continue;
                }
                if ((poll_list[i].revents&POLLIN) == POLLIN){
                    uint8_t msgtype;
                    if (read(poll_list[i].fd, &msgtype, sizeof(msgtype)) <= 0){
                        /* traiter le prob server */
                        continue;
                    }
                    /* Switch pour les différents types de messages */
                    switch (msgType) {
                    case MSG_TOTAL_REPLICATION: /* Replication totale */
                        if(snd_total_replication(poll_list[i].fd) == -1){
                            goto disconnect;
                        }
                        break;
                    case MSG_PARTIAL_REPLICATION:   /* Replication partielle */
                        if(do_partial_replication(poll_list[i].fd) == -1){
                            goto disconnect;
                        }
                        break;
                    case MSG_PING:  /* Ping server */
                        if(do_ping(poll_list[i].fd) == -1){
                            goto disconnect;
                        }
                        break;
                    case MSG_ERROR: /* Message d'erreur */
                    default:                /* Unknown message code, version problem? */
                        goto disconnect;
                    }








                server = server->next;
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
