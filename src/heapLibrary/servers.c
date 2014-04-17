#include "distributedHeap.h"

int addserver(uint8_t id, char *address, int port){
    struct dheapServer *newServer, *tmp;
    int addlen;
    uint8_t msgtype;

    addlen = strlen(address);

    newServer = malloc(sizeof(struct dheapServer));

    newServer->id = id;
    strncpy(newServer->address, address, addlen);
    newServer->port = port;
    newServer->next = NULL;
    newServer->status = 0;

    tmp = dheapServers;
    while (tmp->next != NULL)
        tmp = tmp->next;
    tmp->next = newServer;

    /* Connexion au serveur */
    if ((newServer->sock = connectToServer(newServer->address, newServer->port)) == -1){
        return 0;
    }
    newServer->status = 1;
    
    /* Déclaration au serveur */
    msgtype = MSG_HELLO_NOT_NEW;
    if (write(newServer->sock, &msgtype, sizeof(msgtype)) == -1){
        setServerDown(newServer->id);
        return 0;
    }
    if (write(newServer->sock, &(heapInfo->clientId), sizeof(heapInfo->clientId)) == -1){
        setServerDown(newServer->id);
        return 0;
    }
    if (read(newServer->sock, &msgtype, sizeof(msgtype)) <= 0){
        setServerDown(newServer->id);
        return 0;
    }

    /* Ajout dans la poll list */
    buildPollList();

    return 0;
}

void setServerDown(uint8_t id){
    struct dheapServer *tmp;

    tmp = dheapServers;
    while (tmp->id != id && tmp != NULL)
        tmp = tmp->next;

    if (tmp == NULL)
        exit(EXIT_FAILURE);

    close(tmp->sock);
    tmp->sock = -1;
    tmp->status = 0;

    buildPollList();
}

void buildPollList(){
    struct pollfd *old, *new;
    struct dheapServer *ds;
    int j = 0;

    old = poll_list;

    countServersOnline = 0;
    ds = dheapServers;
    while (dh != NULL){
        if (dh->status == 1 && dh->sock != -1)
            countServersOnline++;
        dh = dh->next;
    }

    if (countServersOnline == 0){
        free(old);
        exit_data_thread(DHEAP_ERROR_CONNECTION);
        return;
    }

    new = malloc(sizeof(struct pollfd)*countServersOnline);
    ds = dheapServers;
    while(dh != NULL){
        if (j >= countServersOnline)
            exit(EXIT_FAILURE);
        if (dh->status == 1 && dh->sock != -1){
            new[j].fd = ds->sock;
            new[j].events = POLLHUP | POLLIN | POLLNVAL; /* POLLERR aussi? */
            j++;
        }
    }

    poll_list = new;
    free(old);
}

int switchMain(){
    /* Remplace le sock de heapInfo et le mainId */
    return 0;
}

void cleanServers(){
    struct dheapServer *tmp, *tofree;

    tmp = dheapServers;

    while (tmp != NULL){
        tofree = tmp;
        if (tmp->sock != -1){
            /* TODO: envoyer un msg_disconnect ? */
            close(tmp->sock); /* TODO: vérifier erreur? */
        }
        free(tmp->address);
        tmp = tmp->next;
        free(tofree);
    }    
}

/* retourne un socket ou -1 */
int connectToServer(char *address, int port){
    struct sockaddr_in servaddr;
    /* struct addrinfo hints, *result; */
    int socket, retry;

    socket = socket(AF_INET, SOCK_STREAM, 0);

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
    while(connect(socket, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1){
        retry++;
        if (retry == NB_SERVER_RETRY) 
            return -1;
    }

    return socket;
}

uint8_t getServerIdBySock(int sock){
    struct dheapServer *tmp;

    tmp = dheapServers;

    while (tmp->sock != sock && tmp != NULL)
        tmp = tmp->next;

    if (tmp == NULL)
        exit(EXIT_FAILURE);

    return tmp->id;
}
