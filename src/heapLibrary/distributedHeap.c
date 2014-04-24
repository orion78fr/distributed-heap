#include "distributedHeap.h"

void unlockAndSignal(){
    pthread_cond_signal(&readcond);
    pthread_mutex_unlock(&readlock);
    pthread_mutex_lock(&readylock);
    pthread_mutex_unlock(&readylock);
}

int checkError(){
    int ret;
    if (dheapErrorNumber == NULL)
        return DHEAP_SUCCESS;

    ret = *dheapErrorNumber;
    free(dheapErrorNumber);
    dheapErrorNumber = NULL;

    return ret;
}

void setError(uint8_t e){
    if (dheapErrorNumber == NULL)
        dheapErrorNumber = malloc(sizeof(uint8_t));
    *dheapErrorNumber = e;
}
