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

    msgtypeClient = MSG_TYPE_NULL;

    /* Création du poll */
    buildPollList();

    /* Boucle avec le poll */
    while (1){
        int retval, i;
        retval = poll(poll_list, countServersOnline, -1);
        if (retval < 0){
            perror("poll");
            exit(EXIT_FAILURE); 
        }
        if (retval > 0){
            for (i=0; i < countServersOnline; i++){
                if (poll_list[i].revents&POLLNVAL == POLLNVAL){
                    perror("poll socket"); 
                    exit(EXIT_FAILURE); 
                }
                if (poll_list[i].revents&POLLHUP == POLLHUP){
                    setDownAndSwitch(poll_list[i].fd);
                    continue;
                }
                if (poll_list[i].revents&POLLIN == POLLIN){
                    uint8_t msgtype;
                    if (read(poll_list[i].fd, &msgtype, sizeof(msgtype)) <= 0){
                        setDownAndSwitch(poll_list[i].fd);
                        continue;
                    }
                    if (msgtype < MSG_ERROR && msgtype > MSG_SERVER_ID){
                        /* On passe le message à l'autre thread */
                    } else if (msgtype == MSG_ERROR){
                        /* On traite l'erreur */
                    } else if (msgtype == MSG_DISCONNECT){

                    } else if (msgtype == MSG_PING){

                    } else if (msgtype == MSG_ADD_SERVER){
                        /* Comme on modifie la poll_list, on repart dans la boucle */
                        continue;
                    } else if (msgtype == MSG_REM_SERVER){

                    } else {
                        exit_data_thread(DHEAP_ERROR_UNEXPECTED_MSG);
                    }
                }
            }
        }
    }
}

void setDownAndSwitch(int sock){
    uint8_t sid;
    sid = getServerIdBySock(sock);
    setServerDown(sid);
    if (sid == heapInfo->mainId)
        switchMain();
}

/**
 * Ferme le thread et laisse un message d'erreur
 * @param errorCodes
 */
void exit_data_thread(int e){
    if (dheapErrorNumber == NULL){
        dheapErrorNumber = malloc(sizeof(int));
    }
    *dheapErrorNumber = e;
    pthread_exit(NULL);
}

