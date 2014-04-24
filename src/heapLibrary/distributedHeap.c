#include "distributedHeap.h"

/**
 * Libère le mutex read, et attend que le thread de la librairie
 * ait repris la main.
 * sched_yield() ne fonctionnant pas correctement, il a été
 * remplacé par le mutex readylock
 */
void unlockAndSignal(){
    pthread_cond_signal(&readcond);
    pthread_mutex_unlock(&readlock);
    pthread_mutex_lock(&readylock);
    pthread_mutex_unlock(&readylock);
}

/**
 * Vérifie si le thread de la librairie a rencontré une erreur
 * qu'il faut renvoyer au client.
 * @return enum errorCodes
 */
int checkError(){
    int ret;
    if (dheapErrorNumber == NULL)
        return DHEAP_SUCCESS;

    ret = *dheapErrorNumber;
    free(dheapErrorNumber);
    dheapErrorNumber = NULL;

    return ret;
}

/**
 * Place une erreur dans dheapErrorNumber, qui sera éventuellement
 * retournée au client via checkError()
 * @param enum errorCodes
 */
void setError(uint8_t e){
    if (dheapErrorNumber == NULL)
        dheapErrorNumber = malloc(sizeof(uint8_t));
    *dheapErrorNumber = e;
}
