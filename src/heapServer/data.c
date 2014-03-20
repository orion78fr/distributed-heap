#include "common.h"

struct heapData **hashTable;
pthread_mutex_t hashTableMutex = PTHREAD_MUTEX_INITIALIZER;

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
int add_data(char *name, int size)
{
    if (get_data(name) != NULL) {
        return -1;
    } else {
        int sum = getHashSum(name);
        struct heapData *newData = malloc(sizeof(struct heapData));
        newData->name = name;
        newData->size = size;
        newData->readAccess = NULL;
        newData->writeAccess = NULL;
        newData->readWait = NULL;
        newData->writeWait = NULL;
        pthread_mutex_init(&(newData->mutex), NULL);

        newData->offset = alloc_space(size);
        
        pthread_mutex_lock(&hashTableMutex);

        newData->next = hashTable[sum];
        hashTable[sum] = newData;
        
        pthread_mutex_unlock(&hashTableMutex);
    }
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
    freeList = malloc(sizeof(struct freeListChain));
    freeList->startOffset = 0;
    freeList->size = parameters.heapSize;
    freeList->next = NULL;
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
    struct heapData *data = hashTable[sum];
    
    /* L'user qui fait le free doit faire l'équivalent d'une demande en
     * en écriture avant */
    
    while(data != NULL){
        if(strcmp(data->name, name) == 0){
            break;
        } else {
            prevData = data;
            data = data->next;
        }
    }
    
    if(data == NULL){
        return -1; /* Impossible de trouver une variable ayant ce nom */
    }
    
    free_space(data->offset, data->size);
    
    if(prevData == NULL){
        hashTable[sum] = data->next;
    } else {
        prevData->next = data->next;
    }
    
    /* TODO Faire les free des demandes */
    
    free(data);
    
    return 0;
}
