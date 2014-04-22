#include "distributedHeap.h"

/**
 * Alloue de la mémoire pour un nom de variable donné dans le tas
 * @param taille à allouer (en octet), nom de la variable
 * @return enum errorCodes
 */
int t_malloc(uint64_t size, char *name){
    uint8_t msgtype, namelen;
    int ret;

#if DEBUG
    printf("Appel t_malloc(%" PRIu64 ", %s)\n", size, name);
#endif

    /* On traite une erreur venant du thread de la librairie */
    if ((ret = checkError()) != DHEAP_SUCCESS)
        return ret;

    /* On envoie le type de message (ALLOC) */
    msgtype = MSG_ALLOC;
    if (write(heapInfo->sock, &msgtype, sizeof(msgtype)) == -1){
        return DHEAP_ERROR_CONNECTION;
    }

    /* On envoie la longueur du nom à allouer */
    namelen = strlen(name);
    if (write(heapInfo->sock, &namelen, sizeof(namelen)) == -1){
        return DHEAP_ERROR_CONNECTION;
    }

    /* On envoie le nom */
    if (write(heapInfo->sock, name, namelen) == -1){
        return DHEAP_ERROR_CONNECTION;
    }

    /* On envoie la taille qu'on veut allouer */
    if (write(heapInfo->sock, &size, sizeof(size)) == -1){
        return DHEAP_ERROR_CONNECTION;
    }

    ret = receiveAck(MSG_ALLOC);
    unlockAndSignal();
    return ret;
}

/**
 * Libérer l'espace mémoire lié à une variable du tas
 * @param nom de la variable
 * @return enum errorCodes
 */
int t_free(char *name){
    uint8_t msgtype, namelen;
    int ret;

#if DEBUG
    printf("Appel t_free(%s)\n", name);
#endif 

    /* On traite une erreur venant du thread de la librairie */
    if ((ret = checkError()) != DHEAP_SUCCESS)
        return ret;

    /* On envoie le type de message (FREE) */
    msgtype = MSG_FREE;
    if (write(heapInfo->sock, &msgtype, sizeof(msgtype)) == -1){
        return DHEAP_ERROR_CONNECTION;
    }

    /* On envoie la longueur du nom à free */
    namelen = strlen(name);
    if (write(heapInfo->sock, &namelen, sizeof(namelen)) == -1){
        return DHEAP_ERROR_CONNECTION;
    }

    /* On envoie le nom */
    if (write(heapInfo->sock, name, namelen) == -1){
        return DHEAP_ERROR_CONNECTION;
    }

    ret = receiveAck(MSG_FREE);
    unlockAndSignal();
    return ret;
}
