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

/* TODO ajouter l'envoi des @ des autres serveurs et leurs id */
int do_greetings(int sock){
    uint8_t msgType;
    uint16_t clientId;

    
    clientId = sock;
    if(send_data(sock, MSG_HELLO_NEW_CLIENT, 3,
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
           pthread_getspecific(id), nom, varSize);
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
        printf("[Client %d] Lib�ration de %s\n", pthread_getspecific(id),
           data->name);
#endif

    if((data->writeAccess != NULL) && (data->writeAccess->clientId == pthread_getspecific(id))){
        /* Lock en write */
        if (read(sock, theHeap+offset, data->size) <= 0) {        /* Contenu */
            goto disconnect;
        }
        release_write_lock(data);
    } else {
        struct clientChainRead *temp = data->readAccess;
        while(temp != NULL && (pthread_getspecific(id) != temp->clientId)){
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
    printf("[Client %d] D�sallocation de %s\n", pthread_getspecific(id),
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

    pthread_mutex_lock(&hashTableMutex);
    int i=0;
    while(data==NULL && i!=parameters.hashSize){
        data = hashTable[i];
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
    return 1;
    disconnect:
    return -1;
}

int snd_data_replication(struct replicationData *rep){
    struct serverChain *servTemp = servers;
    char *tailleNom;
    switch (rep->modification) {
        case MAJ_ACCESS_READ:
            tailleNom = strlen(rep->data->name);
            if(send_data(servTemp->sock, MSG_MAJ_ACCESS_READ, 3,
                            (DS){sizeof(tailleNom),&tailleNom},
                            (DS){tailleNom, rep->data->name},
                            (DS){sizeof(rep->clientId),&rep->clientId})<0){
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

    if(data == NULL){/* cr�ation de la var */
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

        data->readAccessSize=NULL;
        data->readWaitSize=NULL;
        data->writeAccessSize=NULL;
        data->writeWaitSize=NULL;
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


    } else {/* mis � jour de la var */
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
    return 0;
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
    struct clientChainWrite *temp=NULL, *prev=NULL;

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
    /* Ajout du client dans la cha�ne de socket (ajout au d�but pour
    * �viter le parcours) */
    newClient = malloc(sizeof(struct clientChain));
    newClient->sock = NULL;
    newClient->next = clients;

    if (read(sock, &newClient->clientId, sizeof(newClient->clientId)) <= 0) {
        goto disconnect;
    }

    numClient++;
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
    disconnect:
    return -1;
}

int rcv_maj_access_read(int sock){
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

    if(data == NULL){/* cr�ation de la var */
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


    } else {/* mis � jour de la var */
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
    return 0;
    disconnect:
    if(nom != NULL){
        free(nom);
    }
    return -1;
}
