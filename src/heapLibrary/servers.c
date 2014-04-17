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
    if ((newServer->sock = connectToServer(newServer->address, newServer->port)) == -1){
        /* TODO: Reessayer de se connecter en cas d'échec */
        return 0;
    }
    newServer->status = 1;
    countServersOnline++;

    /* Ajout dans la poll list */
    buildPollList();

    /* TODO: dire qu'on le considère comme un backup */

    return 0;
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

    new = malloc(sizeof(struct pollfd)*countServersOnline);
    ds = dheapServers;
    while(dh != NULL){
        if (j >= countServersOnline)
            /* TODO: voir si on a pas besoin d'un mutex pour bloquer l'ajout/suppr de servers */
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

int switchMain(uint8_t id){
    /* Remplace le sock de heapInfo et le mainId */
    /* Potentielement changer le status de l'ancienne machine */
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
    int socket;

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

    if (connect(socket, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1){
        return -1;
    }

    return socket;
}
