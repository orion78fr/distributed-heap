#include "common.h"

struct clientChain *clients = NULL;
struct serverChain *servers = NULL;

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

int do_inquire(int sock){
    uint8_t msgType;

    if (read(sock, (void *) &msgType, sizeof(msgType)) <= 0) {       /* Msg type */
        goto disconnect;
    }

    if(msgType == MSG_HELLO_NEW_CLIENT){

        /* Ajout du client dans la cha�ne de socket (ajout au d�but pour
         * �viter le parcours) */
        newClient = malloc(sizeof(struct clientChain));
        newClient->sock = sock;
        newClient->next = clients;
        clients = newClient;

        /* Cr�ation d'un thread pour traiter les requ�tes */
        pthread_create((pthread_t *) & (newClient->clientId), NULL,
                       clientThread, (void *) newClient);
        clientsConnected++;
        return 1;

    }else if(msgType == MSG_HELLO_NEW_SERVER){

        pthread_mutex_lock(&schainlock);
        newServer = malloc(sizeof(struct serverChain));
        newServer->serverId = serversConnected;
        newServer->sock = sock;
        newServer->next = servers;
        servers = newServer;

        poll_list=realloc(poll_list,sizeof(struct pollfd)*serversConnected);
        poll_list[serversConnected].fd = newServer->sock;
        poll_list[serversConnected].events = POLLHUP | POLLIN | POLLNVAL;
        serversConnected++;

        pthread_mutex_unlock(&schainlock);

        if(send_data(sock, MSG_HELLO_NEW_SERVER, 3,
                        (DS){sizeof(parameters.serverNum), &(parameters.serverNum)},
                        (DS){sizeof(newServer->serverId), &(newServer->serverId)},
                        (DS){sizeof(parameters.heapSize), &(parameters.heapSize)})<0){
            goto disconnect;
        }
        return 2;
    }
    return -1;
}

/* TODO ajouter l'envoi des @ des autres serveurs et leurs id */
int do_greetings(int sock){
    uint8_t msgType;
    uint16_t clientId;

    
    clientId = sock;
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

int snd_total_replication(int sock){



}

int rcv_total_replication(int sock){
    int *servId;
    char **servAddress;
    uint8_t taille;
    char *nom;
    uint64_t offset;
    uint64_t varSize;
    struct heapData *data;
    uint8_t continuer;

    /* Replication des serveurs */
    if (read(sock, (void *) &serversConnected, sizeof(serversConnected)) <= 0) { /* Nombre serveurs */
        goto disconnect;
    }

    servAddress = malloc(serversConnected * sizeof(char*));
    for (int i = 0; i < serversConnected; i++)
        servAddress[i] = malloc(16);

    for(int i=0; i<serversConnected; i++){
        struct serverChain *newServer;
        if (read(sock, (void *) &newServer->serverId, sizeof(newServer->serverId)) <= 0) { /* Nombre serveurs */
            goto disconnect;
        } 
        if (read(sock, (void *) &servAddress[i], sizeof(servAddress[i])) <= 0) { /* Nombre serveurs */
            goto disconnect;
        } 
    }

    /* Replication des donn�es */

    if (read(sock, (void *) &continuer, sizeof(continuer)) <= 0) { /* nouvelle donnee a recevoir */
        goto disconnect;
    }

    while(continuer!=0){
        if (read(sock, (void *) &taille, sizeof(taille)) <= 0) { /* taille donnee a recevoir */
            goto disconnect;
        }   
        if (read(sock, (void *) &taille, sizeof(taille)) <= 0) { /* taille donnee a recevoir */
            goto disconnect;
        }   
    }


    /* connexion aux serveurs */


    /* envoi d'un message pour confirmer la replication totale */


}

int snd_partial_replication(int sock){

}

int rcv_partial_replication(int sock){
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

    if(data == NULL){/* cr�ation de la var */
#if DEBUG
    printf("[Server %d] Demande creation(replication partielle) de %s\n",
           pthread_self(), data->name);
#endif
        
        /* TODO */

    } else {/* mis � jour de la var */
#if DEBUG
    printf("[Server %d] Demande mis a jour(replication partielle) de %s\n",
           pthread_self(), data->name);
#endif

        /* TODO */  
    
    }
    return 0;
    disconnect:
    if(nom != NULL){
        free(nom);
    }
    return -1;
}
