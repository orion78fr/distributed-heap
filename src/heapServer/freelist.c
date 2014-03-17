#include "common.h"

struct freeListChain *freeList;

/**
 * Alloue un espace de taille size dans le tas
 * @param size Taille de la variable à allouer
 * @return L'offset de la variable
 */
int alloc_space(int size)
{
    struct freeListChain *tempFreeList = freeList;
    struct freeListChain *prevFreeList = NULL;
    int offset = -1;

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
		    if (tempFreeList->next != NULL) {
			/* On est au début et il y a quelque chose après */
			freeList = tempFreeList->next;
		    } else {
			/* Il n'y a plus rien de libre */
			freeList = NULL;
		    }
		}
		free(tempFreeList);
	    }
	    break;
	} else {
	    prevFreeList = tempFreeList;
	    tempFreeList = tempFreeList->next;
	}
    }
    /* GTU : Si -1, peut être lancer une défragmentation et réessayer */
    return offset;
}
