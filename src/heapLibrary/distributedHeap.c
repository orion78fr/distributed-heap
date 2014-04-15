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
 * Thread de la librairie gérant les lectures et les ping/pong
 * @param aucun
 */
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
            perror("poll");
            exit(EXIT_FAILURE); 
        }
        if (retval > 0){
           if (poll_list[0].revents&POLLNVAL == POLLNVAL){
               perror("poll socket"); 
               exit(EXIT_FAILURE); 
           }
           if (poll_list[0].revents&POLLHUP == POLLHUP){
                exit
           }
           if (poll_list[0].revents&POLLIN == POLLIN){
               uint8_t msgtype;
               if (read(poll_list[0].fd, &msgtype, sizeof(msgtype)) <= 0){
                   exit(1); /* TODO: temporaire */
                   /* switchToBackup(DHEAP_ERROR_CONNECTION); TODO: à voir et à améliorer */
                   continue; 
               }
               if (msgtype < MSG_ERROR){
                   /* On passe le message à l'autre thread */
               } else if (msgtype == MSG_ERROR){
                   /* On traite l'erreur */
               } else {
                    exit_data_thread(DHEAP_ERROR_UNEXPECTED_MSG);
               }
           }
        }
    }
}

/**
 * Ferme le thread et laisse un message d'erreur */
 * @param errorCodes
 */
void exit_data_thread(int e){
    if (dheapErrorNumber == NULL){
        dheapErrorNumber = malloc(sizeof(int));
    }
    *dheapErrorNumber = e;
    pthread_exit(NULL);
}

