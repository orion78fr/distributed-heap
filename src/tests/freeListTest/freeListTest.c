#include "common.h"

#define HEAPSIZE 1024

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
         
    return 0;
}
