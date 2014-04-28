#include "common.h"

int serversConnected = 0;

/**
 * Thread du server en mode clone
 * @param arg Socket de communication du server
 */
void *clientThread(void *arg)
{
    int backup = ((struct serverChain*)arg)->backup;
    int sock = ((struct serverChain*)arg)->sock;
    uint8_t msgType;

#if DEBUG
    printf("[Server id: %d, thread: %d] Connexion\n", ((struct serverChain*)arg)->serverId, pthread_self());
#endif

    if(backup){
        /* Boucle principale */
        for (;;) {
            if (read(sock, (void *) &msgType, sizeof(msgType)) <= 0) {       /* Msg type */
                goto disconnect;
            }

            /* Switch pour les différents types de messages */
            switch (msgType) {
            case MSG_DATA_REPLICATION: /* Allocation/Modification d'une variable */
                if(rcv_data_replication(sock) == -1){
                    goto disconnect;
                }
                break;
            case MSG_RELEASE_REPLICATION: /* Release d'une variable */
                if(rcv_release_replication(sock) == -1){
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
            case MSG_ERROR: /* Message d'erreur */
            default:                /* Unknown message code, version problem? */
                goto disconnect;
            }
        }
    }else{

        if(read(sock, &msgType, sizeof(msgType)) <= 0){
            goto disconnect;
        }

        if(msgType==MSG_TOTAL_REPLICATION){
            snd_total_replication(sock);
        }

        for(;;) {
            pthread_mutex_lock(&rep->mutex_server);
            if(rep->data!=NULL)
            {
                if(snd_data_replication(rep) <=0){
                    goto disconnect;
                }

                if (read(sock, (void *) &msgType, sizeof(msgType)) <= 0) {       /* Msg type */
                    goto disconnect;
                }

                if (msgType != MSG_ACK){
                    goto disconnect;
                }

                rep->modification= MSG_ACK;
                pthread_mutex_unlock(&rep->mutex_server);
                pthread_cond_signal(&rep->cond_server);
                
            }else if(rep->clientId!=NULL){
                if(snd_maj_client(rep) <=0){
                    goto disconnect;
                }

                if (read(sock, (void *) &msgType, sizeof(msgType)) <= 0) {       /* Msg type */
                    goto disconnect;
                }

                if (msgType != MSG_ACK){
                    goto disconnect;
                }

                rep->modification= MSG_ACK;
                pthread_mutex_unlock(&rep->mutex_server);
                pthread_cond_signal(&rep->cond_server);
            }else{
                pthread_cond_wait(&rep->cond_server, &rep->mutex_server);
            }
        }
    }

disconnect:

#if DEBUG
    printf("[Client %d] Déconnexion\n", pthread_self());
#endif

    /* Fermer la connexion */
    shutdown(sock, 2);
    close(sock);
    clientsConnected--;
    pthread_exit(NULL);
    return NULL;
}
