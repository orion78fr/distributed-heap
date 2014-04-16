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

    tmp = dheapServers;
    while (tmp->next != NULL)
        tmp = tmp->next;
    tmp->next = newServer;

    /* Connexion au serveur */
    /* Rajouter status à dheapServer dans le cas ou une erreur DHEAP CONNECTION */
    /* dire qu'on le considère comme un backup */


    return 0;
}

int switchMain(uint8_t id){
    /* Remplace le sock de heapInfo et le mainId */
    /* Potentielement changer le status de l'ancienne machine */
    return 0;
}

void cleanServers(){
    /* Déconnecte de tous les serveurs et free les structures */
}
