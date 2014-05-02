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
    newServer->address = malloc(sizeof(char)*strlen(address));

    newServer->id = id;
    newServer->address=malloc(addlen);
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

    pthread_mutex_unlock(&polllock);

    buildPollList();
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
