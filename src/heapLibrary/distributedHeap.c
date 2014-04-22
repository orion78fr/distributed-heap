#include "distributedHeap.h"

/**
 * Partie commune des fonctions de communication avec le serveur
 * permettant de traiter les erreurs et les acquittements
 * @return enum errorCodes
 */
int receiveAck(uint8_t msgtype){
    return receiveAckPointer(&msgtype);
}

int receiveAckPointer(uint8_t *msgtypeP){
    uint8_t msgtype;
    uint8_t msgtypeReponse;

#if DEBUG
    printf("Appel receiveAck()\n");
#endif 

    msgtype = *msgtypeP;

    pthread_mutex_lock(&readlock);

    /* On receptionne le type du message de réponse */
    msgtypeReponse = msgtypeClient;
    msgtypeClient = MSG_TYPE_NULL;

#if DEBUG
    printf("Réponse recu: %d\n", msgtypeReponse);
#endif 

    /* On verifie s'il y a eu une erreur ou non */
    /* TODO: && msgtype == MSG_RELEASE ?? */
    if (msgtypeReponse == MSG_ERROR){
        /* On récupère le code d'erreur */
        if (read(heapInfo->sock, &msgtypeReponse, sizeof(msgtypeReponse)) <= 0){
            return DHEAP_ERROR_CONNECTION;
        }

        /* On retourne le code d'erreur */
        return msgtypeReponse;
    } else {
        if (msgtypeReponse != msgtype){
            if (msgtype == MSG_ACCESS_READ && msgtypeReponse == MSG_ACCESS_READ_MODIFIED){
                *msgtypeP = MSG_ACCESS_READ_MODIFIED;
                return DHEAP_SUCCESS;
            } else if (msgtype == MSG_ACCESS_WRITE && msgtypeReponse == MSG_ACCESS_WRITE_MODIFIED){
                *msgtypeP = MSG_ACCESS_WRITE_MODIFIED;
                return DHEAP_SUCCESS;
            } else {
                return DHEAP_ERROR_UNEXPECTED_MSG;
            }
        } else {
            /* return success */
            return DHEAP_SUCCESS;
        }
    }
#if DEBUG
    printf("Fin de receiveAck()\n");
#endif 
}

void unlockAndSignal(){
    pthread_cond_signal(&readcond);
    pthread_mutex_unlock(&readlock);
    pthread_mutex_lock(&readylock);
    pthread_mutex_unlock(&readylock);
}

int checkError(){
    int ret;
    if (dheapErrorNumber == NULL)
        return DHEAP_SUCCESS;

    ret = *dheapErrorNumber;
    free(dheapErrorNumber);
    dheapErrorNumber = NULL;

    return ret;
}

void setError(uint8_t e){
    if (dheapErrorNumber == NULL)
        dheapErrorNumber = malloc(sizeof(uint8_t));
    *dheapErrorNumber = e;
}


/**
 * Thread de la librairie gérant les lectures et les ping/pong
 * @param aucun
 */
void *data_thread(void *arg){
    /* TODO: à changer pour gérer plusieurs serveurs */

    msgtypeClient = MSG_TYPE_NULL;

#if DEBUG
    printf("Création du thread()\n");
#endif

    /* Création du poll */
    buildPollList();

    /* On bloque la lecture sur le socket (mainId) */
    pthread_mutex_lock(&readlock);
    pthread_cond_signal(&readcond);
    pthread_cond_wait(&readcond, &readlock);

    /* Boucle avec le poll */
    while (1){
        int retval, i;
        /* TODO: here reconnect servers */
        retval = poll(poll_list, countServersOnline, -1);
        if (retval < 0){
            perror("poll");
            exit(EXIT_FAILURE); 
        }
        if (retval > 0){
            for (i=0; i < countServersOnline; i++){
                struct dheapServer *ds;
                ds = getServerBySock(poll_list[i].fd);

                if ((poll_list[i].revents&POLLNVAL) == POLLNVAL){
#if DEBUG
    printf("POLLNVAL, id = %d\n", ds->id);
#endif 
                    setDownAndSwitch(ds->id);
                    continue;
                }
                if ((poll_list[i].revents&POLLHUP) == POLLHUP){
#if DEBUG
    printf("POLLHUP, id = %d\n", ds->id);
#endif 
                    setDownAndSwitch(ds->id);
                    continue;
                }
                if ((poll_list[i].events&POLLOUT) == POLLOUT && (poll_list[i].revents&POLLOUT) == POLLOUT){
                    helloNotNew(ds->id);
                    continue; 
                }
                if ((poll_list[i].revents&POLLIN) == POLLIN){
                    uint8_t msgtype;
                    if (read(poll_list[i].fd, &msgtype, sizeof(msgtype)) <= 0){
                        setDownAndSwitch(ds->id);
                        continue;
                    }
                    if (msgtype == MSG_ALLOC || msgtype == MSG_ACCESS_READ
                        || msgtype == MSG_ACCESS_READ || msgtype == MSG_ACCESS_READ_MODIFIED
                        || msgtype == MSG_ACCESS_WRITE || msgtype == MSG_ACCESS_WRITE_MODIFIED
                        || msgtype == MSG_RELEASE || msgtype == MSG_FREE
                        || msgtype == MSG_ERROR){
                        msgtypeClient = msgtype;
#if DEBUG
    printf("POLLIN, id = %d, msgtype = %d\n", ds->id, msgtypeClient);
#endif
                        pthread_mutex_lock(&readylock);
                        pthread_cond_wait(&readcond, &readlock);
                        pthread_mutex_unlock(&readylock);
                        continue;
                    } else if (msgtype == MSG_DISCONNECT){
#if DEBUG
                        printf("DISCONNECT, id = %d\n", ds->id);
#endif
                        setDownAndSwitch(ds->id);
                        continue;
                    } else if (msgtype == MSG_PING){

                    } else if (msgtype == MSG_ADD_SERVER){
                        /* Comme on modifie la poll_list, on repart dans la boucle */
                        continue;
                    } else if (msgtype == MSG_REMOVE_SERVER){

                    } else {
                        exit_data_thread(DHEAP_ERROR_UNEXPECTED_MSG);
                    }
                }
            }
        }
    }

    pthread_mutex_unlock(&readlock);
}

void setDownAndSwitch(uint8_t sid){
#if DEBUG
    printf("Appel de setDownAndSwitch(%" PRIu8 ")\n", sid);
#endif 
    setServerDown(sid);
    if (sid == heapInfo->mainId)
        switchMain();
}

/**
 * Ferme le thread et laisse un message d'erreur
 * @param errorCodes
 */
void exit_data_thread(int e){
#if DEBUG
    printf("Appel de exit_data_thread(%d)\n", e);
#endif 
    if (dheapErrorNumber == NULL){
        dheapErrorNumber = malloc(sizeof(int));
    }
    *dheapErrorNumber = e;
    pthread_exit(NULL);
}

