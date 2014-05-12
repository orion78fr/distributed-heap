#include "common.h"

struct clientChain *clients = NULL;
struct serverChain *servers = NULL;

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

#if DEBUG
    static int num = 0;
    printf("Envoi %d:\n", num++);
    printf("\tmsgType: %d\n", msgType);
#endif


    if(write(sock, &msgType, sizeof(msgType)) <= 0){
        return -1;
    }


    va_start(ap, nb);

    for(i=0;i<nb;i++){
        snd = va_arg(ap, DS);
#if DEBUG
        printf("\tSize %d\n", snd.taille);
#endif
        if(write(sock, snd.data, snd.taille) <= 0){
            return -1;
        }
    }

#if DEBUG
    printf("\n");
#endif

    va_end(ap);

    return 0;
}

int send_error(int sock, uint8_t errType){
    return send_data(sock, MSG_ERROR, 1, (DS){sizeof(errType), &errType});
}

/* TODO ajouter l'envoi des @ des autres serveurs et leurs id */
int do_greetings(int sock, uint16_t clientId){

    if(send_data(sock, MSG_HELLO_NEW, 3,
                (DS){sizeof(parameters.serverNum), &(parameters.serverNum)},
                (DS){sizeof(clientId), &(clientId)},
                (DS){sizeof(parameters.heapSize), &(parameters.heapSize)})<0){
       goto disconnect;
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

#if DEBUG
    printf("[Client %d] demande Allocation\n",
           *(uint16_t*)pthread_getspecific(id));
#endif

    if (read(sock, (void *) &taille, sizeof(taille)) <= 0) { /* Name size */
        goto disconnect;
    }

#if DEBUG
    printf("taille nom %d\n",taille);
#endif

    nom = malloc((taille + 1) * sizeof(char));
    if(nom == NULL){
        goto disconnect;
    }

    nom[taille] = '\0';

    if (read(sock, (void *) nom, taille) <= 0) {        /* Name */
        goto disconnect;
    }

#if DEBUG
    printf("nom %s\n",nom);
#endif

    if (read(sock, (void *) &varSize, sizeof(varSize)) <= 0) { /* Var size */
        goto disconnect;
    }

#if DEBUG
    printf("size %d\n",varSize);
#endif



    if ((err = add_data(nom, varSize)) != 0) {
        /* ERREUR */
        if(err == -1){
            if(retry){
                nom = NULL; /* Le nom est stocké dans la structure et ne doit pas être free, même en cas d'erreur d'envoi */
                if(send_data(sock, MSG_ALLOC, 0)<0){
                    goto disconnect;
                }
                return 0;
            }
            if(send_error(sock, ERROR_VAR_NAME_EXIST)<0){
                goto disconnect;
            }
        } else {
            if(send_error(sock, ERROR_HEAP_FULL)<0){
                goto disconnect;
            }
        }
    } else {

#if DEBUG
    printf("[Client %d] Allocation réussi de %s de taille %d\n",
           *(uint16_t*)pthread_getspecific(id), nom, varSize);
#endif

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
        printf("[Client %d] Libération de %s\n", *(uint16_t*)pthread_getspecific(id),
           data->name);
#endif

    if(retry){
        int testRead=0, testWrite=0;
        struct clientChainRead *temp = data->readAccess;
        while(temp != NULL && (*(uint16_t*)pthread_getspecific(id) != temp->clientId)){
            temp = temp->next;
        }
        if(temp == NULL){
            testRead = 1;
        }
        if(data->writeAccess == NULL){
            testWrite = 1;
        }else if(data->writeAccess->clientId != *(uint16_t*)pthread_getspecific(id)){
            testWrite = 1;
        }

        if(testRead && testWrite){
            if(send_data(sock, MSG_RELEASE, 0)<0){
                goto disconnect;
            }
            return 0;
        }
    }

    if((data->writeAccess != NULL) && (data->writeAccess->clientId == *(uint16_t*)pthread_getspecific(id))){
        /* Lock en write */
        if (read(sock, theHeap+offset, data->size) <= 0) {        /* Contenu */
            goto disconnect;
        }

        if(servers!=NULL && !servers->backup){
#if DEBUG
            printf("avant lock rep MAJ_DATA\n");
#endif            
            pthread_mutex_lock(&rep->mutex_server);
            if(servers!=NULL){
                rep->modification = MAJ_DATA;
                rep->data = data;
                rep->clientId = 0;
#if DEBUG
            printf("avant lock ack MAJ_DATA\n");
#endif 
                pthread_mutex_lock(&ack->mutex_server);
                pthread_cond_signal(&rep->cond_server);
                pthread_mutex_unlock(&rep->mutex_server);
#if DEBUG
            printf("avant cond wait MAJ_DATA\n");
#endif 
                if(!ack->modification){
                    pthread_cond_wait(&ack->cond_server, &ack->mutex_server);
                }
                pthread_mutex_unlock(&ack->mutex_server);
#if DEBUG
                printf("MAJ_DATA, replication done\n");
#endif
            }else{
                pthread_mutex_unlock(&rep->mutex_server);
#if DEBUG
                printf("MAJ_DATA, replication abandonné\n");
#endif
            }    
        }
        release_write_lock(data);
    } else {
        struct clientChainRead *temp = data->readAccess;
        while(temp != NULL && (*(uint16_t*)pthread_getspecific(id) != temp->clientId)){
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
    printf("[Client %d] Désallocation de %s\n", *(uint16_t*)pthread_getspecific(id),
           nom);
#endif

    if(remove_data(nom) != 0){
        if(retry){
            if(send_data(sock, MSG_FREE, 0)<0){
                goto disconnect;
            }
        }
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

int do_pong(int sock){
    return send_data(sock, MSG_PING, 0);
}

int read_rep(int sock, void *buf, size_t count){
    struct pollfd poll_rep[1];
    int retval;
    int retour;

    poll_rep[0].fd = sock;
    poll_rep[0].events = POLLIN | POLLNVAL | POLLHUP;

    retval = poll(poll_rep, 1, 4000);
    if (retval < 0){
        /* problème */
        return -1;
    } else if (retval == 0){
        /* ici on considère déconnecté -> return < 0 */
        return -1;
    }
    if ( (poll_rep[0].revents&POLLHUP) == POLLHUP || (poll_rep[0].revents&POLLNVAL) == POLLNVAL ){
        /* ici on a déco */
        return -1;
    } else if ((poll_rep[0].revents&POLLIN) == POLLIN){
        /* ici on fait le read de size_t */
        if ((retour=read(sock, buf, count)) <= 0) {
            return -1;
        }
    }
    return retour;
}