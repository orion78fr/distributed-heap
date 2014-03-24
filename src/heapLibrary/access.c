#include "distributedHeap.h"

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
        *p = heapInfo->heapStart + offset;

        /* Si vrai: */
        if (bool != 0) {
            int tailleContent;
            /* Si vrai, on récupère la taille */
            if (read(heapInfo->sock, &tailleContent, sizeof(tailleContent)) <= 0){
                return DHEAP_ERROR_CONNECTION;
            }

            /* Puis le contenu directement dans le pointeur */
            if (read(heapInfo->sock, *p, tailleContent) <= 0){
                return DHEAP_ERROR_CONNECTION;
            }
        }

        /* TODO: ajouter le add_var ici */

        return DHEAP_SUCCESS;
    }
}

int t_access_write(char *name, void *p){

    return 0;
}


/* TODO: ici utiliser la hashtable pour connaitre la taille et renvoyer le
 * contenu au serveur */
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
