#include "distributedHeap.h"

/**
 * Ajoute un serveur à la liste de serveurs, puis lance la connexion,
 * et rebuild la poll list pour le prendre en compte.
 * @param id du nouveau serveur, adresse, port
 * @return toujours 0
 */
int addserver(uint8_t id, char *address, int port){
    struct dheapServer *newServer, *tmp;
    int addlen;

    addlen = strlen(address);

    newServer = malloc(sizeof(struct dheapServer));

    newServer->id = id;
    strncpy(newServer->address, address, addlen);
    newServer->port = port;
    newServer->next = NULL;
    newServer->status = 0;
    newServer->lastMsgTime = 0;
    newServer->lastPing = 0;

    tmp = dheapServers;
    pthread_mutex_lock(&polllock);
    while (tmp->next != NULL)
        tmp = tmp->next;
    tmp->next = newServer;
    tmp->lastConnect = time(NULL);

    /* Connexion au serveur */
    if ((newServer->sock = connectToServer(newServer->address, newServer->port, 0)) == -1){
        return 0;
    }
    newServer->status = 2;
    pthread_mutex_unlock(&polllock);
    
    buildPollList();
    
    return 0;
}

/**
 * Lance la reconnexion aux serveurs qui ne sont pas connectés,
 * seulement si la dernière tentative est plus ancienne que la durée
 * du timeout du pong
 */
void reconnectServers(){
    struct dheapServer *tmp;

    pthread_mutex_lock(&polllock); /* TODO: peut etre que ce lock n'est pas necessaire */
    
    tmp = dheapServers;
    while (tmp != NULL){
        if (tmp->status == 0 && tmp->lastConnect < (time(NULL)-PONG_TIMEOUT)){
            tmp->lastConnect = time(NULL);
            if ((tmp->sock = connectToServer(tmp->address, tmp->port, 0)) != -1){
                tmp->status = 2;
            }
        }
        tmp = tmp->next;
    }

    buildPollList();

    pthread_mutex_unlock(&polllock);
}

/**
 * Appelé quand le serveur est connecté, et envoie le client id qu'on
 * a reçu lors de la connexion au serveur main au moment de l'init_dat()
 * @param serveur id 
 */
void helloNotNew(uint8_t sid){
    /* Déclaration au serveur */
    uint8_t msgtype;
    struct dheapServer *ds;
    int sockflags;

    ds = getServerById(sid);

    pthread_mutex_lock(&polllock);
    ds->status = 1;
    pthread_mutex_unlock(&polllock);

    /* on remet la socket en bloquante */
    sockflags = fcntl(ds->sock, F_GETFL);
    sockflags &=~ O_NONBLOCK;
    if (fcntl(ds->sock, F_SETFL, sockflags) == -1)
       exit(EXIT_FAILURE); 

    msgtype = MSG_HELLO_NOT_NEW;
    if (write(ds->sock, &msgtype, sizeof(msgtype)) == -1){
        setServerDown(ds->id);
        return;
    }
    if (write(ds->sock, &(heapInfo->clientId), sizeof(heapInfo->clientId)) == -1){
        setServerDown(ds->id);
        return;
    }
    if (read(ds->sock, &msgtype, sizeof(msgtype)) <= 0){
        setServerDown(ds->id);
        return;
    }

    if (msgtype != MSG_HELLO_NOT_NEW){
        setServerDown(ds->id);
        return;
    }

    buildPollList();
}

/**
 * Ferme la connexion d'un serveur et le place sur down,
 * puis rebuild la poll list si on est le thread de la librairie.
 * Si on ne l'est pas, alors on ne veut pas interférer avec un poll
 * qui pourrait être en cours.
 * @param serveur id, buildPollList = 0 -> ne pas rebuild, 1 -> rebuild.
 */
void setServerDownInternal(uint8_t id, int doBuildPollList){
    struct dheapServer *tmp;
    uint8_t msgtype;

    tmp = dheapServers;
    pthread_mutex_lock(&polllock);
    while (tmp->id != id && tmp != NULL)
        tmp = tmp->next;

    if (tmp == NULL)
        exit(EXIT_FAILURE);

    if (tmp->sock == -1 && tmp->status == 0){
        return;
    }

    msgtype = MSG_DISCONNECT;
    /* Pas de vérification d'erreur nécessaire pour le write */
    write(tmp->sock, &msgtype, sizeof(msgtype));
    close(tmp->sock);
    tmp->sock = -1;
    tmp->status = 0;
    tmp->lastMsgTime = 0;
    tmp->lastPing = 0;

    pthread_mutex_unlock(&polllock);

    if (pthread_equal(pthread_self(), *dheap_tid) == 1 && doBuildPollList == 1)
        buildPollList();
}

void setServerDown(uint8_t id){
    setServerDownInternal(id, 1);
}
void setServerDownNoRebuild(uint8_t id){
    setServerDownInternal(id, 0);
}

/**
 * Créer la poll list à partir de la liste de serveurs et de leurs status
 */
void buildPollList(){
    struct pollfd *old, *new;
    struct dheapServer *ds;
    int j = 0;

#if DEBUG
    printf("Appel de buildPollList()\n");
#endif 

    old = poll_list;
    
    pthread_mutex_lock(&polllock);

    countServersOnline = 0;
    ds = dheapServers;
    while (ds != NULL){
        if ((ds->status == 1 || ds->status == 2) && ds->sock != -1)
            countServersOnline++;
        ds = ds->next;
    }

    if (countServersOnline == 0){
#if DEBUG
        printf("ERROR: 0 servers online\n");
#endif 
        free(old);
        exit_data_thread(DHEAP_ERROR_CONNECTION);
        return;
    }

    new = malloc(sizeof(struct pollfd)*countServersOnline);
    ds = dheapServers;
    while(ds != NULL){
        if (j > countServersOnline){
            free(new);
            exit(EXIT_FAILURE);
        }
        if (ds->status == 1 && ds->sock != -1){
            new[j].fd = ds->sock;
            new[j].events = POLLHUP | POLLIN | POLLNVAL;
            j++;
        } else if (ds->status == 2 && ds->sock != -1){
            new[j].fd = ds->sock;
            new[j].events = POLLOUT | POLLHUP | POLLNVAL;
            j++;
        }
        ds = ds->next;
    }

    poll_list = new;
    free(old);

    pthread_mutex_unlock(&polllock);

#if DEBUG
    printf("Fin de buildPollList(), %d servers online\n", countServersOnline);
#endif 
}

/**
 * Permet de changer de serveur main en cherchant un serveur avec le status à 1
 * @return 0 si un nouveau main a été trouvé, exit le thread sinon
 */
int switchMain(){
    struct dheapServer *ds;
#if DEBUG
        printf("Appel de switchMain()\n");
#endif 
    ds = dheapServers;
    while (ds != NULL && ds->status != 1){
        if (ds->status == 1){
            heapInfo->mainId = ds->id;
            heapInfo->sock = ds->sock;
            return 0;
        }
        ds = ds->next;
    }
    exit_data_thread(DHEAP_ERROR_CONNECTION);
    return -1;
}

/**
 * Supprime un serveur de la liste de serveurs
 * @param serveur id
 * @return 0 si réussi, -1 sinon
 */
int removeServer(uint8_t sid){
    struct dheapServer *tmp, *prev;

    tmp = dheapServers;
    prev = NULL;

    while (tmp != NULL){
        if (tmp->id == sid){
            setServerDown(sid);
            if (prev == NULL)
                dheapServers = tmp->next;
            else
                prev->next = tmp->next;
            free(tmp->address);
            free(tmp);
            return 0;
        }
        prev = tmp;
        tmp = tmp->next;
    }

    return -1;
}

/**
 * Ferme toutes les connexions aux serveurs, envoie un RELEASE ALL
 * au main pour être sur de libérer les accès en cours,
 * puis free l'espace alloué pour les structures des serveurs
 */
void cleanServers(){
    struct dheapServer *tmp, *tofree;

    tmp = dheapServers;

    while (tmp != NULL){
        tofree = tmp;
        if (tmp->sock != -1){
            uint8_t msgtype;
            if (tmp->id == heapInfo->mainId)
                msgtype = MSG_DISCONNECT_RELEASE_ALL;
            else
                msgtype = MSG_DISCONNECT;
            /* Pas de vérification d'erreur nécessaire pour le write */
            write(tmp->sock, &msgtype, sizeof(msgtype));
            close(tmp->sock);
        }
        free(tmp->address);
        tmp = tmp->next;
        free(tofree);
    }    
}

/**
 * Se connecte à un serveur et retourne une socket
 * @param adresse du serveur, port, block=0 -> connexion non bloquante, =1->bloquant
 * @return socket ou -1
 */
int connectToServer(char *address, int port, int block){
    struct sockaddr_in servaddr;
    /* struct addrinfo hints, *result; */
    int sock, socktype, retry;

    if (block == 0)
        socktype = SOCK_STREAM | SOCK_NONBLOCK;
    else
        socktype = SOCK_STREAM;
    sock = socket(AF_INET, socktype, 0);

    /* TODO: Gestion des hostname en plus des IPs
     * memset(&hints, 0, sizeof(hints));
     hints.ai_family = PF_UNSPEC; // AF_INET ou AF_INET6 pour ipv4 ou 6
     hints.ai_socktype = SOCK_STREAM;
     hints.ai_flags = AI_ADDRCONFIG | AI_CANONNAME;

     if (getaddrinfo(DHEAP_SERVER_ADDRESS, &hints, &result) == -1){
     return DHEAP_ERROR_CONNECTION;
     } */

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=inet_addr(address);
    servaddr.sin_port=htons(port);

    retry = 0;
    while(connect(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1){
        if (errno == EINPROGRESS && block == 0){
            return sock;
        }
        retry++;
        if (retry == DH_SERVER_RETRY) 
            return -1;
    }

    return sock;
}

/**
 * Appel setServerDown() pour fermer la connexion d'un serveur,
 * puis switchMain() dans le cas où le serveur fermé était le main
 * @param id du serveur
 */
void setDownAndSwitch(uint8_t sid){
#if DEBUG
    printf("Appel de setDownAndSwitch(%" PRIu8 ")\n", sid);
#endif 
    setServerDown(sid);
    if (sid == heapInfo->mainId)
        switchMain();
}


/**
 * Retourne l'id d'un serveur depuis sa socket
 * @param socket
 * @return serveur id
 */
uint8_t getServerIdBySock(int sock){
    struct dheapServer *tmp;

    tmp = dheapServers;

    while (tmp->sock != sock && tmp != NULL)
        tmp = tmp->next;

    if (tmp == NULL)
        exit(EXIT_FAILURE);

    return tmp->id;
}

/**
 * Retourne la structure d'un serveur depuis sa socket
 * @param socket
 * @return structure dheapServer associé à la socket
 */
struct dheapServer* getServerBySock(int sock){
    struct dheapServer *tmp;

    tmp = dheapServers;

    while (tmp->sock != sock && tmp != NULL)
        tmp = tmp->next;

    if (tmp == NULL)
        exit(EXIT_FAILURE);

    return tmp;
}

/**
 * Retourne la structure d'un serveur depuis son id
 * @param serveur id
 * @return structure dheapServer associé à l'id
 */
struct dheapServer* getServerById(uint8_t sid){
    struct dheapServer *tmp;

    tmp = dheapServers;

    while (tmp->id != sid && tmp != NULL)
        tmp = tmp->next;

    if (tmp == NULL)
        exit(EXIT_FAILURE);

    return tmp;
}

/**
 * Actualise l'heure du dernier message reçu d'un serveur
 * @param serveur id
 */
void setTime(uint8_t sid){
    struct dheapServer *ds;
    ds = getServerById(sid);
    ds->lastMsgTime = time(NULL);
}
