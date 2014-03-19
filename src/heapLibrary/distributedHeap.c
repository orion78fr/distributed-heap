#include "distributedHeap.h"

struct heapInfo heapInfo;

int init_data(){
    struct sockaddr_in servaddr,cliaddr;

    /* TODO: Ajouter la gestion d'erreur et du hostname */

    heapInfo.sock = socket(AF_INET,SOCK_STREAM,0);

    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=inet_addr(DHEAP_SERVER_ADDRESS);
    servaddr.sin_port=htons(DHEAP_SERVER_PORT);

    connect(heapInfo.sock, (struct sockaddr *)&servaddr, sizeof(servaddr));

    /* Reception de la taille du tas */			
    if (read(heapInfo.sock,&(heapInfo.heapSize),sizeof(heapInfo.heapSize)) == -1){
        /* TODO: ajouter une vérification de la taille du tas */
        return DHEAP_ERROR_CONNECTION;
    }

    return 0;
}


int close_data(){

}

int t_malloc(int size, char *name){
    /* envoie de la requete de malloc
     * si erreur sur le write -> return error
     */

    /* return success */
    return 0;

}

int t_access_read(char *name, void *p){
    /* - envoie de la requete de read
     * - erreur write
     * - allocation dans le tas
     * - retour success */
}

int t_access_write(char *name, void *p){

}

int t_release(void *p){

}

int t_free(char *name){

}


void data_thread(){
    /* Pour l'instant inutile, pourrait etre utilisé dans le cas ou
     * fait un thread en plus dans le client pour maintenir la connexion */
}

