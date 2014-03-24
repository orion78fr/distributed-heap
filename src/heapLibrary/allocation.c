#include "distributedHeap.h"

int t_malloc(int size, char *name){
    int msgtype, tmp;

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

int t_free(char *name){
    int msgtype, tmp;

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
