#include "distributedHeap.h"

/**
 * Créer la poll list à partir de la liste de serveurs et de leurs status
 */
void buildPollList(){
    struct pollfd *old, *new;
    struct dheapServer *ds;
    int j = 0;

#if DEBUG
    printf("Appel de buildPollList()\n");
#endif 

    old = poll_list;
    
    pthread_mutex_lock(&polllock);

    countServersOnline = 0;
    ds = dheapServers;
    while (ds != NULL){
        if ((ds->status == 1 || ds->status == 2) && ds->sock != -1)
            countServersOnline++;
        ds = ds->next;
    }

    if (countServersOnline == 0){
#if DEBUG
        printf("ERROR: 0 servers online\n");
#endif 
        free(old);
        exit_data_thread(DHEAP_ERROR_CONNECTION);
        return;
    }

    new = malloc(sizeof(struct pollfd)*countServersOnline);
    ds = dheapServers;
    while(ds != NULL){
        if (j > countServersOnline){
            free(new);
            exit(EXIT_FAILURE);
        }
        if (ds->status == 1 && ds->sock != -1){
            new[j].fd = ds->sock;
            new[j].events = POLLHUP | POLLIN | POLLNVAL;
            j++;
        } else if (ds->status == 2 && ds->sock != -1){
            new[j].fd = ds->sock;
            new[j].events = POLLOUT | POLLHUP | POLLNVAL;
            j++;
        }
        ds = ds->next;
    }

    poll_list = new;
    free(old);

    pthread_mutex_unlock(&polllock);

#if DEBUG
    printf("Fin de buildPollList(), %d servers online\n", countServersOnline);
#endif 
}

/**
 * Ferme la connexion d'un serveur et le place sur down,
 * puis rebuild la poll list si on est le thread de la librairie.
 * Si on ne l'est pas, alors on ne veut pas interférer avec un poll
 * qui pourrait être en cours.
 * @param serveur id, buildPollList = 0 -> ne pas rebuild, 1 -> rebuild.
 */
void setServerDownInternal(uint8_t id, int doBuildPollList){
    struct dheapServer *tmp;
    uint8_t msgtype;

    tmp = dheapServers;
    pthread_mutex_lock(&polllock);
    while (tmp->id != id && tmp != NULL)
        tmp = tmp->next;

    if (tmp == NULL)
        exit(EXIT_FAILURE);

    if (tmp->sock == -1 && tmp->status == 0){
        return;
    }

    msgtype = MSG_DISCONNECT;
    /* Pas de vérification d'erreur nécessaire pour le write */
    /* write(tmp->sock, &msgtype, sizeof(msgtype)); --> Envoie un SIGPIPE */
    close(tmp->sock);
    tmp->sock = -1;
    tmp->status = 0;
    tmp->lastMsgTime = 0;
    tmp->lastPing = 0;

    pthread_mutex_unlock(&polllock);

    if (pthread_equal(pthread_self(), *dheap_tid) == 1 && doBuildPollList == 1)
        buildPollList();
}

void setServerDown(uint8_t id){
    setServerDownInternal(id, 1);
}
void setServerDownNoRebuild(uint8_t id){
    setServerDownInternal(id, 0);
}

/**
 * Permet de changer de serveur main en cherchant un serveur avec le status à 1
 * @return 0 si un nouveau main a été trouvé, exit le thread sinon
 */
int switchMain(){
    struct dheapServer *ds;
#if DEBUG
        printf("Appel de switchMain()\n");
#endif 
    ds = dheapServers;
    while (ds != NULL && ds->status != 1){
        if (ds->status == 1){
            heapInfo->mainId = ds->id;
            heapInfo->sock = ds->sock;
            return 0;
        }
        ds = ds->next;
    }
    exit_data_thread(DHEAP_ERROR_CONNECTION);
    return -1;
}

