#include "common.h"

int do_access_read(int sock){
    uint8_t taille;
    char *nom;
    struct heapData *data;

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

    data = get_data(nom);

    if(data == NULL){
        if(send_error(sock, ERROR_VAR_DOESNT_EXIST)<0){
            goto disconnect;
        }
    } else {
        if(do_access_read_common(sock, data)<0){
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
int do_access_read_by_offset(int sock){
    uint64_t offset;
    struct heapData *data;
    if (read(sock, (void *) &offset, sizeof(offset)) <= 0) { /* Name size */
        goto disconnect;
    }

    data = get_data_by_offset(offset);

    if(data == NULL){
        if(send_error(sock, ERROR_VAR_DOESNT_EXIST)<0){
            goto disconnect;
        }
    } else {
        if(do_access_read_common(sock, data)<0){
            goto disconnect;
        }
    }

    return 0;
    disconnect:
    return -1;
}

int do_access_read_common(int sock, struct heapData *data){
#if DEBUG
    printf("[Client %d] Demande d'accès en lecture de %s\n",
           pthread_self(), data->name);
#endif

    if(acquire_read_lock(data) != 0) {
        /* ERREUR */
        if(send_error(sock, ERROR_VAR_DOESNT_EXIST)<0){
            goto disconnect;
        }
    } else {
        /* TODO à refaire */
        int offset = data->offset;
        int size = data->size;

        struct tempCorrespondance *newData = malloc(sizeof(struct tempCorrespondance));
        newData->offset = offset;
        newData->name = data->name;
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
    return -1;
}

int do_access_write(int sock){
    uint8_t taille;
    char *nom;
    struct heapData *data;

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

    data = get_data(nom);

    if(data == NULL){
        if(send_error(sock, ERROR_VAR_DOESNT_EXIST)<0){
            goto disconnect;
        }
    } else {
        if(do_access_write_common(sock, data)<0){
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

int do_access_write_by_offset(int sock){
    uint64_t offset;
    struct heapData *data;
    if (read(sock, (void *) &offset, sizeof(offset)) <= 0) { /* Name size */
        goto disconnect;
    }

    data = get_data_by_offset(offset);

    if(data == NULL){
        if(send_error(sock, ERROR_VAR_DOESNT_EXIST)<0){
            goto disconnect;
        }
    } else {
        if(do_access_write_common(sock, data)<0){
            goto disconnect;
        }
    }

    return 0;
    disconnect:
    return -1;
}

int do_access_write_common(int sock, struct heapData *data){

#if DEBUG
    printf("[Client %d] Demande d'accès en écriture de %s\n",
           pthread_self(), data->name);
#endif

    if(acquire_write_lock(data) != 0) {
        /* ERREUR */
        if(send_error(sock, ERROR_VAR_DOESNT_EXIST)<0){
            goto disconnect;
        }
    } else {
        int offset = data->offset;
        int size = data->size;

        struct tempCorrespondance *newData = malloc(sizeof(struct tempCorrespondance));
        newData->offset = offset;
        newData->name = data->name;
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
    return -1;
}
