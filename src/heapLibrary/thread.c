#include "distributedHeap.h"

/**
 * Thread de la librairie gérant les lectures et les ping/pong
 * @param aucun
 */
void *data_thread(void *arg){
    msgtypeClient = MSG_TYPE_NULL;

    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

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
        struct dheapServer *dstmp;

        /* On vérifie s'il faut ping une machine ou s'il faut la déconnecter */
        dstmp = dheapServers;
        while (dstmp != NULL){
            /* On vérifie s'il y a un ping en attente */
            if (dstmp->lastPing != 0 && dstmp->lastPing < (time(NULL) - (PONG_TIMEOUT*2))){
                if (dstmp->id == heapInfo->mainId){
                    if (pthread_mutex_trylock(&mainlock) != 0){
                        msgtypeClient = MSG_RETRY;
                        switchMain();
                        pthread_cond_wait(&readcond, &readlock);
                    } else {
                        pthread_mutex_unlock(&mainlock);
                    }
                }
                setDownAndSwitch(dstmp->id);
                dstmp = dstmp->next;
                continue;
            }

            /* On vérifie s'il faut pinger */
            if (dstmp->status == 1 && dstmp->lastMsgTime < (time(NULL) - TIMEOUT_BEFORE_PING)){
                uint8_t msgtype = MSG_PING;
                if (write(heapInfo->sock, &msgtype, sizeof(msgtype)) == -1){
                    if (dstmp->id == heapInfo->mainId){
                        if (pthread_mutex_trylock(&mainlock) != 0){
                            msgtypeClient = MSG_RETRY;
                            switchMain();
                            pthread_cond_wait(&readcond, &readlock);
                        } else {
                            pthread_mutex_unlock(&mainlock);
                        }
                    }
                    setDownAndSwitch(dstmp->id);
                    dstmp = dstmp->next;
                    continue;
                }
                dstmp->lastPing = time(NULL);
            }

            dstmp = dstmp->next;
        }

        /* On essaye de se reconnecter aux serveurs offline */
        reconnectServers();

        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

        retval = poll(poll_list, countServersOnline, PONG_TIMEOUT*1000);
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        if (retval < 0){
            perror("poll");
            exit(EXIT_FAILURE); 
        } else if (retval > 0){
            for (i=0; i < countServersOnline; i++){
                struct dheapServer *ds;
                ds = getServerBySock(poll_list[i].fd);

                if ((poll_list[i].revents&POLLNVAL) == POLLNVAL){
#if DEBUG
                    printf("POLLNVAL, id = %d\n", ds->id);
#endif
                    if (ds->id == heapInfo->mainId){
                        if (pthread_mutex_trylock(&mainlock) != 0){
                            msgtypeClient = MSG_RETRY;
                            switchMain();
                            pthread_cond_wait(&readcond, &readlock);
                        } else {
                            pthread_mutex_unlock(&mainlock);
                        }
                    }
                    setDownAndSwitch(ds->id);
                    continue;
                }
                if ((poll_list[i].revents&POLLHUP) == POLLHUP){
#if DEBUG
                    printf("POLLHUP, id = %d\n", ds->id);
#endif
                    if (ds->id == heapInfo->mainId){
                        if (pthread_mutex_trylock(&mainlock) != 0){
                            msgtypeClient = MSG_RETRY;
                            switchMain();
                            pthread_cond_wait(&readcond, &readlock);
                        } else {
                            pthread_mutex_unlock(&mainlock);
                        }
                    }
                    setDownAndSwitch(ds->id);
                    continue;
                }
                if ((poll_list[i].events&POLLOUT) == POLLOUT && (poll_list[i].revents&POLLOUT) == POLLOUT){
                    helloNotNew(ds->id); /* TODO: risque de blocage sur le read du ack */
                    setTime(ds->id);
                    continue; 
                }
                if ((poll_list[i].revents&POLLIN) == POLLIN){
                    uint8_t msgtype;
                    if (read(poll_list[i].fd, &msgtype, sizeof(msgtype)) <= 0){
                        setDownAndSwitch(ds->id);
                        continue;
                    }
                    setTime(ds->id);
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
#if DEBUG
                        printf("PING, id = %d\n", ds->id);
#endif
                        ds->lastPing = 0;
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

