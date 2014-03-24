#include "distributedHeap.h"

/* TODO: factoriser write & read */

int t_access_read(char *name, void **p){
    int msgtype, tmp;

    /* On envoie le type de message (READ) */
    msgtype = MSG_ACCESS_READ;
    if (write(heapInfo->sock, &msgtype, sizeof(msgtype)) == -1){
        return DHEAP_ERROR_CONNECTION;
    }

    /* On envoie la longueur du nom qu'on veut lire */
    tmp = strlen(name);
    if (write(heapInfo->sock, &tmp, sizeof(tmp)) <= 0){
        return DHEAP_ERROR_CONNECTION;
    }

    /* On envoie le nom */
    if (write(heapInfo->sock, name, strlen(name)) <= 0){
        return DHEAP_ERROR_CONNECTION;
    }
    
    /* On traite le retour en cas d'erreur */
    if ((tmp = receiveAck()) != DHEAP_SUCCESS){
        return tmp;
    } else {
        /* On traite le retour en cas de success */
        int offset, tailleContent;
        char bool;
        struct dheapVar *dv;
        /* On récupère l'offset ou est située la variable */
        if (read(heapInfo->sock, &offset, sizeof(offset)) <= 0){
            return DHEAP_ERROR_CONNECTION;
        }

        /* On récupère la taille */
        if (read(heapInfo->sock, &tailleContent, sizeof(tailleContent)) <= 0){
            return DHEAP_ERROR_CONNECTION;
        }

        /* On récupère le booléen (qui siginifie: modification ou non) */
        if (read(heapInfo->sock, &bool, sizeof(bool)) <= 0){
            return DHEAP_ERROR_CONNECTION;
        }

        /* Si faux, alors on renvoie le pointeur directement */
        p = heapInfo->heapStart + offset;

        /* Si vrai: */
        if (bool != 0) {
            /* On récupère le contenu directement dans le pointeur */
            if (read(heapInfo->sock, p, tailleContent) <= 0){
                return DHEAP_ERROR_CONNECTION;
            }
        }

        /* On ajoute la variable dans la hashtable */
        dv = malloc(sizeof(struct dheapVar));
        dv->p = p;
        dv->size = tailleContent;
        dv->rw = DHEAPVAR_READ;
        /* TODO: initialiser dv->next à NULL? */
        /* TODO: gerer l'erreur possible sur la hashtable ? */
        add_var(dv);

        return DHEAP_SUCCESS;
    }
}

int t_access_write(char *name, void *p){
    int msgtype, tmp;

    /* On envoie le type de message (WRITE) */
    msgtype = MSG_ACCESS_WRITE;
    if (write(heapInfo->sock, &msgtype, sizeof(msgtype)) == -1){
        return DHEAP_ERROR_CONNECTION;
    }

    /* On envoie la longueur du nom qu'on veut en écriture */
    tmp = strlen(name);
    if (write(heapInfo->sock, &tmp, sizeof(tmp)) <= 0){
        return DHEAP_ERROR_CONNECTION;
    }

    /* On envoie le nom */
    if (write(heapInfo->sock, name, strlen(name)) <= 0){
        return DHEAP_ERROR_CONNECTION;
    }
    
    /* On traite le retour en cas d'erreur */
    if ((tmp = receiveAck()) != DHEAP_SUCCESS){
        return tmp;
    } else {
        /* On traite le retour en cas de success */
        int offset, tailleContent;
        char bool;
        struct dheapVar *dv;
        /* On récupère l'offset ou est située la variable */
        if (read(heapInfo->sock, &offset, sizeof(offset)) <= 0){
            return DHEAP_ERROR_CONNECTION;
        }

        /* On récupère la taille */
        if (read(heapInfo->sock, &tailleContent, sizeof(tailleContent)) <= 0){
            return DHEAP_ERROR_CONNECTION;
        }

        /* On récupère le booléen (qui siginifie: modification ou non) */
        if (read(heapInfo->sock, &bool, sizeof(bool)) <= 0){
            return DHEAP_ERROR_CONNECTION;
        }

        /* Si faux, alors on renvoie le pointeur directement */
        p = heapInfo->heapStart + offset;

        /* Si vrai: */
        if (bool != 0) {
            /* On récupère le contenu directement dans le pointeur */
            if (read(heapInfo->sock, p, tailleContent) <= 0){
                return DHEAP_ERROR_CONNECTION;
            }
        }

        /* On ajoute la variable dans la hashtable */
        dv = malloc(sizeof(struct dheapVar));
        dv->p = p;
        dv->size = tailleContent;
        dv->rw = DHEAPVAR_WRITE;
        /* TODO: initialiser dv->next à NULL? */
        /* TODO: gerer l'erreur possible sur la hashtable ? */
        add_var(dv);

        return DHEAP_SUCCESS;
    }

    return 0;
}


int t_release(void *p){
    int msgtype;
    struct dheapVar *dv;

    /* On vérifie que le pointeur passé est bien dans la zone du tas réparti */
    if ( p > heapInfo->heapStart || p < (heapInfo->heapStart - heapInfo->heapSize)){
        return DHEAP_ERROR_BAD_POINTER;
    }

    /* On récupère le dheapVar dans la hashtable correspondant à cet accès */
    dv = getVarFromPointer(p);
    if (dv == NULL){
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

    /* On envoie la taille */
    if (write(heapInfo->sock, &(dv->size), sizeof(dv->size)) == -1){
        return DHEAP_ERROR_CONNECTION;
    }

    /* On envoie le contenu */
    if (write(heapInfo->sock, p, dv->size) == -1){
        return DHEAP_ERROR_CONNECTION;
    }

    /* On supprime la variable de la hashtable */
    /* TODO: gérer l'erreur sur le retour de remove_var ? */
    remove_var(p);

    /* On s'occupe de l'acquittement ou de l'erreur */
    return receiveAck();
}
