#include "common.h"

struct heapData **hashTable, **hashTableOffset;
pthread_mutex_t hashTableMutex = PTHREAD_MUTEX_INITIALIZER;
void *theHeap;
pthread_mutex_t addDataMutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * Recupere la structure heapData correspondant à un nom
 * @param name Nom de la variable
 * @return Pointeur sur la structure heapData, NULL sinon
 */
struct heapData *get_data(char *name)
{
    int sum = getHashSum(name);
    struct heapData *data;

    pthread_mutex_lock(&hashTableMutex);

    data = hashTable[sum];

    while (data != NULL) {
        if (strcmp(name, data->name) == 0) {
            break;
        } else {
            data = data->next;
        }
    }

    pthread_mutex_unlock(&hashTableMutex);

    return data;
}

struct heapData *get_data_by_offset(uint64_t offset)
{
    int sum = offset % parameters.hashSize;
    struct heapData *data;

    pthread_mutex_lock(&hashTableMutex);

    data = hashTableOffset[sum];

    while (data != NULL) {
        if (data->offset == offset) {
            break;
        } else {
            data = data->next;
        }
    }

    pthread_mutex_unlock(&hashTableMutex);

    return data;
}

/**
 * Hash du nom pour la hashTable
 * @param name Nom de la variable
 * @return Le hash du nom
 */
int getHashSum(char *name)
{
    int sum = 0;
    int size = strlen(name);
    int i;

    for (i = 0; i < size; i++) {
        sum += name[i];
    }

    return sum % parameters.hashSize;
}

/**
 * Ajoute une nouvelle variable au tas
 * @param name Nom de la variable
 * @param size Taille de la variable
 * @return 0 si succès, -1 si la variable existe déjà
 */
int add_data(char *name, uint64_t size)
{
    pthread_mutex_lock(&addDataMutex);
    if (get_data(name) != NULL) {
        pthread_mutex_unlock(&addDataMutex);
        return -1;
    } else {
        int sum = getHashSum(name);
        int offsetSum;
        struct heapData *newData = malloc(sizeof(struct heapData));
        newData->name = name;
        newData->size = size;
        newData->readAccess = NULL;
        newData->writeAccess = NULL;
        newData->readWait = NULL;
        newData->writeWait = NULL;
        pthread_mutex_init(&(newData->mutex), NULL);
        pthread_cond_init(&(newData->readCond), NULL);

        newData->offset = alloc_space(size);
        if(newData->offset == -1){
            /* Tas plein, tentative de défrag */
            newData->offset = defrag_if_possible(size);
            if(newData->offset == -1){
                free(newData);
                return -2;
            }
        }

        pthread_mutex_lock(&hashTableMutex);

        newData->next = hashTable[sum];
        hashTable[sum] = newData;

        offsetSum = newData->offset % parameters.hashSize;
        newData->nextOffset = hashTableOffset[offsetSum];
        hashTableOffset[offsetSum] = newData;

        memset(theHeap + newData->offset, 0, size);

#if DEBUG
    printf("ADD_DATA, debut replication\n");
#endif

        if(servers!=NULL && !servers->backup){

            pthread_mutex_lock(&rep->mutex_server);

            rep->modification = MAJ_DATA;
            rep->data = newData;
            rep->clientId = 0;

            pthread_mutex_unlock(&rep->mutex_server);
            pthread_mutex_lock(&ack->mutex_server);
            pthread_cond_signal(&rep->cond_server);
            pthread_cond_wait(&ack->cond_server, &ack->mutex_server);
            pthread_mutex_unlock(&ack->mutex_server);
#if DEBUG
    printf("ADD_DATA, replication done\n");
#endif
        }
        pthread_mutex_unlock(&hashTableMutex);
    }
    pthread_mutex_unlock(&addDataMutex);
    return 0;
}

/**
 * Initialise les variables du tas
 */
void init_data()
{
    int i;
    hashTable = malloc(parameters.hashSize * sizeof(struct heapData *));
    for (i = 0; i < parameters.hashSize; i++) {
        hashTable[i] = NULL;
    }
    hashTableOffset = malloc(parameters.hashSize * sizeof(struct heapData *));
    for (i = 0; i < parameters.hashSize; i++) {
        hashTableOffset[i] = NULL;
    }

    freeList = malloc(sizeof(struct freeListChain));
    freeList->startOffset = 0;
    freeList->size = parameters.heapSize;
    freeList->next = NULL;

    theHeap = malloc(parameters.heapSize);
}

/**
 * Retire une variable du tas (free)
 * @param name Nom de la variable à retirer
 * @return 0 si succès
 */
int remove_data(char *name)
{
    int sum = getHashSum(name);
    struct heapData *prevData = NULL;
    struct heapData *data;

    pthread_mutex_lock(&hashTableMutex);

    data = hashTable[sum];

    /* L'user qui fait le free doit faire l'équivalent d'une demande en
     * en écriture avant */

    while (data != NULL) {
        if (strcmp(data->name, name) == 0) {
            break;
        } else {
            prevData = data;
            data = data->next;
        }
    }

    if (data == NULL) {
        return -1;              /* Impossible de trouver une variable ayant ce nom */
    }

    free_space(data->offset, data->size);

    if (prevData == NULL) {
        hashTable[sum] = data->next;
    } else {
        prevData->next = data->next;
    }

    /* TODO Faire les free des demandes */

    pthread_mutex_unlock(&hashTableMutex);

    
    if(servers!=NULL && !servers->backup){
        pthread_mutex_lock(&rep->mutex_server);

        rep->modification = FREE_DATA;
        rep->data = data;
        rep->clientId = 0;

        pthread_mutex_unlock(&rep->mutex_server);
        pthread_mutex_lock(&ack->mutex_server);
        pthread_cond_signal(&rep->cond_server);
        pthread_cond_wait(&ack->cond_server, &ack->mutex_server);
        pthread_mutex_unlock(&ack->mutex_server);
#if DEBUG
    printf("FREE_DATA, replication done\n");
#endif
    }
    free(data);
    return 0;
}

/* TODO faire les pthread_equal (== n'est pas bon) */
int acquire_read_lock(struct heapData *data)
{
    struct clientChainRead *me;

    pthread_mutex_lock(&(data->mutex));

    me = malloc(sizeof(struct clientChainRead));
    me->clientId = pthread_getspecific(id);

    if (data->readAccess != NULL) {
        /* On est en mode read... */
        if (data->writeWait == NULL) {
            /* ...et personne n'attend l'écriture */
        } else {
            acquire_read_sleep(data, me);
        }
    } else if (data->writeAccess != NULL) {
        /* On est en mode write */
        acquire_read_sleep(data, me);
    } else {
        /* Le data n'est pas actif... */
        if (data->readWait != NULL || data->writeWait != NULL) {
            /* ...mais il y a deja des personnes en attente */
            acquire_read_sleep(data, me);
        } else {
            /* ...et il y a personne */
        }
    }

    /* On se met dans la liste de read */
    me->next = data->readAccess;
    data->readAccess = me;

    pthread_mutex_unlock(&(data->mutex));

    if(servers!=NULL && !servers->backup){

        pthread_mutex_lock(&rep->mutex_server);

        rep->modification = MAJ_ACCESS_READ;
        rep->data = data;
        rep->clientId = me->clientId;

        pthread_mutex_unlock(&rep->mutex_server);
        pthread_mutex_lock(&ack->mutex_server);
        pthread_cond_signal(&rep->cond_server);
        pthread_cond_wait(&ack->cond_server, &ack->mutex_server);
        pthread_mutex_unlock(&ack->mutex_server);
#if DEBUG
    printf("MAJ_ACCESS_READ, replication done\n");
#endif
    }
    return 0;
}

void acquire_read_sleep(struct heapData *data, struct clientChainRead *me)
{
    struct clientChainRead *prevMe;
    me->next = data->readWait;
    data->readWait = me;

    if(servers!=NULL && !servers->backup){
        pthread_mutex_lock(&rep->mutex_server);

        rep->modification = MAJ_WAIT_READ;
        rep->data = data;
        rep->clientId = me->clientId;

        pthread_mutex_unlock(&rep->mutex_server);
        pthread_mutex_lock(&ack->mutex_server);
        pthread_cond_signal(&rep->cond_server);
        pthread_cond_wait(&ack->cond_server, &ack->mutex_server);
        pthread_mutex_unlock(&ack->mutex_server);

#if DEBUG
    printf("MAJ_WAIT_READ, replication done\n");
#endif
    }
    /* On attend */
    pthread_cond_wait(&(data->readCond), &(data->mutex));
    /* On se retire de la liste d'attente et on se met en read */
    if ((prevMe = data->readWait) == me) {
        data->readWait = me->next;
    } else {
        while (prevMe != NULL && prevMe->next != me) {
            /* prevMe ne devrait pas être NULL de toute façon, peut-être
             * retirer le test */
            prevMe = prevMe->next;
        }
        prevMe->next = me->next;
    }
}

int acquire_write_lock(struct heapData *data)
{
    struct clientChainWrite *me;

    pthread_mutex_lock(&(data->mutex));

    me = malloc(sizeof(struct clientChainWrite));
    me->clientId = pthread_getspecific(id);
    pthread_cond_init(&(me->cond), NULL);

    if (data->readAccess != NULL || data->writeAccess != NULL) {
        /* Si quelqu'un est en accès */
        acquire_write_sleep(data, me);
    } else {
        if (data->readWait != NULL || data->writeWait != NULL) {
            /* Si quelqu'un attend déjà */
            acquire_write_sleep(data, me);
        }
    }

    me->next = NULL;
    data->writeAccess = me;

    pthread_mutex_unlock(&(data->mutex));

    if(servers!=NULL && !servers->backup){
        pthread_mutex_lock(&rep->mutex_server);

        rep->modification = MAJ_ACCESS_WRITE;
        rep->data = data;
        rep->clientId = me->clientId;

        pthread_mutex_unlock(&rep->mutex_server);
        pthread_mutex_lock(&ack->mutex_server);
        pthread_cond_signal(&rep->cond_server);
        pthread_cond_wait(&ack->cond_server, &ack->mutex_server);
        pthread_mutex_unlock(&ack->mutex_server);

#if DEBUG
    printf("MAJ_ACCESS_WRITE, replication done\n");
#endif
    }
    return 0;
}

void acquire_write_sleep(struct heapData *data,
                         struct clientChainWrite *me)
{
    struct clientChainWrite *prevMe;
    me->next = data->writeWait;
    data->writeWait = me;

    if(servers!=NULL && !servers->backup){
        pthread_mutex_lock(&rep->mutex_server);

        rep->modification = MAJ_WAIT_WRITE;
        rep->data = data;
        rep->clientId = me->clientId;

        pthread_mutex_unlock(&rep->mutex_server);
        pthread_mutex_lock(&ack->mutex_server);
        pthread_cond_signal(&rep->cond_server);
        pthread_cond_wait(&ack->cond_server, &ack->mutex_server);
        pthread_mutex_unlock(&ack->mutex_server);

#if DEBUG
    printf("MAJ_WAIT_WRITE, replication done\n");
#endif
    }
    pthread_cond_wait(&(me->cond), &(data->mutex));

    if ((prevMe = data->writeWait) == me) {
        data->writeWait = me->next;
    } else {
        while (prevMe != NULL && prevMe->next != me) {
            prevMe = prevMe->next;
        }
        prevMe->next = me->next;
    }
}

int release_read_lock(struct heapData *data)
{
    struct clientChainRead *me, *prevMe;
    int myId;

    pthread_mutex_lock(&(data->mutex));

    /* On se retire de la liste */
    if ((prevMe = data->readAccess) == NULL) {
        return -2;
    }
    myId = pthread_getspecific(id);
    if (prevMe->clientId == myId) {
        data->readAccess = prevMe->next;
        free(prevMe);
    } else {
        while (prevMe->next != NULL && prevMe->next->clientId != myId) {
            prevMe = prevMe->next;
        }
        if ((me = prevMe->next) == NULL) {
            return -2;
        }
        prevMe->next = me->next;
        free(me);
    }

    if(servers!=NULL && !servers->backup){
        pthread_mutex_lock(&rep->mutex_server);

        rep->modification = RELEASE_DATA;
        rep->data = data;
        rep->clientId = myId;
        pthread_mutex_unlock(&rep->mutex_server);
        pthread_mutex_lock(&ack->mutex_server);
        pthread_cond_signal(&rep->cond_server);
        pthread_cond_wait(&ack->cond_server, &ack->mutex_server);
        pthread_mutex_unlock(&ack->mutex_server);

#if DEBUG
    printf("RELEASE_DATA, replication done\n");
#endif
    }   
    /* Réveil du/des suivant(s) */
    if (data->readAccess == NULL) {
        struct clientChainWrite *nextAccess;
        if ((nextAccess = data->writeWait) != NULL) {
            /* La première demande se trouve à la fin */
            while (nextAccess->next != NULL) {
                nextAccess = nextAccess->next;
            }
            pthread_cond_signal(&(nextAccess->cond));

        }
    }

    pthread_mutex_unlock(&(data->mutex));

    return 0;
}

int release_write_lock(struct heapData *data)
{
    pthread_mutex_lock(&(data->mutex));

    if (data->writeAccess == NULL
        || data->writeAccess->clientId != pthread_getspecific(id)) {
        return -2;
    }
    free(data->writeAccess);
    data->writeAccess = NULL;

    if(servers!=NULL && !servers->backup){
        pthread_mutex_lock(&rep->mutex_server);

        rep->modification = RELEASE_DATA;
        rep->data = data;
        rep->clientId = pthread_getspecific(id);
        pthread_mutex_unlock(&rep->mutex_server);
        pthread_mutex_lock(&ack->mutex_server);
        pthread_cond_signal(&rep->cond_server);
        pthread_cond_wait(&ack->cond_server, &ack->mutex_server);
        pthread_mutex_unlock(&ack->mutex_server);

#if DEBUG
    printf("RELEASE_DATA, replication done\n");
#endif
    }
    /* Réveil des suivants */
    if (data->readWait != NULL) {
        pthread_cond_broadcast(&(data->readCond));
    } else {
        struct clientChainWrite *nextAccess;
        if ((nextAccess = data->writeWait) != NULL) {
            /* La première demande se trouve à la fin */
            while (nextAccess->next != NULL) {
                nextAccess = nextAccess->next;
            }
            pthread_cond_signal(&(nextAccess->cond));
        }
    }

    pthread_mutex_unlock(&(data->mutex));

    return 0;
}
