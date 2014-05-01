#include "common.h"

struct clientChain *clients = NULL;
struct serverChain *servers = NULL;

/**
 * Envoie sur la socket sock les données passées en argument
 * @param sock Socket de communication
 * @param msgType Le type de message (voir messages.h)
 * @param nb Nombre de données à envoyer
 * @param ... les structures dataSend à envoyer
 * @return 0 si succès
 */
int send_data(int sock, uint8_t msgType, int nb, ...){
    va_list ap;
    int i;
    DS snd = {0,NULL};

#if DEBUG
    static int num = 0;
    printf("Envoi %d:\n", num++);
    printf("\tmsgType: %d\n", msgType);
#endif


    if(write(sock, &msgType, sizeof(msgType)) <= 0){
        return -1;
    }

    va_start(ap, nb);

    for(i=0;i<nb;i++){
        snd = va_arg(ap, DS);
#if DEBUG
        printf("\tSize %d\n", snd.taille);
#endif
        if(write(sock, snd.data, snd.taille) <= 0){
            return -1;
        }
    }

#if DEBUG
    printf("\n");
#endif

    va_end(ap);

    return 0;
}

int send_error(int sock, uint8_t errType){
    return send_data(sock, MSG_ERROR, 1, (DS){sizeof(errType), &errType});
}

/* TODO ajouter l'envoi des @ des autres serveurs et leurs id */
int do_greetings(int sock, uint16_t clientId){
    uint8_t msgType;


    if(send_data(sock, MSG_HELLO_NEW, 3,
                (DS){sizeof(parameters.serverNum), &(parameters.serverNum)},
                (DS){sizeof(clientId), &(clientId)},
                (DS){sizeof(parameters.heapSize), &(parameters.heapSize)})<0){
       goto disconnect;
    }
    return 0;
    disconnect:
    return -1;
}

int do_alloc(int sock){
    uint8_t taille;
    char *nom;
    uint64_t varSize;
    int err;

#if DEBUG
    printf("[Client %d] demande Allocation\n",
           pthread_getspecific(id));
#endif

    if (read(sock, (void *) &taille, sizeof(taille)) <= 0) { /* Name size */
        goto disconnect;
    }

#if DEBUG
    printf("taille nom %d\n",taille);
#endif

    nom = malloc((taille + 1) * sizeof(char));
    if(nom == NULL){
        goto disconnect;
    }

    nom[taille] = '\0';

    if (read(sock, (void *) nom, taille) <= 0) {        /* Name */
        goto disconnect;
    }

#if DEBUG
    printf("nom %s\n",nom);
#endif

    if (read(sock, (void *) &varSize, sizeof(varSize)) <= 0) { /* Var size */
        goto disconnect;
    }

#if DEBUG
    printf("size %d\n",varSize);
#endif



    if ((err = add_data(nom, varSize)) != 0) {
        /* ERREUR */
        if(err == -1){
            if(send_error(sock, ERROR_VAR_NAME_EXIST)<0){
                goto disconnect;
            }
        } else {
            if(send_error(sock, ERROR_HEAP_FULL)<0){
                goto disconnect;
            }
        }
    } else {

#if DEBUG
    printf("[Client %d] Allocation réussi de %s de taille %d\n",
           pthread_getspecific(id), nom, varSize);
#endif

        /* OK */
        nom = NULL; /* Le nom est stocké dans la structure et ne doit pas être free, même en cas d'erreur d'envoi */
        if(send_data(sock, MSG_ALLOC, 0)<0){
            goto disconnect;
        }
    }

    return 0;
    disconnect:
    if(nom != NULL){
        free(nom);
    }
    return -1;
}

int do_release(int sock){
    uint64_t offset;
    struct heapData *data;

    if (read(sock, (void *) &offset, sizeof(offset)) <= 0) { /* Offset */
        goto disconnect;
    }

    data = get_data_by_offset(offset);
    if(data == NULL){
        goto disconnect;
    }

#if DEBUG
        printf("[Client %d] Libération de %s\n", *(int*)pthread_getspecific(id),
           data->name);
#endif

    if((data->writeAccess != NULL) && (data->writeAccess->clientId == *(int*)pthread_getspecific(id))){
        /* Lock en write */
        if (read(sock, theHeap+offset, data->size) <= 0) {        /* Contenu */
            goto disconnect;
        }
        release_write_lock(data);
    } else {
        struct clientChainRead *temp = data->readAccess;
        while(temp != NULL && (*(int*)pthread_getspecific(id) != temp->clientId)){
            temp = temp->next;
        }
        if(temp == NULL){
            if(send_error(sock, ERROR_NOT_LOCKED)<0){
                goto disconnect;
            }
        } else {
            release_read_lock(data);
        }
    }





    if(send_data(sock, MSG_RELEASE, 0)<0){
        goto disconnect;
    }

    return 0;
    disconnect:
    return -1;
}

int do_free(int sock){
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

#if DEBUG
    printf("[Client %d] Désallocation de %s\n", pthread_getspecific(id),
           nom);
#endif

    if(remove_data(nom) != 0){
        /* ERREUR */
        goto disconnect;
    } else {

        /* OK */
        if(send_data(sock, MSG_FREE, 0)<0){
            goto disconnect;
        }
    }

    return 0;
    disconnect:
    return -1;
}

int snd_total_replication(int sock){
    struct  serverChain *servTemp = servers;
    struct heapData *data=NULL;
    uint8_t continuer=0, tailleNom;
#if DEBUG
    printf("debut snd total rep\n");
#endif
    pthread_mutex_lock(&hashTableMutex);
    int i=0;
    while(data==NULL && i!=parameters.hashSize){
        data = hashTable[i];
        i++;
    }
    if(data!=NULL){
        continuer=1;
    }

#if DEBUG
    printf("continuer %d\n",continuer);
#endif

    while(continuer){

#if DEBUG
    printf("continuer %d\n",continuer);
#endif
        if(write(sock, &continuer, sizeof(continuer)) <= 0){
            goto disconnect;
        }

        tailleNom = strlen(data->name);
        if(write(sock, &tailleNom, sizeof(tailleNom)) <= 0){
            goto disconnect;
        }

        if(write(sock, data->name, tailleNom) <= 0){
            goto disconnect;
        }

        if(write(sock, &data->offset, sizeof(data->offset)) <= 0){
            goto disconnect;
        }

        if(write(sock, &data->size, sizeof(data->size)) <= 0){
            goto disconnect;
        }


        if(write(sock, theHeap + data->offset, data->size) <= 0){
            goto disconnect;
        }

        /* TODO            *
         * Lock en attente */
        int i;
        if(write(sock, &data->readAccessSize, sizeof(data->readAccessSize)) <= 0){
                goto disconnect;
        }
        struct clientChainRead *readAccessTemp = data->readAccess;
        for(i=0; i<data->readAccessSize; i++){
            if(write(sock, &readAccessTemp->clientId, sizeof(readAccessTemp->clientId)) <= 0){
                goto disconnect;
            }
            readAccessTemp = readAccessTemp->next;
        }

        if(write(sock, &data->readWaitSize, sizeof(data->readWaitSize)) <= 0){
                goto disconnect;
        }
        struct clientChainRead *readWaitTemp = data->readWait;
        for(i=0; i<data->readWaitSize; i++){
            if(write(sock, &readWaitTemp->clientId, sizeof(readWaitTemp->clientId)) <= 0){
                goto disconnect;
            }
            readWaitTemp = readWaitTemp->next;
        }

        if(write(sock, &data->writeAccessSize, sizeof(data->writeAccessSize)) <= 0){
                goto disconnect;
        }
        struct clientChainWrite *writeAccessTemp = data->writeAccess;
        for(i=0; i<data->writeAccessSize; i++){
            if(write(sock, &writeAccessTemp->clientId, sizeof(writeAccessTemp->clientId)) <= 0){
                goto disconnect;
            }
            writeAccessTemp = writeAccessTemp->next;
        }

        if(write(sock, &data->writeWaitSize, sizeof(data->writeWaitSize)) <= 0){
                goto disconnect;
        }
        struct clientChainWrite *writeWaitTemp = data->writeWait;
        for(i=0; i<data->writeWaitSize; i++){
            if(write(sock, &writeWaitTemp->clientId, sizeof(writeWaitTemp->clientId)) <= 0){
                goto disconnect;
            }
            continuer = writeWaitTemp->clientId;
            writeWaitTemp = writeWaitTemp->next;
        }

        while(data==NULL && i!=parameters.hashSize){
            data = hashTable[i];
            i++;
        }
        if(data==NULL){
            continuer=0;
        }
    }

    pthread_mutex_unlock(&hashTableMutex);

    if(write(sock, &continuer, sizeof(continuer)) <= 0){
        goto disconnect;
    }

#if DEBUG
    printf("continuer %d\n",continuer);
    printf("fin snd_total_replication\n");
#endif

    return 1;
    disconnect:
    return -1;


}

int rcv_total_replication(int sock){
    int *servId;
    char **servAddress;
    uint8_t taille;
    char *nom;
    uint64_t offset;
    uint64_t varSize;
    struct heapData *data;
    uint8_t continuer;
    uint8_t msgType;

    /* Replication des données */
#if DEBUG
    printf("debut rcv total rep\n");
#endif
    if (read(sock, (void *) &continuer, sizeof(continuer)) <= 0) { /* nouvelle donnee a recevoir */
        goto disconnect;
    }
#if DEBUG
    printf("continuer %d\n",continuer);
#endif
    while(continuer!=0){
#if DEBUG
    printf("continuer %d\n",continuer);
#endif
        struct heapData *newData = malloc(sizeof(struct heapData));

        if(read(sock, (void *) &taille, sizeof(taille)) <= 0){
            goto disconnect;
        }

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
        newData->serverReadAccess = NULL;
        newData->serverWriteAccess = NULL;
        newData->serverReadWait = NULL;
        newData->serverReadWait = NULL;
        pthread_mutex_init(&(newData->mutex), NULL);
        pthread_cond_init(&(newData->readCond), NULL);
        pthread_cond_init(&(newData->serverReadCond), NULL);

        /* TODO            *
         * Lock en attente */

        if(read(sock, &newData->readAccessSize, sizeof(newData->readAccessSize)) <= 0){
                goto disconnect;
        }
        int i;
        struct clientChainRead *prevRead;
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

        prevRead=NULL;
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

        struct clientChainWrite *prevWrite;
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

        prevWrite=NULL;
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


        int sum = getHashSum(newData->name);
        int offsetSum = newData->offset % parameters.hashSize;

        pthread_mutex_lock(&hashTableMutex);

        newData->next = hashTable[sum];
        hashTable[sum] = newData;

        offsetSum = newData->offset % parameters.hashSize;
        newData->nextOffset = hashTableOffset[offsetSum];
        hashTableOffset[offsetSum] = newData;

        memset(theHeap + newData->offset, 0, newData->size);

        pthread_mutex_unlock(&hashTableMutex);

        if (read(sock, (void *) &continuer, sizeof(continuer)) <= 0) { /* nouvelle donnee a recevoir */
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

int snd_server_to_client(int sock, struct clientChain *client){
    uint8_t msgType = MSG_ADD_SERVER;
    uint8_t taille = strlen(servers->serverAddress);

#if DEBUG
    printf("debut snd server %d, @: %s, port: %d to clients\n",servers->serverId,servers->serverAddress,servers->serverPort);
#endif

        pthread_mutex_lock(&client->mutex_sock);

        if(send_data(client->sock, msgType, 4,
                    (DS){sizeof(servers->serverId),&servers->serverId},
                    (DS){sizeof(taille),&taille},
                    (DS){taille,servers->serverAddress},
                    (DS){sizeof(servers->serverPort),&servers->serverPort})<0){
            goto disconnect;
        }

        pthread_mutex_unlock(&client->mutex_sock);


#if DEBUG
    printf("fin snd server to clients\n");
#endif

    return 1;
    disconnect:
    return -1;

}

int snd_server_to_clients(char *address, uint16_t port, uint16_t serverId){
    struct clientChain *client = clients;
    uint8_t msgType = MSG_ADD_SERVER;
    uint8_t taille = strlen(address);

#if DEBUG
    printf("debut snd server %d, @: %s, port: %d to clients\n",serverId,address,port);
#endif


    while(client!=NULL){
        pthread_mutex_lock(&client->mutex_sock);

        if(send_data(client->sock, msgType, 4,
                    (DS){sizeof(serverId),&serverId},
                    (DS){sizeof(taille),&taille},
                    (DS){taille,address},
                    (DS){sizeof(port),&port})<0){
            goto disconnect;
        }

        pthread_mutex_unlock(&client->mutex_sock);
        client=client->next;
    }

#if DEBUG
    printf("fin snd server to clients\n");
#endif

    return 1;
    disconnect:
    return -1;
}

int snd_maj_client(struct replicationData *rep){
    struct serverChain *servTemp = servers;
    switch (rep->modification) {
        case ADD_CLIENT:
            if(send_data(servTemp->sock, MSG_ADD_CLIENT, 1,
                            (DS){sizeof(rep->clientId),&rep->clientId})<0){
                goto disconnect;
            }
            break;
        case REMOVE_CLIENT:
            if(send_data(servTemp->sock, MSG_RMV_CLIENT, 1,
                            (DS){sizeof(rep->clientId),&rep->clientId})<0){
                goto disconnect;
            }
            break;
        default:
            return 0;
    }
    rep->data=NULL;
    rep->clientId=0;
    return 1;
    disconnect:
    return -1;
}

int snd_data_replication(struct replicationData *rep){
    struct serverChain *servTemp = servers;
    uint8_t tailleNom;
    switch (rep->modification) {
        case DEFRAG:
            tailleNom = strlen(rep->data->name);
            if(send_data(servTemp->sock, MSG_DEFRAG_REPLICATION, 3,
                            (DS){sizeof(tailleNom),&tailleNom},
                            (DS){tailleNom, rep->data->name},
                            (DS){sizeof(rep->clientId),&rep->clientId})<0){
                goto disconnect;
            }
            break;
        case MAJ_ACCESS_READ:
            tailleNom = strlen(rep->data->name);
            if(send_data(servTemp->sock, MSG_MAJ_ACCESS_READ, 3,
                            (DS){sizeof(tailleNom),&tailleNom},
                            (DS){tailleNom, rep->data->name},
                            (DS){sizeof(rep->data->offset),&rep->data->offset})<0){
                goto disconnect;
            }
            break;
        case MAJ_ACCESS_WRITE:
            tailleNom = strlen(rep->data->name);
            if(send_data(servTemp->sock, MSG_MAJ_ACCESS_WRITE, 3,
                            (DS){sizeof(tailleNom),&tailleNom},
                            (DS){tailleNom, rep->data->name},
                            (DS){sizeof(rep->clientId),&rep->clientId})<0){
                goto disconnect;
            }
            break;
        case MAJ_WAIT_READ:
            tailleNom = strlen(rep->data->name);
            if(send_data(servTemp->sock, MSG_MAJ_WAIT_READ, 3,
                            (DS){sizeof(tailleNom),&tailleNom},
                            (DS){tailleNom, rep->data->name},
                            (DS){sizeof(rep->clientId),&rep->clientId})<0){
                goto disconnect;
            }
            break;
        case MAJ_WAIT_WRITE:
            tailleNom = strlen(rep->data->name);
            if(send_data(servTemp->sock, MSG_MAJ_WAIT_WRITE, 3,
                            (DS){sizeof(tailleNom),&tailleNom},
                            (DS){tailleNom, rep->data->name},
                            (DS){sizeof(rep->clientId),&rep->clientId})<0){
                goto disconnect;
            }
            break;
        case FREE_DATA:
            tailleNom = strlen(rep->data->name);
            if(send_data(servTemp->sock, MSG_FREE_REPLICATION, 5,
                            (DS){sizeof(tailleNom), &tailleNom},
                            (DS){tailleNom, rep->data->name})<0){
                goto disconnect;
            }
            break;
        case MAJ_DATA:
            tailleNom = strlen(rep->data->name);
            if(send_data(servTemp->sock, MSG_DATA_REPLICATION, 5,
                            (DS){sizeof(tailleNom), &tailleNom},
                            (DS){tailleNom, rep->data->name},
                            (DS){sizeof(rep->data->offset), &rep->data->offset},
                            (DS){sizeof(rep->data->size), &rep->data->size},
                            (DS){rep->data->size, theHeap + rep->data->offset})<0){
                goto disconnect;
            }
            break;
        case RELEASE_DATA:
            tailleNom = strlen(rep->data->name);
            if(send_data(servTemp->sock, MSG_RELEASE_REPLICATION, 2,
                            (DS){sizeof(tailleNom), &tailleNom},
                            (DS){tailleNom, rep->data->name},
                            (DS){sizeof(rep->clientId),&rep->clientId})<0){
                goto disconnect;
            }
            break;
        default:                /* Unknown message code, version problem? */
            return 0;
    }
    rep->data=NULL;
    rep->clientId=0;
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

int rcv_release_replication(int sock){
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
        data->writeAccessSize--;
        free(data->writeAccess);
    }else{
        temp=data->readAccess;
        while(temp->clientId != id){
            prev=temp;
            temp=temp->next;
        }
        if(temp!=NULL){
            if(prev!=NULL){
                prev->next=temp->next;
                free(temp);
            }else{
                data->readAccess= temp->next;
                free(temp);
            }
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

    //numClient++;
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
    uint16_t id;
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
    uint16_t id;
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
    uint16_t id;
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
    uint16_t id;
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

int snd_partial_replication(struct heapData *data){
    struct  serverChain *servTemp = servers;
    uint8_t tailleNom;
    int i;
    pthread_mutex_lock(&hashTableMutex);
    tailleNom = strlen(data->name);
    for(i=0; i<serversConnected; i++){
        if(send_data(servTemp->sock, MSG_PARTIAL_REPLICATION, 5,
                        (DS){sizeof(tailleNom), &tailleNom},
                        (DS){tailleNom, data->name},
                        (DS){sizeof(data->offset), &data->offset},
                        (DS){sizeof(data->size), &data->size},
                        (DS){data->size, theHeap + data->offset})<0){
            goto disconnect;

        /* TODO            *
         * Lock en attente */
        }

       servTemp=servTemp->next;
    }
    pthread_mutex_unlock(&hashTableMutex);
    return 1;
    disconnect:
    return -1;
}

int rcv_partial_replication(int sock){
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

        /* TODO            *
         * Lock en attente */

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

        /* TODO            *
         * Lock en attente */

    }
    return 1;
    disconnect:
    if(nom != NULL){
        free(nom);
    }
    return -1;
}