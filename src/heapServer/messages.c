#include "common.h"

/**
 * Envoie sur la socket sock les données passées en argument
 * @param sock Socket de communication
 * @param msgType Le type de message (voir messages.h)
 * @param nb Nombre de données à envoyer
 * @param ... les structures dataSend à envoyer
 * @return 0 si succès
 */
int send_data(int sock, uint8_t msgType, int nb, ...){
    va_list ap;
    int i;
    DS snd = {0,NULL};

    if(write(sock, &msgType, sizeof(msgType)) <= 0){
        return -1;
    }

    va_start(ap, nb);

    for(i=0;i<nb;i++){
        snd = va_arg(ap, DS);
        if(write(sock, snd.data, snd.taille) <= 0){
            return -1;
        }
    }

    va_end(ap);

    return 0;
}

int send_error(int sock, uint8_t errType){
    return send_data(sock, MSG_ERROR, 1, (DS){sizeof(errType), &errType});
}

int do_greetings(int sock){
    uint8_t msgType;
    uint16_t clientId;

    if (read(sock, (void *) &msgType, sizeof(msgType)) <= 0) {       /* Msg type */
        goto disconnect;
    }

    if(msgType == MSG_HELLO_NEW){
        clientId = sock;
        if(send_data(sock, MSG_HELLO_NEW, 1,
                        (DS){sizeof(parameters.serverNum), &(parameters.serverNum)},
                        (DS){sizeof(clientId), &(clientId)},
                        (DS){sizeof(parameters.heapSize), &(parameters.heapSize)})<0){
            goto disconnect;
        }
    } else {
        if (read(sock, (void *) &clientId, sizeof(clientId)) <= 0) {       /* Msg type */
            goto disconnect;
        }
        /* TODO : stocker l'id du client... */
    }
    return 0;
    disconnect: return -1;
}

int do_alloc(int sock){
    uint8_t taille;
    char *nom;
    uint64_t varSize;
    int err;

    if (read(sock, (void *) &taille, sizeof(taille)) <= 0) { /* Name size */
        goto disconnect;
    }

    nom = malloc((taille + 1) * sizeof(char));
    if(nom == NULL){
        goto disconnect;
    }

    nom[taille] = '\0';

    if (read(sock, (void *) nom, taille) <= 0) {        /* Name */
        goto disconnect;
    }

    if (read(sock, (void *) &varSize, sizeof(varSize)) <= 0) { /* Var size */
        goto disconnect;
    }

#if DEBUG
    printf("[Client %d] Allocation de %s de taille %d\n",
           pthread_self(), nom, varSize);
#endif

    if ((err = add_data(nom, varSize)) != 0) {
        /* ERREUR */
        if(err == -1){
            if(send_error(sock, ERROR_VAR_NAME_EXIST)<0){
                goto disconnect;
            }
        } else {
            if(send_error(sock, ERROR_HEAP_FULL)<0){
                goto disconnect;
            }
        }
    } else {
        /* OK */
        nom = NULL; /* Le nom est stocké dans la structure et ne doit pas être free, même en cas d'erreur d'envoi */
        if(send_data(sock, MSG_ALLOC, 0)<0){
            goto disconnect;
        }
    }
    return 0;

disconnect:
    if(nom != NULL){
        free(nom);
    }
    return -1;
}

int do_access_read(int sock){

}

int do_access_write(int sock){

}

int do_release(int sock){

}

int do_free(int sock){

}
