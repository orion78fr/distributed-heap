#include "common.h"

struct clientChain *clients = NULL;

/**
 * Envoie sur la socket sock les donn�es pass�es en argument
 * @param sock Socket de communication
 * @param msgType Le type de message (voir messages.h)
 * @param nb Nombre de donn�es � envoyer
 * @param ... les structures dataSend � envoyer
 * @return 0 si succ�s
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
    disconnect:
    return -1;
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
        nom = NULL; /* Le nom est stock� dans la structure et ne doit pas �tre free, m�me en cas d'erreur d'envoi */
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

int do_release(int sock){
    uint64_t offset;
    struct heapData *data;

    if (read(sock, (void *) &offset, sizeof(offset)) <= 0) { /* Offset */
        goto disconnect;
    }

    data = get_data_by_offset(offset);
    if(data == NULL){
        goto disconnect;
    }

#if DEBUG
        printf("[Client %d] Lib�ration de %s\n", pthread_self(),
           data->name);
#endif

    if(data->writeAccess != NULL && (pthread_equal(data->writeAccess->clientId, pthread_self()) != 0)){
        /* Lock en write */
        if (read(sock, theHeap+offset, data->size) <= 0) {        /* Contenu */
            goto disconnect;
        }
        release_write_lock(data);
    } else {
        struct clientChainRead *temp = data->readAccess;
        while(temp != NULL && (pthread_equal(pthread_self(), temp->clientId) ==0)){
            temp = temp->next;
        }
        if(temp == NULL){
            if(send_error(sock, ERROR_NOT_LOCKED)<0){
                goto disconnect;
            }
        } else {
            release_read_lock(data);
        }
    }

    if(send_data(sock, MSG_RELEASE, 0)<0){
        goto disconnect;
    }

    return 0;
    disconnect:
    return -1;
}

int do_free(int sock){
    uint8_t taille;
    char *nom;

    if (read(sock, (void *) &taille, sizeof(taille)) <= 0) { /* Name size */
        goto disconnect;
    }

    nom = malloc((taille + 1) * sizeof(char));
    if(nom == NULL){
        goto disconnect;
    }

    nom[taille] = '\0';

    if (read(sock, nom, taille) <= 0) {        /* Name */
        goto disconnect;
    }

#if DEBUG
    printf("[Client %d] D�sallocation de %s\n", pthread_self(),
           nom);
#endif

    if(remove_data(nom) != 0){
        /* ERREUR */
        goto disconnect;
    } else {
        /* OK */
        if(send_data(sock, MSG_FREE, 0)<0){
            goto disconnect;
        }
    }

    return 0;
    disconnect:
    return -1;
}
