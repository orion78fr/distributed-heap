#include "common.h"

struct clientChain *clients = NULL;

struct tempCorrespondance{
    int offset;
    char *name;
    int write;
    struct tempCorrespondance *next;
} *corresp = NULL;


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
    printf("[Client %d] Demande d'accès en lecture de %s\n",
           pthread_self(), nom);
#endif

    if(acquire_read_lock(nom) != 0) {
        /* ERREUR */
        if(send_error(sock, ERROR_VAR_DOESNT_EXIST)<0){
            goto disconnect;
        }
    } else {
        /* TODO à refaire */
        struct heapData *data = get_data(nom);
        int offset = data->offset;
        int size = data->size;

        struct tempCorrespondance *newData = malloc(sizeof(struct tempCorrespondance));
        newData->offset = offset;
        newData->name = nom;
        nom = NULL;
        newData->next = corresp;
        newData->write = 0;
        corresp = newData;

        /* OK */
        if(send_data(sock, MSG_ACCESS_READ_MODIFIED, 3,
                        (DS){sizeof(offset), &offset},
                        (DS){sizeof(size), &size},
                        (DS){size, theHeap + data->offset})<0){
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

int do_access_write(int sock){
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
    printf("[Client %d] Demande d'accès en écriture de %s\n",
           pthread_self(), nom);
#endif

    if(acquire_write_lock(nom) != 0) {
        /* ERREUR */
        if(send_error(sock, ERROR_VAR_DOESNT_EXIST)<0){
            goto disconnect;
        }
    } else {
        struct heapData *data = get_data(nom);
        int offset = data->offset;
        int size = data->size;

        struct tempCorrespondance *newData = malloc(sizeof(struct tempCorrespondance));
        newData->offset = offset;
        newData->name = nom;
        nom = NULL;
        newData->next = corresp;
        newData->write = 1;
        corresp = newData;

        /* OK */
        if(send_data(sock, MSG_ACCESS_WRITE_MODIFIED, 3,
                        (DS){sizeof(offset), &offset},
                        (DS){sizeof(size), &size},
                        (DS){size, theHeap + data->offset})<0){
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

    if (read(sock, (void *) &offset, sizeof(offset)) <= 0) { /* Offset */
        goto disconnect;
    }

    struct tempCorrespondance *prevData = NULL, *data = corresp;
    while(data != NULL && data->offset != offset){
        prevData = data;
        data = data->next;
    }
    if(data == NULL){
        /* ERREUR */
        if(send_error(sock, ERROR_NOT_LOCKED)<0){
            goto disconnect;
        }
    } else {

#if DEBUG
        printf("[Client %d] Libération de %s\n", pthread_self(),
           data->name);
#endif

        if(data->write){
            uint64_t varSize;
            if (read(sock, (void *) &varSize, sizeof(varSize)) <= 0) {        /* Taille */
                goto disconnect;
            }
            if (read(sock, theHeap+offset, varSize) <= 0) {        /* Contenu */
                goto disconnect;
            }
            release_write_lock(data->name);
        } else {
            release_read_lock(data->name);
        }
        if(prevData == NULL){
            corresp = data->next;
        } else {
            prevData->next = data->next;
        }
        free(data->name);
        free(data);
        if(send_data(sock, MSG_RELEASE, 0)<0){
            goto disconnect;
        }
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
    printf("[Client %d] Désallocation de %s\n", pthread_self(),
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
