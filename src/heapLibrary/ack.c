#include "distributedHeap.h"

/**
 * Partie commune des fonctions de communication avec le serveur
 * permettant de traiter les erreurs et les acquittements
 * @return enum errorCodes
 */
int receiveAck(uint8_t msgtype){
    return receiveAckPointer(&msgtype);
}

int receiveAckPointer(uint8_t *msgtypeP){
    uint8_t msgtype;
    uint8_t msgtypeReponse;

#if DEBUG
    printf("Appel receiveAck()\n");
#endif 

    msgtype = *msgtypeP;

    pthread_mutex_lock(&readlock);

    /* On receptionne le type du message de réponse */
    msgtypeReponse = msgtypeClient;
    msgtypeClient = MSG_TYPE_NULL;

#if DEBUG
    printf("Réponse recu: %d\n", msgtypeReponse);
#endif 

    if (msgtypeReponse == MSG_RETRY){
        unlockAndSignal();
        return DHEAP_RETRY;
    }

    if (msgtypeReponse == MSG_DISCONNECT_RELEASE_ALL){
        return *dheapErrorNumber;
    }

    /* On verifie s'il y a eu une erreur ou non */
    if (msgtypeReponse == MSG_ERROR){
        /* On récupère le code d'erreur */
        if (read(heapInfo->sock, &msgtypeReponse, sizeof(msgtypeReponse)) <= 0){
            setDownAndSwitch(heapInfo->mainId);
            return DHEAP_RETRY;
        }

        /* On retourne le code d'erreur */
        return msgtypeReponse;
    } else {
        if (msgtypeReponse != msgtype){
            if (msgtype == MSG_ACCESS_READ && msgtypeReponse == MSG_ACCESS_READ_MODIFIED){
                *msgtypeP = MSG_ACCESS_READ_MODIFIED;
                return DHEAP_SUCCESS;
            } else if (msgtype == MSG_ACCESS_WRITE && msgtypeReponse == MSG_ACCESS_WRITE_MODIFIED){
                *msgtypeP = MSG_ACCESS_WRITE_MODIFIED;
                return DHEAP_SUCCESS;
            } else {
                return DHEAP_ERROR_UNEXPECTED_MSG;
            }
        } else {
            /* return success */
            return DHEAP_SUCCESS;
        }
    }
}

