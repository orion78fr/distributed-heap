#include "common.h"

int rcv_total_replication(int sock){
    uint8_t taille;
    uint8_t msgType;

    /* Replication des données */
#if DEBUG
    printf("debut rcv total rep\n");
#endif
    if (read(sock, (void *) &msgType, sizeof(msgType)) <= 0) { /* nouvelle donnee a recevoir */
        goto disconnect;
    }

    while(msgType==MSG_REP_CONTINUE){
        struct clientChainRead *prevRead;
        struct clientChainWrite *prevWrite;
        struct heapData *newData = malloc(sizeof(struct heapData));
        int sum, offsetSum;

        if(read(sock, (void *) &taille, sizeof(taille)) <= 0){
            goto disconnect;
        }

        newData->name = malloc(taille);

        if(read(sock, newData->name, taille) <= 0){
            goto disconnect;
        }

        if(read(sock, &newData->offset, sizeof(newData->offset)) <= 0){
            goto disconnect;
        }

        if(read(sock, &newData->size, sizeof(newData->size)) <= 0){
            goto disconnect;
        }

        if(read(sock, theHeap + newData->offset, newData->size) <= 0){
            goto disconnect;
        }

        newData->readAccess = NULL;
        newData->writeAccess = NULL;
        newData->readWait = NULL;
        newData->writeWait = NULL;
        pthread_mutex_init(&(newData->mutex), NULL);
        pthread_cond_init(&(newData->readCond), NULL);

        if(read(sock, &newData->readAccessSize, sizeof(newData->readAccessSize)) <= 0){
                goto disconnect;
        }
        int i;
        prevRead = newData->readAccess;
        for(i=0; i<newData->readAccessSize; i++){
            struct clientChainRead *temp;
            temp = malloc(sizeof(struct clientChainRead));
            if(read(sock, &temp->clientId, sizeof(temp->clientId)) <= 0){
                goto disconnect;
            }
            if(i!=0){
                prevRead->next=temp;
            }
            prevRead=temp;
        }

        if(read(sock, &newData->readWaitSize, sizeof(newData->readWaitSize)) <= 0){
                goto disconnect;
        }

        prevRead = newData->readWait;
        for(i=0; i<newData->readWaitSize; i++){
            struct clientChainRead *temp;
            temp = malloc(sizeof(struct clientChainRead));
            if(read(sock, &temp->clientId, sizeof(temp->clientId)) <= 0){
                goto disconnect;
            }
            if(i!=0){
                prevRead->next=temp;
            }
            prevRead=temp;
        }

        if(read(sock, &newData->writeAccessSize, sizeof(newData->writeAccessSize)) <= 0){
                goto disconnect;
        }

        prevWrite = newData->writeAccess;;
        for(i=0; i<newData->writeAccessSize; i++){
            struct clientChainWrite *temp;
            temp = malloc(sizeof(struct clientChainWrite));
            if(read(sock, &temp->clientId, sizeof(temp->clientId)) <= 0){
                goto disconnect;
            }
            pthread_cond_init(&(temp->cond), NULL);
            if(i!=0){
                prevWrite->next=temp;
            }
            prevWrite=temp;
        }

        if(read(sock, &newData->writeWaitSize, sizeof(newData->writeWaitSize)) <= 0){
                goto disconnect;
        }

        prevWrite = newData->writeWait;
        for(i=0; i<newData->writeWaitSize; i++){
            struct clientChainWrite *temp;
            temp = malloc(sizeof(struct clientChainWrite));
            if(read(sock, &temp->clientId, sizeof(temp->clientId)) <= 0){
                goto disconnect;
            }
            pthread_cond_init(&(temp->cond), NULL);
            if(i!=0){
                prevWrite->next=temp;
            }
            prevWrite=temp;
        }

        sum = getHashSum(newData->name);
        offsetSum = newData->offset % parameters.hashSize;

        pthread_mutex_lock(&hashTableMutex);

        newData->next = hashTable[sum];
        hashTable[sum] = newData;

        offsetSum = newData->offset % parameters.hashSize;
        newData->nextOffset = hashTableOffset[offsetSum];
        hashTableOffset[offsetSum] = newData;

        memset(theHeap + newData->offset, 0, newData->size);

        pthread_mutex_unlock(&hashTableMutex);

        if (read(sock, (void *) &msgType, sizeof(msgType)) <= 0) { /* nouvelle donnee a recevoir */
            goto disconnect;
        }
    }
    /* envoi d'un message pour confirmer la replication totale */
    if(send_data(sock, MSG_ACK, 0)<0){
        goto disconnect;
    }

#if DEBUG
    printf("fin rcv_total_replication\n");
#endif

    return 1;
    disconnect:
    return -1;
}

int rcv_data_replication(int sock){
    uint8_t taille;
    char *nom;
    struct heapData *data;

    if (read(sock, (void *) &taille, sizeof(taille)) <= 0) { /* Name size */
        goto disconnect;
    }

    nom = malloc((taille + 1) * sizeof(char));
    if(nom == NULL){
        goto disconnect;
    }

    nom[taille] = '\0';

    if (read(sock, nom, taille) <= 0) {        /* Name */
        goto disconnect;
    }

    data = get_data(nom);

    if(data == NULL){/* création de la var */
        struct freeListChain *tempFreeList;
        struct freeListChain *prevFreeList = NULL;

#if DEBUG
    printf("[Server %d] Demande creation(replication partielle) de %s\n",
           pthread_getspecific(id), nom);
#endif
        data = malloc(sizeof(struct heapData));
        data->name=nom;

        if(read(sock, &data->offset, sizeof(data->offset)) <= 0){
            goto disconnect;
        }

        if(read(sock, &data->size, sizeof(data->size)) <= 0){
            goto disconnect;
        }


        if(read(sock, theHeap + data->offset, data->size) <= 0){
            goto disconnect;
        }

        pthread_mutex_lock(&freeListMutex);
        tempFreeList = freeList;
        while(tempFreeList->startOffset != data->offset){
            prevFreeList = tempFreeList;
            tempFreeList = tempFreeList->next;
        }
        if(tempFreeList->startOffset != data->offset){
            goto disconnect;
        }else{
            tempFreeList->startOffset += data->size;
            tempFreeList->size -= data->size;
            if(tempFreeList->size == 0){
                if(prevFreeList != NULL){
                    prevFreeList->next = tempFreeList->next;
                } else {
                    freeList = tempFreeList->next;
                }
                free(tempFreeList);
            }
        }

        pthread_mutex_unlock(&freeListMutex);

        data->readAccessSize=0;
        data->readWaitSize=0;
        data->writeAccessSize=0;
        data->writeWaitSize=0;
        data->readAccess=NULL;
        data->writeAccess=NULL;
        data->readWait=NULL;
        data->writeWait=NULL;


        pthread_mutex_init(&(data->mutex), NULL);
        pthread_cond_init(&(data->readCond), NULL);

        int sum = getHashSum(data->name);
        int offsetSum = data->offset % parameters.hashSize;

        pthread_mutex_lock(&hashTableMutex);

        data->next = hashTable[sum];
        hashTable[sum] = data;

        offsetSum = data->offset % parameters.hashSize;
        data->nextOffset = hashTableOffset[offsetSum];
        hashTableOffset[offsetSum] = data;

        memset(theHeap + data->offset, 0, data->size);

        pthread_mutex_unlock(&hashTableMutex);


    } else {/* mis à jour de la var */
#if DEBUG
    printf("[Server %d] Demande mis a jour(replication partielle) de %s\n",
           pthread_getspecific(id), data->name);
#endif

        if(read(sock, &data->offset, sizeof(data->offset)) <= 0){
            goto disconnect;
        }
        if(read(sock, &data->size, sizeof(data->size)) <= 0){
            goto disconnect;
        }
        if(read(sock, theHeap + data->offset, data->size) <= 0){
            goto disconnect;
        }
    }
#if DEBUG
    printf("fin rcv_data_replication\n");
#endif
    return 1;
    disconnect:
    if(nom != NULL){
        free(nom);
    }
    if(data != NULL){
        free(data);
    }
    return -1;
}

int rcv_release_read_replication(int sock){
    uint8_t taille;
    uint16_t id;
    char *nom;
    struct heapData *data;
    struct clientChainRead *temp=NULL, *prev=NULL;

    if (read(sock, (void *) &taille, sizeof(taille)) <= 0) { /* Name size */
        goto disconnect;
    }

    nom = malloc((taille + 1) * sizeof(char));
    if(nom == NULL){
        goto disconnect;
    }

    nom[taille] = '\0';

    if (read(sock, nom, taille) <= 0) {        /* Name */
        goto disconnect;
    }

    if (read(sock, &id, sizeof(id)) <= 0){
        goto disconnect;
    }

    data = get_data(nom);

    if(data->writeAccess!=NULL){
        goto disconnect;
    }else{
        temp=data->readAccess;
        while(temp->clientId != id){
            prev=temp;
            temp=temp->next;
        }
        if(temp!=NULL){
            if(prev!=NULL){
                prev->next=temp->next;
            }else{
                data->readAccess= temp->next;
            }
            free(temp);
            data->readAccessSize--;
        }
    }

    return 1;
    disconnect:
    if(nom!=NULL){
        free(nom);
    }
    return -1;
}

int rcv_release_write_replication(int sock){
    uint8_t taille;
    uint16_t id;
    char *nom;
    struct heapData *data;

    if (read(sock, (void *) &taille, sizeof(taille)) <= 0) { /* Name size */
        goto disconnect;
    }

    nom = malloc((taille + 1) * sizeof(char));
    if(nom == NULL){
        goto disconnect;
    }

    nom[taille] = '\0';

    if (read(sock, nom, taille) <= 0) {        /* Name */
        goto disconnect;
    }

    if (read(sock, &id, sizeof(id)) <= 0){
        goto disconnect;
    }

    data = get_data(nom);

    if(data->writeAccess!=NULL){
        data->writeAccessSize--;
        free(data->writeAccess);
        data->writeAccess=NULL;
    }

    return 1;
    disconnect:
    if(nom!=NULL){
        free(nom);
    }
    return -1;
}

int rcv_free_replication(int sock){
    uint8_t taille;
    char *nom;

    if (read(sock, (void *) &taille, sizeof(taille)) <= 0) { /* Name size */
        goto disconnect;
    }

    nom = malloc((taille + 1) * sizeof(char));
    if(nom == NULL){
        goto disconnect;
    }

    nom[taille] = '\0';

    if (read(sock, nom, taille) <= 0) {        /* Name */
        goto disconnect;
    }

    return remove_data(nom);
    disconnect:
    if(nom!=NULL){
        free(nom);
    }
    return -1;
}

int rcv_new_client(int sock){
    struct clientChain *newClient;
    /* Ajout du client dans la chaîne de socket (ajout au début pour
    * éviter le parcours) */
    newClient = malloc(sizeof(struct clientChain));
    newClient->sock = 0;
    newClient->next = clients;

    if (read(sock, &newClient->clientId, sizeof(newClient->clientId)) <= 0) {
        goto disconnect;
    }

    clients = newClient;
    clientsConnected++;

    return 1;
    disconnect:
    if(newClient!=NULL){
        free(newClient);
    }
    return -1;
}

int rcv_rmv_client(int sock){
    struct clientChain *newClient =  clients;
    struct clientChain *prev = NULL;
    uint16_t id;

    if (read(sock, &id, sizeof(id)) <= 0) {
        goto disconnect;
    }

    while(newClient->clientId != id){
        prev = newClient;
        newClient = newClient->next;
    }

    if(newClient!=NULL){
        if(prev==NULL){
            clients = newClient->next;
            free(newClient);
            clientsConnected--;
        }else{
            prev->next = newClient->next;
            free(newClient);
            clientsConnected--;
        }
        return 1;
    }
    return 0;
    disconnect:
    return -1;
}

int rcv_maj_access_read(int sock){
    uint8_t taille;
    char *nom;
    struct heapData *data;
    struct clientChainRead *temp;

    if (read(sock, (void *) &taille, sizeof(taille)) <= 0) { /* Name size */
        goto disconnect;
    }

    nom = malloc((taille + 1) * sizeof(char));
    if(nom == NULL){
        goto disconnect;
    }

    nom[taille] = '\0';

    if (read(sock, nom, taille) <= 0) {        /* Name */
        goto disconnect;
    }

    data = get_data(nom);
    temp = malloc(sizeof(struct clientChainRead));
    temp->next = data->readAccess;

    if (read(sock, &temp->clientId, sizeof(temp->clientId)) <= 0){
        goto disconnect;
    }

    data->readAccess = temp;
    data->readAccessSize++;
    return 1;
    disconnect:
    if(nom!=NULL){
        free(nom);
    }
    if(temp!=NULL){
        free(temp);
    }
    return -1;
}

int rcv_maj_access_write(int sock){
    uint8_t taille;
    char *nom;
    struct heapData *data;
    struct clientChainWrite *temp;

    if (read(sock, (void *) &taille, sizeof(taille)) <= 0) { /* Name size */
        goto disconnect;
    }

    nom = malloc((taille + 1) * sizeof(char));
    if(nom == NULL){
        goto disconnect;
    }

    nom[taille] = '\0';

    if (read(sock, nom, taille) <= 0) {        /* Name */
        goto disconnect;
    }

    data = get_data(nom);
    temp = malloc(sizeof(struct clientChainWrite));
    temp->next = data->writeAccess;

    if (read(sock, &temp->clientId, sizeof(temp->clientId)) <= 0){
        goto disconnect;
    }

    data->writeAccess = temp;
    data->writeAccessSize++;
    return 1;
    disconnect:
    if(nom!=NULL){
        free(nom);
    }
    if(temp!=NULL){
        free(temp);
    }
    return -1;
}

int rcv_maj_wait_read(int sock){
    uint8_t taille;
    char *nom;
    struct heapData *data;
    struct clientChainRead *temp;

    if (read(sock, (void *) &taille, sizeof(taille)) <= 0) { /* Name size */
        goto disconnect;
    }

    nom = malloc((taille + 1) * sizeof(char));
    if(nom == NULL){
        goto disconnect;
    }

    nom[taille] = '\0';

    if (read(sock, nom, taille) <= 0) {        /* Name */
        goto disconnect;
    }

    data = get_data(nom);
    temp = malloc(sizeof(struct clientChainRead));
    temp->next = data->readWait;

    if (read(sock, &temp->clientId, sizeof(temp->clientId)) <= 0){
        goto disconnect;
    }

    data->readWait = temp;
    data->readWaitSize++;
    return 1;
    disconnect:
    if(nom!=NULL){
        free(nom);
    }
    if(temp!=NULL){
        free(temp);
    }
    return -1;
}

int rcv_maj_wait_write(int sock){
    uint8_t taille;
    char *nom;
    struct heapData *data;
    struct clientChainWrite *temp;

    if (read(sock, (void *) &taille, sizeof(taille)) <= 0) { /* Name size */
        goto disconnect;
    }

    nom = malloc((taille + 1) * sizeof(char));
    if(nom == NULL){
        goto disconnect;
    }

    nom[taille] = '\0';

    if (read(sock, nom, taille) <= 0) {        /* Name */
        goto disconnect;
    }

    data = get_data(nom);
    temp = malloc(sizeof(struct clientChainWrite));
    temp->next = data->writeWait;

    if (read(sock, &temp->clientId, sizeof(temp->clientId)) <= 0){
        goto disconnect;
    }

    data->writeWait = temp;
    data->writeWaitSize++;
    return 1;
    disconnect:
    if(nom!=NULL){
        free(nom);
    }
    if(temp!=NULL){
        free(temp);
    }
    return -1;
}

int rcv_defrag_replication(int sock){
    uint8_t taille;
    char *nom;
    struct heapData *data, *prevData;
    struct freeListChain *tempFreeList;
    uint16_t oldOffset, newOffset;

    pthread_mutex_lock(&freeListMutex);

    if (read(sock, (void *) &taille, sizeof(taille)) <= 0) { /* Name size */
        goto disconnect;
    }

    nom = malloc((taille + 1) * sizeof(char));
    if(nom == NULL){
        goto disconnect;
    }

    nom[taille] = '\0';

    if (read(sock, nom, taille) <= 0) {        /* Name */
        goto disconnect;
    }

    data = get_data(nom);

    if (read(sock, (void *) &newOffset, sizeof(newOffset)) <= 0) { /* new offset */
        goto disconnect;
    }

    tempFreeList=freeList;
    while(tempFreeList->startOffset != (data->offset - tempFreeList->size) ){
        tempFreeList = tempFreeList->next;
    }

    acquire_write_lock(data);
    pthread_mutex_lock(&(data->mutex));
    memmove(theHeap+newOffset, theHeap+data->offset, data->size);
    oldOffset = data->offset;
    /* mis à jour du nouvel offset */
    data->offset=newOffset;

    /* mis à jour hashtable */
    pthread_mutex_lock(&hashTableMutex);
    prevData = NULL;
    data = hashTableOffset[oldOffset%parameters.hashSize];

    while(data != NULL && data->offset != newOffset){
        prevData = data;
        data = data->nextOffset;
    }
    if(data == NULL){
        /* WHAT ???!!!??? */
        return -42;
    }
    /* On retire de l'ancienne */
    if(prevData == NULL){
        hashTableOffset[oldOffset%parameters.hashSize] = data->nextOffset;
    } else {
        prevData->nextOffset = data->nextOffset;
    }
    /* On met dans la nouvelle */
    data->nextOffset = hashTableOffset[newOffset%parameters.hashSize];
    hashTableOffset[newOffset%parameters.hashSize] = data;

    pthread_mutex_unlock(&hashTableMutex);
    pthread_mutex_unlock(&(data->mutex));
    release_write_lock(data);

    if((tempFreeList->startOffset + tempFreeList->size) == tempFreeList->next->startOffset){
        /* On fusionne */
        tempFreeList->size += tempFreeList->next->size;
        tempFreeList->next = tempFreeList->next->next;
        free(tempFreeList->next);
    }

    pthread_mutex_unlock(&freeListMutex);

    return 1;
    disconnect:
    if(nom != NULL){
        free(nom);
    }
    if(data != NULL){
        free(data);
    }
    return -1;
}
