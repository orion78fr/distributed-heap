#include "distributedHeap.h"

/* TODO:
 * - Gestion du msg type, dans le cas ou le serveur envoie une erreur directement
 */

struct heapInfo *heapInfo;
char *dheapErrorMsg;

int init_data(){
    struct sockaddr_in servaddr;
    /* struct addrinfo hints, *result; */
    int msgtype;

    heapInfo = malloc(sizeof(struct heapInfo));

    heapInfo->sock = socket(AF_INET,SOCK_STREAM,0);
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
    servaddr.sin_addr.s_addr=inet_addr(DHEAP_SERVER_ADDRESS);
    servaddr.sin_port=htons(DHEAP_SERVER_PORT);

    if (connect(heapInfo->sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1){
        return DHEAP_ERROR_CONNECTION;
    }

    /* Reception du type de message (MSG_HEAP_SIZE) */			
    if (read(heapInfo->sock, &msgtype, sizeof(int)) <= 0){
        return DHEAP_ERROR_CONNECTION;
    }

    if (msgtype != MSG_HEAP_SIZE){
        return DHEAP_ERROR_UNEXPECTED_MSG;
    }

    /* Reception de la taille du tas */			
    if (read(heapInfo->sock,&(heapInfo->heapSize),sizeof(heapInfo->heapSize)) <= 0){
        return DHEAP_ERROR_CONNECTION;
    }

    /* TODO: DEBUG */
    printf("HeapSize: %d\n", heapInfo->heapSize);

    /* allocation du tas dans la mémoire */
    heapInfo->heapStart = malloc(heapInfo->heapSize);
    if (heapInfo->heapStart == NULL){
        return DHEAP_ERROR_HEAP_ALLOC;
    }

    return DHEAP_SUCCESS;
}

int close_data(){
    /* Fermeture de la connexion */
    if (close(heapInfo->sock) == -1){
        /* TODO: quelle erreur renvoyer? */
    }

    /* On vide le tas */
    free(heapInfo->heapStart);

    /* On vide la structure heapInfo */
    free(heapInfo);

    return DHEAP_SUCCESS;
}

int t_malloc(int size, char *name){
    int msgtype, tmp;

    /* On envoie le type de message (ALLOC) */
    msgtype = MSG_ALLOC;
    if (write(heapInfo->sock, &msgtype, sizeof(msgtype)) == -1){
        return DHEAP_ERROR_CONNECTION;
    }

    /* On envoie la longueur du nom à allouer */
    tmp = strlen(name);
    if (write(heapInfo->sock, &tmp, sizeof(tmp)) <= 0){
        return DHEAP_ERROR_CONNECTION;
    }

    /* On envoie le nom */
    if (write(heapInfo->sock, name, strlen(name)) <= 0){
        return DHEAP_ERROR_CONNECTION;
    }

    /* On envoie la taille qu'on veut allouer */
    if (write(heapInfo->sock, &size, sizeof(size)) <= 0){
        return DHEAP_ERROR_CONNECTION;
    }

    return receiveAck();
}

int t_access_read(char *name, void **p){
    /* - envoie de la requete de read
     * - erreur write
     * - allocation dans le tas
     * - retour success */

    int msgtype, tmp;

    /* On envoie le type de message (ALLOC) */
    msgtype = MSG_ACCESS_READ;
    if (write(heapInfo->sock, &msgtype, sizeof(msgtype)) == -1){
        return DHEAP_ERROR_CONNECTION;
    }

    /* On envoie la longueur du nom à allouer */
    tmp = strlen(name);
    if (write(heapInfo->sock, &tmp, sizeof(tmp)) <= 0){
        return DHEAP_ERROR_CONNECTION;
    }

    /* On envoie le nom */
    if (write(heapInfo->sock, name, strlen(name)) <= 0){
        return DHEAP_ERROR_CONNECTION;
    }
    
    /* On traite le retour */
    if ((tmp = receiveAck()) != DHEAP_SUCCESS){
        return tmp;
    } else {
        int offset;
        char bool;
        /* On récupère l'offset ou est située la variable */
        if (read(heapInfo->sock, &offset, sizeof(offset)) <= 0){
            return DHEAP_ERROR_CONNECTION;
        }

        /* On récupère le booléen (qui siginifie: modification ou non */
        if (read(heapInfo->sock, &bool, sizeof(bool)) <= 0){
            return DHEAP_ERROR_CONNECTION;
        }
        
        /* Si faux, alors on renvoie le pointeur directement */
        if (bool == 0){
            *p = heapInfo->heapStart + offset;
            return DHEAP_SUCCESS;
        } else {
            int tailleContent;
            /* Si vrai, on récupère la taille */
            if (read(heapInfo->sock, &tailleContent, sizeof(tailleContent)) <= 0){
                return DHEAP_ERROR_CONNECTION;
            }

            /* Puis le contenu */
            if (read(heapInfo->sock, *p, tailleContent) <= 0){
                return DHEAP_ERROR_CONNECTION;
            }

            /* On renvoie le pointeur */
            return DHEAP_SUCCESS;
        }
    }
}

int t_access_write(char *name, void *p){

    return 0;
}


/* TODO: hashtable et compagnie, HAVE FUN ! (pour vicelard) */
int t_release(void *p){
    int msgtype, tmp;

    /* On vérifie que le pointeur passé est bien dans la zone du tas réparti */
    if ( p > heapInfo->heapStart || p < (heapInfo->heapStart - heapInfo->heapSize)){
        return DHEAP_ERROR_BAD_POINTER;
    }

    /* On envoie le type de message (RELEASE) */
    msgtype = MSG_RELEASE;
    if (write(heapInfo->sock, &msgtype, sizeof(msgtype)) == -1){
        return DHEAP_ERROR_CONNECTION;
    }

    /* On envoie le pointeur */
    if (write(heapInfo->sock, p, sizeof(p)) == -1){
        return DHEAP_ERROR_CONNECTION;
    }

    return receiveAck();
}

int t_free(char *name){
    int msgtype, tmp;

    /* On envoie le type de message (FREE) */
    msgtype = MSG_FREE;
    if (write(heapInfo->sock, &msgtype, sizeof(msgtype)) == -1){
        return DHEAP_ERROR_CONNECTION;
    }

    /* On envoie la longueur du nom à free */
    tmp = strlen(name);
    if (write(heapInfo->sock, &tmp, sizeof(tmp)) <= 0){
        return DHEAP_ERROR_CONNECTION;
    }

    /* On envoie le nom */
    if (write(heapInfo->sock, name, strlen(name)) <= 0){
        return DHEAP_ERROR_CONNECTION;
    }

    return receiveAck();
}

int receiveAck(){
    /* On receptionne le type du message de réponse */
    int tmp = 0;
    if (read(heapInfo->sock, &tmp, sizeof(tmp)) <= 0){
        return DHEAP_ERROR_CONNECTION;
    }

    /* On verifie s'il y a eu une erreur ou non */
    /* TODO: && msgtype == MSG_RELEASE ?? */
    if (tmp == MSG_ERROR){
        tmp = 0;
        int tailleError;

        /* On récupère le code d'erreur */
        if (read(heapInfo->sock, &tmp, sizeof(tmp)) <= 0){
            return DHEAP_ERROR_CONNECTION;
        }

        /* On récupère la taille du message d'erreur */
        if (read(heapInfo->sock, &tailleError, sizeof(tailleError)) <= 0){
            return DHEAP_ERROR_CONNECTION;
        }

        /* Si la taille est supérieur à 0 on récupère le message */
        if (tailleError > 0){
            if (dheapErrorMsg != NULL){
                free(dheapErrorMsg);
            }
            dheapErrorMsg = malloc(sizeof(char)*tailleError);
            if (read(heapInfo->sock, dheapErrorMsg, tailleError) <=0 ){
                return DHEAP_ERROR_CONNECTION;
            }
        } else if (tailleError < 0){
            /* TODO: erreur à traiter */
        }

        /* On retourne le code d'erreur */
        return tmp;
        
    } else {
        /* return success */
        return DHEAP_SUCCESS;
    }
}



void data_thread(){
    /* Pour l'instant inutile, pourrait etre utilisé dans le cas ou
     * fait un thread en plus dans le client pour maintenir la connexion */
}

