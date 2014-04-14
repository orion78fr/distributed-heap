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

    /* On receptionne le type du message de réponse */
    if (read(heapInfo->sock, &msgtypeReponse, sizeof(msgtypeReponse)) <= 0){
        return DHEAP_ERROR_CONNECTION;
    }

    /* On verifie s'il y a eu une erreur ou non */
    /* TODO: && msgtype == MSG_RELEASE ?? */
    if (msgtypeReponse == MSG_ERROR){
        uint8_t tailleError;
        msgtypeReponse = ERROR_UNKNOWN_ERROR;

        /* On récupère le code d'erreur */
        if (read(heapInfo->sock, &msgtypeReponse, sizeof(msgtypeReponse)) <= 0){
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



void *data_thread(void *arg){
    /* TODO: à changer pour gérer plusieurs serveurs */
    struct pollfd poll_list[1];

    msgtypeClient = MSG_TYPE_NULL;

    /* Création du poll */
    poll_list[0].fd = heapInfo->sock;
    poll_list[0].events = POLLHUP | POLLIN | POLLNVAL; /* TODO: ajouter POLLERR? */

    /* Boucle avec le poll */
    while (1){
        int retval;
        retval = poll(poll_list, 1, -1);
        if (retval < 0){
            /* TODO: trouver une erreur */
            return ERROR_UNKNOWN_ERROR;
        }
        if (retval > 0){
           if (poll_list[0].revents&POLLNVAL == POLLNVAL){
                /* TODO: trouver une erreur */
               return ERROR_UNKNOWN_ERROR;
           }
           if (poll_list[0].revents&POLLHUP == POLLHUP){
            
           }
           if (poll_list[0].revents&POLLIN == POLLIN){
                
           }
        }
    }
}

