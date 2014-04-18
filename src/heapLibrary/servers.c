#include "distributedHeap.h"

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

    tmp = dheapServers;
    while (tmp->next != NULL)
        tmp = tmp->next;
    tmp->next = newServer;

    /* Connexion au serveur */
    if ((newServer->sock = connectToServer(newServer->address, newServer->port, 0)) == -1){
        return 0;
    }
    newServer->status = 2;
    
    buildPollList();
    
    return 0;
}

void helloNotNew(uint8_t sid){
    /* Déclaration au serveur */
    uint8_t msgtype;
    struct dheapServer *ds;
    int sockflags;

    ds = getServerById(sid);

    ds->status = 1;

    /* on remet la socket en bloquante */
    sockflags = fcntl(ds->sock, F_GETFL);
    sockflags &=~ O_NONBLOCK;
    if (fcntl(ds->sock, F_SETFL, sockflags) == -1)
       exit(EXIT_FAILURE); 

    msgtype = MSG_HELLO_NOT_NEW;
    if (write(ds->sock, &msgtype, sizeof(msgtype)) == -1){
        setServerDown(newServer->id);
        return 0;
    }
    if (write(ds->sock, &(heapInfo->clientId), sizeof(heapInfo->clientId)) == -1){
        setServerDown(newServer->id);
        return 0;
    }
    if (read(ds->sock, &msgtype, sizeof(msgtype)) <= 0){
        setServerDown(newServer->id);
        return 0;
    }

    buildPollList();
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
    while (ds != NULL){
        if ((ds->status == 1 || ds->status == 2) && ds->sock != -1)
            countServersOnline++;
        ds = ds->next;
    }

    if (countServersOnline == 0){
        free(old);
        exit_data_thread(DHEAP_ERROR_CONNECTION);
        return;
    }

    new = malloc(sizeof(struct pollfd)*countServersOnline);
    ds = dheapServers;
    while(ds != NULL){
        if (j >= countServersOnline)
            exit(EXIT_FAILURE);
        if (ds->status == 1 && ds->sock != -1){
            new[j].fd = ds->sock;
            new[j].events = POLLHUP | POLLIN | POLLNVAL;
            j++;
        } else if (ds->status == 2 && ds->sock != -1){
            new[j].fd = ds->sock;
            new[j].events = POLLOUT | POLLHUP | POLLNVAL;
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
/* block: 0: not blocking, 1: blocking */
int connectToServer(char *address, int port, int block){
    struct sockaddr_in servaddr;
    /* struct addrinfo hints, *result; */
    int socket, socktype, retry;

    if (block == 0)
        socktype = SOCK_STREAM | SOCK_NONBLOCK;
    else
        socktype = SOCK_STREAM;
    socket = socket(AF_INET, socktype, 0);

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
        if (errno == EINPROGRESS && block == 0){
            return socket;
        }
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

struct dheapServer* getServerBySock(int sock){
    struct dheapServer *tmp;

    tmp = dheapServers;

    while (tmp->sock != sock && tmp != NULL)
        tmp = tmp->next;

    if (tmp == NULL)
        exit(EXIT_FAILURE);

    return tmp;
}

struct dheapServer* getServerById(uint8_t sid){
    struct dheapServer *tmp;

    tmp = dheapServers;

    while (tmp->id != sid && tmp != NULL)
        tmp = tmp->next;

    if (tmp == NULL)
        exit(EXIT_FAILURE);

    return tmp;
}
