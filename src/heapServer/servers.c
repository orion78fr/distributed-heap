#include "common.h"

int serversConnected = 0;

/**
 * Thread du server en mode clone
 * @param arg Socket de communication du server
 */
void *serverThread(void *arg)
{
    struct clientChain *client;
    int sock = ((struct serverChain*)arg)->sock;
    uint8_t msgType;


#if DEBUG
    printf("[sock: %d, serverNum: %d, serverId: %d, backup: %d] Connexion\n",sock, parameters.serverNum, ((struct serverChain*)arg)->serverId, backup);
#endif

    if(parameters.backup){
#if DEBUG
    printf("[backup: %d]\n",parameters.backup);
#endif
        /* Boucle principale */
        for (;;) {

#if DEBUG
            printf("[attente msgMaj]\n");
#endif
            if (read(sock, (void *) &msgType, sizeof(msgType)) <= 0) {       /* Msg type */
                goto disconnect;
            }

#if DEBUG
            printf("[msgType: %d]\n",msgType);
#endif

            /* Switch pour les différents types de messages */
            switch (msgType) {
            case MSG_DATA_REPLICATION: /* Allocation/Modification d'une variable */
                if(rcv_data_replication(sock) == -1){
                    goto disconnect;
                }
                break;
            case MSG_RELEASE_WRITE_REPLICATION: /* Release d'une variable */
                if(rcv_release_write_replication(sock) == -1){
                    goto disconnect;
                }
                break;
            case MSG_RELEASE_READ_REPLICATION: /* Release d'une variable */
                if(rcv_release_read_replication(sock) == -1){
                    goto disconnect;
                }
                break;
            case MSG_FREE_REPLICATION:          /* Désallocation d'une variable */
                if(rcv_free_replication(sock) == -1){
                    goto disconnect;
                }
                break;
            case MSG_ADD_CLIENT:          /* Ajout d'un client */
                if(rcv_new_client(sock) == -1){
                    goto disconnect;
                }
                break;
            case MSG_RMV_CLIENT:          /* Suppression d'un client */
                if(rcv_rmv_client(sock) == -1){
                    goto disconnect;
                }
                break;
            case MSG_MAJ_ACCESS_READ:          /* Maj file access d'une variable */
                if(rcv_maj_access_read(sock) == -1){
                    goto disconnect;
                }
                break;
            case MSG_MAJ_ACCESS_WRITE:          /* Maj file access d'une variable */
                if(rcv_maj_access_write(sock) == -1){
                    goto disconnect;
                }
                break;
            case MSG_MAJ_WAIT_READ:          /* Maj file wait d'une variable */
                if(rcv_maj_wait_read(sock) == -1){
                    goto disconnect;
                }
                break;
            case MSG_MAJ_WAIT_WRITE:          /* Maj file wait d'une variable */
                if(rcv_maj_wait_read(sock) == -1){
                    goto disconnect;
                }
                break;
            case MSG_DEFRAG_REPLICATION:          /* Defragmentation */
                if(rcv_defrag_replication(sock) == -1){
                    goto disconnect;
                }
                break;
            case MSG_ERROR: /* Message d'erreur */
            default:                /* Unknown message code, version problem? */
                goto disconnect;
            }
            msgType = MSG_ACK;
            if (write(sock, (void *) &msgType, sizeof(msgType)) <= 0) {       /* Msg type */
                    goto disconnect;
            }
        }
    }else{
#if DEBUG
        printf("[backup: %d]\n",parameters.backup);
#endif

        for(;;) {
            pthread_mutex_lock(&rep->mutex_server);
            if(rep->data==NULL && rep->clientId == 0){
#if DEBUG
            printf("[wait rep]\n");
#endif
                pthread_cond_wait(&rep->cond_server, &rep->mutex_server);
            }
            if(rep->data!=NULL)
            {
#if DEBUG
                printf("[reveil et data !=NULL]\n");
#endif
                if(snd_data_replication(rep) <=0){
                    goto disconnect;
                }

                if (read_rep(sock, (void *) &msgType, sizeof(msgType)) <= 0) {       /* Msg type */
                    goto disconnect;
                }

                if (msgType != MSG_ACK){
                    goto disconnect;
                }

                pthread_mutex_lock(&ack->mutex_server);
                pthread_mutex_unlock(&rep->mutex_server);
                pthread_cond_signal(&ack->cond_server);
                pthread_mutex_unlock(&ack->mutex_server);

            }else if(rep->clientId!=0){
#if DEBUG
                printf("[reveil et clientId !=0]\n");
#endif
                if(snd_maj_client(rep) <=0){
                    goto disconnect;
                }

                if (read_rep(sock, (void *) &msgType, sizeof(msgType)) <= 0) {       /* Msg type */
                    goto disconnect;
                }

                if (msgType != MSG_ACK){
                    goto disconnect;
                }
                pthread_mutex_lock(&ack->mutex_server);
                pthread_mutex_unlock(&rep->mutex_server);
                pthread_cond_signal(&ack->cond_server);
                pthread_mutex_unlock(&ack->mutex_server);
            }
        }
    }

disconnect:

#if DEBUG
    printf("[Server %d] Déconnexion\n", parameters.serverNum);
#endif

    if(parameters.backup){
        parameters.backup=0;
    }


    free(servers->serverAddress);
    free(servers);
    servers=NULL;
    client=clients;
    while(client!=NULL){
        pthread_mutex_unlock(&rep->mutex_server);
        client=client->next;
    }

    
    pthread_mutex_lock(&ack->mutex_server);
    ack->modification=1;
    pthread_cond_broadcast(&ack->cond_server);
    pthread_mutex_unlock(&ack->mutex_server);

    /* Fermer la connexion */
    shutdown(sock, 2);
    close(sock);
    serversConnected--;
    pthread_exit(NULL);
    return NULL;
}
