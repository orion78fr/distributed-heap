#include "common.h"

struct heapData *hashTable[HASHSIZE];

/**
 * Recupere la structure heapData correspondant à un nom
 * @param name Nom de la variable
 * @return Pointeur sur la structure heapData, NULL sinon
 */
struct heapData *get_data(char *name)
{
    int sum = getHashSum(name);
    struct heapData *data = hashTable[sum];

    while (data != NULL) {
	if (strcmp(name, data->name) == 0) {
	    return data;
	} else {
	    data = data->next;
	}
    }

    return NULL;
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

    return sum % HASHSIZE;
}

/**
 * Ajoute une nouvelle variable au tas
 * @param name Nom de la variable
 * @param size Taille de la variable
 * @return 0 si succès, -1 si la variable existe déjà
 */
int add_data(char *name, int size)
{
    int sum = getHashSum(name);

    if (get_data(name) != NULL) {
	return -1;
    } else {
	struct heapData *newData = malloc(sizeof(struct heapData));
	newData->name = name;
	newData->size = size;
	newData->readAccess = NULL;
	newData->writeAccess = NULL;
	newData->readWait = NULL;
	newData->writeWait = NULL;

	newData->offset = alloc_space(size);

	newData->next = hashTable[sum];
	hashTable[sum] = newData;
    }
    return 0;
}

/**
 * Initialise les variables du tas
 */
void init_data()
{
    int i;
    for (i = 0; i < HASHSIZE; i++) {
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
    /* TODO */
    return 0;
}
