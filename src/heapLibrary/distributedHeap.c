#include "distributedHeap.h"

/**
 * Partie commune des fonctions de communication avec le serveur
 * permettant de traiter les erreurs et les acquittements
 * @return enum errorCodes
 */
int receiveAck(int msgtype){
    return receiveAckPointer(&msgtype);
}

int receiveAckPointer(int *msgtypeP){
    int msgtype;
    int tmp = 0;

#if DEBUG
    printf("Appel receiveAck()\n");
#endif 

    msgtype = *msgtypeP;

    /* On receptionne le type du message de réponse */
    if (read(heapInfo->sock, &tmp, sizeof(tmp)) <= 0){
        return DHEAP_ERROR_CONNECTION;
    }

    /* On verifie s'il y a eu une erreur ou non */
    /* TODO: && msgtype == MSG_RELEASE ?? */
    if (tmp == MSG_ERROR){
        tmp = 0;
        int tailleError;

        /* On récupère le code d'erreur */
        if (read(heapInfo->sock, &tmp, sizeof(tmp)) <= 0){
            return DHEAP_ERROR_CONNECTION;
        }

        /* On récupère la taille du message d'erreur */
        if (read(heapInfo->sock, &tailleError, sizeof(tailleError)) <= 0){
            return DHEAP_ERROR_CONNECTION;
        }

        /* Si la taille est supérieur à 0 on récupère le message */
        if (tailleError > 0){
            if (dheapErrorMsg != NULL){
                free(dheapErrorMsg);
            }
            dheapErrorMsg = malloc(sizeof(char)*tailleError);
            if (read(heapInfo->sock, dheapErrorMsg, tailleError) <=0 ){
                return DHEAP_ERROR_CONNECTION;
            }
        } else if (tailleError < 0){
            /* TODO: erreur à traiter */
        }

        /* On retourne le code d'erreur */
        return tmp;
    } else {
        if (tmp != msgtype){
            if (msgtype == MSG_ACCESS_READ && tmp == MSG_ACCESS_READ_MODIFIED){
                *msgtypeP = MSG_ACCESS_READ_MODIFIED;
                return DHEAP_SUCCESS;
            } else if (msgtype == MSG_ACCESS_WRITE && tmp == MSG_ACCESS_WRITE_MODIFIED){
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



void data_thread(){
    /* Pour l'instant inutile, pourrait etre utilisé dans le cas ou
     * fait un thread en plus dans le client pour maintenir la connexion */
}

