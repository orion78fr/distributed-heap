#include "distributedHeap.h"

struct heapInfo heapInfo;

int init_data(){
    struct sockaddr_in dest;
    struct addrinfo *result;
    struct addrinfo hints = {};

    /* Création du socket */
    if ((heapInfo.sock = socket(AF_INET,SOCK_STREAM,0)) == -1){
        printf("1\n");
        return DHEAP_ERROR_CONNECTION;
    }

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_ADDRCONFIG | AI_CANONNAME;
    hints.ai_protocol = 0;

    if (getaddrinfo(DHEAP_SERVER_ADDRESS, 0, &hints, &result) != 0){
        printf("2\n");
        return DHEAP_ERROR_CONNECTION;
    }
    memset((void *)&dest,0, sizeof(dest));
    memcpy((void*)&((struct sockaddr_in*)result->ai_addr)->sin_addr, (void*)&dest.sin_addr ,sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(DHEAP_SERVER_PORT);

    /* Connexion au serveur */
    if (connect(heapInfo.sock, (struct sockaddr *) &dest, sizeof(dest)) == -1) {
        printf("3\n");
        return DHEAP_ERROR_CONNECTION;
    }

    /* Reception de la taille du tas */			
    if (read(heapInfo.sock,&(heapInfo.heapSize),sizeof(heapInfo.heapSize)) == -1){
        printf("4\n");
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

