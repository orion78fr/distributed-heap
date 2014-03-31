#include "distributedHeap.h"

/**
 * Alloue de la mémoire pour un nom de variable donné dans le tas
 * @param taille à allouer (en octet), nom de la variable
 * @return enum errorCodes
 */
int t_malloc(int size, char *name){
    int msgtype, tmp;

#if DEBUG
    printf("Appel t_malloc(%d, %s)\n", size, name);
#endif 

    /* On envoie le type de message (ALLOC) */
    msgtype = MSG_ALLOC;
    if (write(heapInfo->sock, &msgtype, sizeof(msgtype)) == -1){
        return DHEAP_ERROR_CONNECTION;
    }

    /* On envoie la longueur du nom à allouer */
    tmp = strlen(name);
    if (write(heapInfo->sock, &tmp, sizeof(tmp)) <= 0){
        return DHEAP_ERROR_CONNECTION;
    }

    /* On envoie le nom */
    if (write(heapInfo->sock, name, strlen(name)) <= 0){
        return DHEAP_ERROR_CONNECTION;
    }

    /* On envoie la taille qu'on veut allouer */
    if (write(heapInfo->sock, &size, sizeof(size)) <= 0){
        return DHEAP_ERROR_CONNECTION;
    }

    return receiveAck();
}

/**
 * Libérer l'espace mémoire lié à une variable du tas
 * @param nom de la variable
 * @return enum errorCodes
 */
int t_free(char *name){
    int msgtype, tmp;

#if DEBUG
    printf("Appel t_free(%s)\n", name);
#endif 

    /* On envoie le type de message (FREE) */
    msgtype = MSG_FREE;
    if (write(heapInfo->sock, &msgtype, sizeof(msgtype)) == -1){
        return DHEAP_ERROR_CONNECTION;
    }

    /* On envoie la longueur du nom à free */
    tmp = strlen(name);
    if (write(heapInfo->sock, &tmp, sizeof(tmp)) <= 0){
        return DHEAP_ERROR_CONNECTION;
    }

    /* On envoie le nom */
    if (write(heapInfo->sock, name, strlen(name)) <= 0){
        return DHEAP_ERROR_CONNECTION;
    }

    return receiveAck();
}
