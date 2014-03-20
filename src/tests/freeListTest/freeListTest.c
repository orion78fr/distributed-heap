/* Headers POSIX */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <getopt.h>


#define HEAPSIZE 2048

struct freeListChain {
    int startOffset;
    int size;
    struct freeListChain *next;
};

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

int main(int argc, char *argv)
{
    freeList = malloc(sizeof(struct freeListChain));
    freeList->startOffset = 0;
    freeList->size = HEAPSIZE;
    freeList->next = NULL;

    /* Test basique */
    printf("Alloc 8 byte (a) -- Offset : %d (should be 0)\n",
           alloc_space(8));
    printf("Alloc 8 byte (b) -- Offset : %d (should be 8)\n",
           alloc_space(8));
    printf("Free a\n");
    free_space(0, 8);
    printf("Alloc 8 byte (a) -- Offset : %d (should be 0)\n",
           alloc_space(8));
    printf("Alloc 8 byte (c) -- Offset : %d (should be 16)\n",
           alloc_space(8));
    printf("Free b\n");
    free_space(8, 8);
    printf("Alloc 16 byte (d) -- Offset : %d (should be 24)\n",
           alloc_space(16));
    printf("Alloc 4 byte (e) -- Offset : %d (should be 8)\n",
           alloc_space(4));
    printf("Alloc 4 byte (f) -- Offset : %d (should be 12)\n",
           alloc_space(4));

    /* Test de remplissage */
    while (alloc_space(4) != -1) {
    };
    printf("Here, all free space should be taken, %d should be NULL\n",
           freeList);

    /* Test lorsque la freeList est nulle */
    free_space(24, 16);
    printf("Free d, %d should NOT be NULL\n", freeList);

    /* Jointure à gauche */
    free_space(40, 4);

    /* Jointure à droite */
    free_space(20, 4);

    /* Jointure discontinue */
    free_space(48, 4);

    /* Jointure aux deux bouts */
    free_space(44, 4);

    printf
        ("Here, start offset %d should be 20 and size %d should be 32. %d should be NULL\n",
         freeList->startOffset, freeList->size, freeList->next);




}
