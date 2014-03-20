#include "common.h"

struct freeListChain *freeList;
pthread_mutex_t freeListMutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * Alloue un espace de taille size dans le tas
 * @param size Taille de la variable à allouer
 * @return L'offset de la variable
 */
int alloc_space(int size)
{
    struct freeListChain *tempFreeList;
    struct freeListChain *prevFreeList = NULL;
    int offset = -1;

    pthread_mutex_lock(&freeListMutex);
    
    tempFreeList = freeList;

    while (tempFreeList != NULL) {
        if (tempFreeList->size >= size) {
            offset = tempFreeList->startOffset;
            tempFreeList->startOffset += size;
            tempFreeList->size -= size;
            if (tempFreeList->size == 0) {
                /* Ce morceau est vide, on le retire de la liste */
                if (prevFreeList != NULL) {
                    /* On est pas au début, on le retire */
                    prevFreeList->next = tempFreeList->next;
                } else {
                    /* On est au début et il y a quelque chose après */
                    freeList = tempFreeList->next;
                    /* Même si la freeList se retrouverait vide,
                     * next contient bien NULL */
                }
                free(tempFreeList);
            }
            break;
        } else {
            prevFreeList = tempFreeList;
            tempFreeList = tempFreeList->next;
        }
    }
    
    pthread_mutex_unlock(&freeListMutex);
    
    /* GTU : Si -1, peut être lancer une défragmentation et réessayer */
    
    return offset;
}

/**
 * Remet dans la freeList une portion du tas
 * @param offset Offset de la variable libérée
 * @param size Taille de la variable libérée
 */
void free_space(int offset, int size)
{
    struct freeListChain *prevFreeList = NULL;
    struct freeListChain *nextFreeList;
    int fusion = 0;

    pthread_mutex_lock(&freeListMutex);

    nextFreeList = freeList;

    /* D'abord rechercher un encadrement de l'offset dans la freeList */
    while (nextFreeList != NULL && nextFreeList->startOffset < offset) {
        prevFreeList = nextFreeList;
        nextFreeList = nextFreeList->next;
    }

    /* On vérifie si on peut fusionner avec la partie d'avant */
    if ((prevFreeList != NULL)
        && ((prevFreeList->startOffset + prevFreeList->size) == offset)) {
        prevFreeList->size += size;
        fusion++;
    }

    /* On vérifie si on peut fusionner avec la partie d'après */
    if ((nextFreeList != NULL)
        && (nextFreeList->startOffset == (offset + size))) {
        if (fusion) {
            /* Si on a fusionné avec la partie d'avant, on doit fusionner
             * avec celle d'après et la supprimer */
            prevFreeList->size += nextFreeList->size;
            prevFreeList->next = nextFreeList->next;
            free(nextFreeList);
        } else {
            /* Sinon, on fusionne seulement avec celle d'après */
            nextFreeList->startOffset = offset;
            nextFreeList->size += size;
        }
        fusion++;
    }

    /* Si on ne peut fusionner avec aucun des bouts de freelist, on doit
     * créer un nouveau bout */
    if (!fusion) {
        struct freeListChain *newFreeList =
            malloc(sizeof(struct freeListChain));
        newFreeList->startOffset = offset;
        newFreeList->size = size;
        newFreeList->next = nextFreeList;
        if (prevFreeList != NULL) {
            prevFreeList->next = newFreeList;
        } else {
            freeList = newFreeList;
        }
    }

    pthread_mutex_unlock(&freeListMutex);
}
