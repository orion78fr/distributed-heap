#include "common.h"

struct clientChain *clients = NULL;
struct serverChain *servers = NULL;

/**
 * Envoie sur la socket sock les donn�es pass�es en argument
 * @param sock Socket de communication
 * @param msgType Le type de message (voir messages.h)
 * @param nb Nombre de donn�es � envoyer
 * @param ... les structures dataSend � envoyer
 * @return 0 si succ�s
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

int do_inquire(int sock, char* address){
    uint8_t msgType;

    if (read(sock, (void *) &msgType, sizeof(msgType)) <= 0) {       /* Msg type */
        goto disconnect;
    }

    if(msgType == MSG_HELLO_NEW_CLIENT){

        /* Ajout du client dans la cha�ne de socket (ajout au d�but pour
         * �viter le parcours) */
        newClient = malloc(sizeof(struct clientChain));
        newClient->sock = sock;
        newClient->next = clients;
        newClient->clientId = numClient;
        numClient++;
        clients = newClient;

        /* Cr�ation d'un thread pour traiter les requ�tes */
        pthread_create((pthread_t *) & (newClient->threadId), NULL,
                       clientThread, (void *) newClient);
        clientsConnected++;
        return 1;

    }else if(msgType == MSG_HELLO_NEW_SERVER){

        pthread_mutex_lock(&schainlock);
        newServer = malloc(sizeof(struct serverChain));
        newServer->sock = sock;
        newServer->next = servers;
        newServer->serverAddress = address;
        newServer->serverId = numServer;
        numServer++;
        servers = newServer;

        poll_list=realloc(poll_list,sizeof(struct pollfd)*serversConnected);
        poll_list[serversConnected].fd = newServer->sock;
        poll_list[serversConnected].events = POLLHUP | POLLIN | POLLNVAL;
        serversConnected++;

        pthread_mutex_unlock(&schainlock);

        if(send_data(sock, MSG_HELLO_NEW_SERVER, 3,
                        (DS){sizeof(parameters.serverNum), &(parameters.serverNum)},
                        (DS){sizeof(newServer->serverId), &(newServer->serverId)})<0){
            goto disconnect;
        }
        return 2;
    }
    return -1;
}

/* TODO ajouter l'envoi des @ des autres serveurs et leurs id */
int do_greetings(int sock){
    uint8_t msgType;
    uint16_t clientId;

    
    clientId = sock;
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

    if (read(sock, (void *) &taille, sizeof(taille)) <= 0) { /* Name size */
        goto disconnect;
    }

    nom = malloc((taille + 1) * sizeof(char));
    if(nom == NULL){
        goto disconnect;
    }

    nom[taille] = '\0';

    if (read(sock, (void *) nom, taille) <= 0) {        /* Name */
        goto disconnect;
    }

    if (read(sock, (void *) &varSize, sizeof(varSize)) <= 0) { /* Var size */
        goto disconnect;
    }

#if DEBUG
    printf("[Client %d] Allocation de %s de taille %d\n",
           pthread_self(), nom, varSize);
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
        /* OK */
        nom = NULL; /* Le nom est stock� dans la structure et ne doit pas �tre free, m�me en cas d'erreur d'envoi */
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
        printf("[Client %d] Lib�ration de %s\n", pthread_self(),
           data->name);
#endif

    if(data->writeAccess != NULL && (pthread_equal(data->writeAccess->clientId, pthread_self()) != 0)){
        /* Lock en write */
        if (read(sock, theHeap+offset, data->size) <= 0) {        /* Contenu */
            goto disconnect;
        }
        release_write_lock(data);
    } else {
        struct clientChainRead *temp = data->readAccess;
        while(temp != NULL && (pthread_equal(pthread_self(), temp->clientId) ==0)){
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
    printf("[Client %d] D�sallocation de %s\n", pthread_self(),
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

    if(write(sock, &serversConnected, sizeof(serversConnected)) <= 0){
        goto disconnect;
    }

    while(servTemp!=NULL){

        if(write(sock, &servTemp->serverId, sizeof(servTemp->serverId)) <= 0){
            goto disconnect;
        }

        tailleNom = strlen(servTemp->serverAddress);

        if(write(sock, &tailleNom, sizeof(tailleNom)) <= 0){
            goto disconnect;
        }

        if(write(sock, servTemp->serverAddress, tailleNom) <= 0){
            goto disconnect;
        }

       servTemp=servTemp->next;
    }

    pthread_mutex_lock(&hashTableMutex);
    int i=0;
    while(data==NULL && i!=hashSize){
        data = hashTable[i]
        i++;
    }
    if(data!=NULL){
        continuer=1;
    }
    while(continuer){


        
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
        struct clientChainRead *readAccessTemp = readAccess;
        for(i=0; i<readAccessSize; i++){
            if(write(sock, &readAccessTemp->clientId, sizeof(readAccessTemp->clientId)) <= 0){
                goto disconnect;
            }   
            readAccessTemp = readAccessTemp->next;
        }

        if(write(sock, &data->readWaitSize, sizeof(data->readWaitSize)) <= 0){
                goto disconnect;
        }
        struct clientChainRead *readWaitTemp = readWait;
        for(i=0; i<readWaitSize; i++){
            if(write(sock, &readWaitTemp->clientId, sizeof(readWaitTemp->clientId)) <= 0){
                goto disconnect;
            }
            readWaitTemp = readWaitTemp->next;
        }
        
        if(write(sock, &data->writeAccessSize, sizeof(data->writeAccessSize)) <= 0){
                goto disconnect;
        }
        struct clientChainWrite *writeAccessTemp = writeAccess;
        for(i=0; i<writeAccessSize; i++){
            if(write(sock, &writeAccessTemp->clientId, sizeof(writeAccessTemp->clientId)) <= 0){
                goto disconnect;
            }
            writeAccessTemp = writeAccessTemp->next;
        }

        if(write(sock, &data->writeWaitSize, sizeof(data->writeWaitSize)) <= 0){
                goto disconnect;
        }
        struct clientChainWrite *writeWaitTemp = writeWait;
        for(i=0; i<writeWaitSize; i++){
            if(write(sock, &writeWaitTemp->clientId, sizeof(writeWaitTemp->clientId)) <= 0){
                goto disconnect;
            }
            continuer = writeWaitTemp->clientId;
            writeWaitTemp = writeWaitTemp->next;
        }

        if(write(sock, &data->serverReadAccessSize, sizeof(data->serverReadAccessSize)) <= 0){
                goto disconnect;
        }
        struct serverChainRead *serverReadAccessTemp = serverReadAccess;
        for(i=0; i<serverReadAccessSize; i++){
            if(write(sock, &serverReadAccessTemp->serverId, sizeof(serverReadAccessTemp->serverId)) <= 0){
                goto disconnect;
            }
            serverReadAccessTemp = serverReadAccessTemp->next;
        }

        if(write(sock, &data->serverReadWaitSize, sizeof(data->serverReadWaitSize)) <= 0){
                goto disconnect;
        }
        struct serverChainRead *serverReadWaitTemp = serverReadWait;
        for(i=0; i<serverReadWaitSize; i++){
            if(write(sock, &serverReadWaitTemp->serverId, sizeof(serverReadWaitTemp->serverId)) <= 0){
                goto disconnect;
            }
            serverReadWaitTemp = serverReadWaitTemp->next;
        }
        
        if(write(sock, &data->serverWriteAccessSize, sizeof(data->serverWriteAccessSize)) <= 0){
                goto disconnect;
        }
        struct serverChainWrite *serverWriteAccessTemp = serverWriteAccess;
        for(i=0; i<serverWriteAccessSize; i++){
            if(write(sock, &serverWriteAccessTemp->serverId, sizeof(serverWriteAccessTemp->serverId)) <= 0){
                goto disconnect;
            }
            serverWriteAccessTemp = serverWriteAccessTemp->next;
        }
        
        if(write(sock, &data->serverWriteWaitSize, sizeof(data->serverWriteWaitSize)) <= 0){
                goto disconnect;
        }
        struct serverChainWrite *serverWriteWaitTemp = serverWriteWait;
        for(i=0; i<serverWriteWaitSize; i++){
            if(write(sock, &serverWriteWaitTemp->serverId, sizeof(serverWriteWaitTemp->serverId)) <= 0){
                goto disconnect;
            }
            serverWriteWaitTemp = serverWriteWaitTemp->next;
        }

        while(data==NULL && i!=hashSize){
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

    /* Replication des serveurs */


    if (read(sock, (void *) &serversConnected, sizeof(serversConnected)) <= 0) { /* Nombre serveurs */
        goto disconnect;
    }

    /*servAddress = malloc(serversConnected * sizeof(char*));
    for (int i = 0; i < serversConnected; i++)
        servAddress[i] = malloc(16);*/

    for(int i=0; i<serversConnected; i++){
        struct serverChain *newServer;
        newServer = malloc(sizeof(struct serverChain));

        if (read(sock, (void *) &newServer->serverId, sizeof(newServer->serverId)) <= 0) { /* id des serveurs */
            goto disconnect;
        } 

        if(read(sock, (void *) &taille, sizeof(taille)) <= 0){ /* taille @ serveur */
            goto disconnect;
        }

        if (read(sock, (void *) newServer->serverAddress, taille) <= 0) { /* @ serveur */
            goto disconnect;
        } 
    }

    /* Replication des donn�es */

    if (read(sock, (void *) &continuer, sizeof(continuer)) <= 0) { /* nouvelle donnee a recevoir */
        goto disconnect;
    }

    while(continuer!=0){
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
            temp = malloc(sizeof(clientChainRead));
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
            temp = malloc(sizeof(clientChainRead));
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
            temp = malloc(sizeof(clientChainWrite));
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
            temp = malloc(sizeof(clientChainWrite));            
            if(read(sock, &temp->clientId, sizeof(temp->clientId)) <= 0){
                goto disconnect;
            }
            pthread_cond_init(&(temp->cond), NULL);
            if(i!=0){
                prevWrite->next=temp;
            }
            prevWrite=temp;
        }

        if(read(sock, &newData->serverReadAccessSize, sizeof(newData->readAccessSize)) <= 0){
                goto disconnect;
        }
        int i;
        struct serverChainRead *prevSRead;
        for(i=0; i<newData->serverReadAccessSize; i++){
            struct serverChainRead *temp;
            temp = malloc(sizeof(serverChainRead));
            if(read(sock, &temp->serverId, sizeof(temp->serverId)) <= 0){
                goto disconnect;
            }
            if(i!=0){
                prevSRead->next=temp;
            }
            prevSRead=temp;
        }

        if(read(sock, &newData->serverReadWaitSize, sizeof(newData->readWaitSize)) <= 0){
                goto disconnect;
        }

        prevSRead=NULL;
        for(i=0; i<newData->readWaitSize; i++){
            struct serverChainRead *temp;
            temp = malloc(sizeof(serverChainRead));
            if(read(sock, &temp->serverId, sizeof(temp->serverId)) <= 0){
                goto disconnect;
            }
            if(i!=0){
                prevSRead->next=temp;
            }
            prevSRead=temp;
        }
        
        if(read(sock, &newData->serverWriteAccessSize, sizeof(newData->writeAccessSize)) <= 0){
                goto disconnect;
        }

        struct serverChainWrite *prevSWrite;
        for(i=0; i<newData->serverWriteAccessSize; i++){
            struct serverChainWrite *temp;
            temp = malloc(sizeof(serverChainWrite));
            if(read(sock, &temp->serverId, sizeof(temp->serverId)) <= 0){
                goto disconnect;
            }
            pthread_cond_init(&(temp->cond), NULL);
            if(i!=0){
                prevSWrite->next=temp;
            }
            prevSWrite=temp;
        }

        if(read(sock, &newData->serverWriteWaitSize, sizeof(newData->writeWaitSize)) <= 0){
                goto disconnect;
        }

        prevSWrite=NULL;
        for(i=0; i<newData->serverWriteWaitSize; i++){
            struct serverChainWrite *temp;
            temp = malloc(sizeof(serverChainWrite));            
            if(read(sock, &temp->serverId, sizeof(temp->serverId)) <= 0){
                goto disconnect;
            }
            pthread_cond_init(&(temp->cond), NULL);
            if(i!=0){
                prevSWrite->next=temp;
            }
            prevSWrite=temp;
        }



        int sum = getHashSum(newData->name); 
        int offsetSum = newData->offset % parameters.hashSize;
        
        pthread_mutex_lock(&hashTableMutex);

        newData->next = hashTable[sum];
        hashTable[sum] = newData;

        offsetSum = newData->offset % parameters.hashSize;
        newData->nextOffset = hashTableOffset[offsetSum];
        hashTableOffset[offsetSum] = newData;

        memset(theHeap + newData->offset, 0, size);

        pthread_mutex_unlock(&hashTableMutex);

        if (read(sock, (void *) &continuer, sizeof(continuer)) <= 0) { /* nouvelle donnee a recevoir */
            goto disconnect;
        }   
    }


    /* connexion aux serveurs */


    /* envoi d'un message pour confirmer la replication totale */


}

int snd_maj_access(struct heapData *data){

}

int rcv_maj_access(int sock){

}

int snd_maj_wait(int sock, struct heapData *data){

}

int rcv_maj_wait(int sock){

}

int snd_data_replication(struct replicationData *rep){
    struct serverChain *servTemp = servers;
    switch (rep->modification) {
        case MAJ_ACCESS_READ:
            tailleNom = strlen(rep->data->name);
            if(send_data(servTemp->sock, MAJ_ACCESS_READ, ,
                            (DS){sizeof(tailleNom),&tailleNom},
                            (DS){tailleNom, rep->data->name},
                            (DS){sizeof(rep->clientId),&rep->clientId},)<0)
            break;
        case MAJ_ACCESS_WRITE:
            
            break;
        case MAJ_WAIT_READ:
            
            break;
        case MAJ_WAIT_WRITE:
            
            break;
        case MAJ_DATA:
            
            break;
        default:                /* Unknown message code, version problem? */
            return 0;
        }
}

int rcv_data_replication(int sock){

}

int snd_partial_replication(struct heapData *data){
    struct  serverChain *servTemp = servers;
    uint8_t tailleNom;

    pthread_mutex_lock(&hashTableMutex);
    tailleNom = strlen(data->name);
    for(int i=0; i<serversConnected; i++){
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

    if(data == NULL){/* cr�ation de la var */
#if DEBUG
    printf("[Server %d] Demande creation(replication partielle) de %s\n",
           pthread_self(), nom);
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

        memset(theHeap + data->offset, 0, size);

        pthread_mutex_unlock(&hashTableMutex);


    } else {/* mis � jour de la var */
#if DEBUG
    printf("[Server %d] Demande mis a jour(replication partielle) de %s\n",
           pthread_self(), data->name);
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
    return 0;
    disconnect:
    if(nom != NULL){
        free(nom);
    }
    return -1;
}
