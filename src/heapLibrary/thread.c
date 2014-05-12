#include "distributedHeap.h"

/**
 * Thread de la librairie gérant les lectures de msgtypes,
 * les ping/pong, les déconnexions, les ajouts et suppressions
 * de serveurs, et les reconnexions
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
    pthread_mutex_lock(&readylock);
    pthread_mutex_lock(&readlock);
    pthread_cond_signal(&readcond);
    pthread_cond_wait(&readcond, &readlock);
    pthread_mutex_unlock(&readylock);

    /* Boucle avec le poll */
    while (1){
        int retval, i;
        struct dheapServer *dstmp;

        /* On vérifie s'il faut ping une machine ou s'il faut la déconnecter */
        dstmp = dheapServers;
        while (dstmp != NULL){
            /* On vérifie s'il y a un ping en attente */
            if (dstmp->lastPing != 0 && dstmp->lastPing < (time(NULL) - (PONG_TIMEOUT*2))){
#if DEBUG
                printf("NO PING, id = %d\n", dstmp->id);
#endif
                if (dstmp->id == heapInfo->mainId){
                    if (pthread_mutex_trylock(&mainlock) != 0){
                        msgtypeClient = MSG_RETRY;
                        setServerDownNoRebuild(dstmp->id);
                        switchMain();
                        pthread_mutex_lock(&readylock);
                        pthread_cond_wait(&readcond, &readlock);
                        pthread_mutex_unlock(&readylock);
                    } else {
                        pthread_mutex_unlock(&mainlock);
                    }
                } else {
                    setDownAndSwitch(dstmp->id);
                }
                dstmp = dstmp->next;
                continue;
            }

            /* On vérifie s'il faut pinger */
            if (dstmp->status == 1 && dstmp->lastPing == 0 && dstmp->lastMsgTime < (time(NULL) - TIMEOUT_BEFORE_PING)){
                uint8_t msgtype = MSG_PING;
                pthread_mutex_lock(&writelock);
#if DEBUG
                printf("PING, id = %d\n", dstmp->id);
#endif
                if (write(dstmp->sock, &msgtype, sizeof(msgtype)) == -1){
                    if (dstmp->id == heapInfo->mainId){
                        if (pthread_mutex_trylock(&mainlock) != 0){
                            msgtypeClient = MSG_RETRY;
                            setServerDownNoRebuild(dstmp->id);
                            switchMain();
                            pthread_mutex_lock(&readylock);
                            pthread_cond_wait(&readcond, &readlock);
                            pthread_mutex_unlock(&readylock);
                        } else {
                            pthread_mutex_unlock(&mainlock);
                        }
                    }
                    dstmp = dstmp->next;
                    pthread_mutex_unlock(&writelock);
                    continue;
                }
                pthread_mutex_unlock(&writelock);
                dstmp->lastPing = time(NULL);
            }

            if (dstmp->status == 2 && dstmp->lastConnect < (time(NULL) - PONG_TIMEOUT)){
#if DEBUG
                printf("Cant reconnect to id = %" PRIu8 "\n", dstmp->id);
#endif
                setServerDownNoRebuild(dstmp->id);
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
                            setServerDownNoRebuild(ds->id);
                            switchMain();
                            pthread_mutex_lock(&readylock);
                            pthread_cond_wait(&readcond, &readlock);
                            pthread_mutex_unlock(&readylock);
                        } else {
                            setDownAndSwitch(ds->id);
                            pthread_mutex_unlock(&mainlock);
                        }
                    } else {
                        setDownAndSwitch(ds->id);
                    }
                    break;
                }
                if ((poll_list[i].revents&POLLHUP) == POLLHUP){
#if DEBUG
                    printf("POLLHUP, id = %d\n", ds->id);
#endif
                    if (ds->id == heapInfo->mainId){
                        if (pthread_mutex_trylock(&mainlock) != 0){
                            msgtypeClient = MSG_RETRY;
                            setServerDownNoRebuild(ds->id);
                            switchMain();
                            pthread_mutex_lock(&readylock);
                            pthread_cond_wait(&readcond, &readlock);
                            pthread_mutex_unlock(&readylock);
                        } else {
                            setDownAndSwitch(ds->id);
                            pthread_mutex_unlock(&mainlock);
                        }
                    } else {
                        setDownAndSwitch(ds->id);
                    }
                    break;
                }
                if ((poll_list[i].events&POLLOUT) == POLLOUT && (poll_list[i].revents&POLLOUT) == POLLOUT){
#if DEBUG
                    printf("POLLOUT, id = %d\n", ds->id);
#endif
                    /* helloNotNew(ds->id); TODO: risque de blocage sur le read du ack
                    setTime(ds->id);
                    break; */
                }
                if ((poll_list[i].revents&POLLIN) == POLLIN){
                    uint8_t msgtype;
                    if (read(poll_list[i].fd, &msgtype, sizeof(msgtype)) <= 0){
                        setDownAndSwitch(ds->id);
                        break;
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
                    } else if (msgtype == MSG_HELLO){
#if DEBUG
                        printf("HELLO, id = %d\n", ds->id);
#endif
                        helloNotNew(ds->id); /* TODO: risque de blocage sur le read du ack */
                        setTime(ds->id);
                        break;
                    } else if (msgtype == MSG_DISCONNECT){
#if DEBUG
                        printf("DISCONNECT, id = %d\n", ds->id);
#endif
                        setDownAndSwitch(ds->id);
                        break;
                    } else if (msgtype == MSG_PING){
#if DEBUG
                        printf("PONG, id = %d\n", ds->id);
#endif
                        ds->lastPing = 0;
                        continue;
                    } else if (msgtype == MSG_ADD_SERVER){
                        /* Comme on modifie la poll_list, on repart dans la boucle */
                        uint8_t serverId, addlen;
                        char *servAddress;
                        uint16_t servPort;
#if DEBUG
                        printf("ADD_SERVER, id = %d\n", ds->id);
#endif
                        if (read(poll_list[i].fd, &serverId, sizeof(serverId)) <= 0){
                            setDownAndSwitch(ds->id);
                            break;
                        }
                        if (read(poll_list[i].fd, &addlen, sizeof(addlen)) <= 0){
                            setDownAndSwitch(ds->id);
                            break;
                        }
                        servAddress = malloc(sizeof(char)*addlen);
                        if (read(poll_list[i].fd, servAddress, sizeof(char)*addlen) <= 0){
                            setDownAndSwitch(ds->id);
                            break;
                        }
                        if (read(poll_list[i].fd, &servPort, sizeof(servPort)) <= 0){
                            setDownAndSwitch(ds->id);
                            break;
                        }
                        addserver(serverId, servAddress, servPort);
                        break;
                    } else if (msgtype == MSG_REMOVE_SERVER){
                        uint8_t serverId;
#if DEBUG
                        printf("REMOVE_SERVER, id = %d\n", ds->id);
#endif
                        if (read(poll_list[i].fd, &serverId, sizeof(serverId)) <= 0){
                            setDownAndSwitch(ds->id);
                            break;
                        }
                        removeServer(serverId);
                        break;
                    } else {
                        exit_data_thread(DHEAP_ERROR_UNEXPECTED_MSG);
                    }
                }
            }
        }
        continue;
    }

    pthread_mutex_unlock(&readlock);
}

/**
 * Ferme le thread et laisse un message d'erreur,
 * puis ferme les connexions et ferme le thread
 * @param errorCodes
 */
void exit_data_thread(uint8_t e){
#if DEBUG
    printf("Appel de exit_data_thread(%d)\n", e);
#endif
    setError(e);
    msgtypeClient = MSG_DISCONNECT_RELEASE_ALL;
    pthread_mutex_unlock(&readlock);
    pthread_mutex_unlock(&writelock);
    pthread_mutex_unlock(&mainlock);
    free(dheap_tid);
    close_data(); /* TODO: peut potentiellement créer des bugs */
    pthread_exit(NULL);
}

