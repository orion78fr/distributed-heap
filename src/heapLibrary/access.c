#include "distributedHeap.h"

/**
 * Partie commune à t_access_read et t_access_write
 * @param type du message, nom de la variable, pointeur pour la récupérer
 * @return enum errorCodes
 */
int t_access_common(uint8_t msgtype, char *name, void **p){
    uint8_t namelen, retack;

#if DEBUG
    printf("Appel t_access_common(%d, %s, p)\n", msgtype, name);
#endif

    /* On traite une erreur venant du thread de la librairie */
    if ((error = checkError()) != DHEAP_SUCCESS)
        return error;

    /* On envoie le msgtype */
    if (write(heapInfo->sock, &msgtype, sizeof(msgtype)) == -1){
        return DHEAP_ERROR_CONNECTION;
    }

    /* On envoie la longueur du nom qu'on veut lire */
    namelen = strlen(name);
    if (write(heapInfo->sock, &namelen, sizeof(namelen)) <= 0){
        return DHEAP_ERROR_CONNECTION;
    }

    /* On envoie le nom */
    if (write(heapInfo->sock, name, namelen) <= 0){
        return DHEAP_ERROR_CONNECTION;
    }
    
    /* On traite le retour en cas d'erreur */
    if ((retack = receiveAckPointer(&msgtype)) != DHEAP_SUCCESS){
        return retack;
    } else {
        /* On traite le retour en cas de success */
        uint64_t offset, tailleContent;
        struct dheapVar *dv;
        /* On récupère l'offset ou est située la variable */
        if (read(heapInfo->sock, &offset, sizeof(offset)) <= 0){
            return DHEAP_ERROR_CONNECTION;
        }

        /* On récupère la taille */
        if (read(heapInfo->sock, &tailleContent, sizeof(tailleContent)) <= 0){
            return DHEAP_ERROR_CONNECTION;
        }

        /* On place le pointeur au bon endroit */
        *p = heapInfo->heapStart + offset;

        /* Si vrai: */
        if (msgtype == MSG_ACCESS_READ_MODIFIED || msgtype == MSG_ACCESS_WRITE_MODIFIED) {
            /* On récupère le contenu directement dans le pointeur */
            if (read(heapInfo->sock, *p, tailleContent) <= 0){
                return DHEAP_ERROR_CONNECTION;
            }
        }

        /* On ajoute la variable dans la hashtable */
        dv = malloc(sizeof(struct dheapVar));
        dv->p = *p;
        dv->size = tailleContent;
        dv->next = NULL;
        if (msgtype == MSG_ACCESS_READ || MSG_ACCESS_READ_MODIFIED)
            dv->rw = DHEAPVAR_READ;
        else if (msgtype == MSG_ACCESS_WRITE || MSG_ACCESS_WRITE_MODIFIED)
            dv->rw = DHEAPVAR_WRITE;
        else
            return DHEAP_ERROR_UNEXPECTED_MSG;
        add_var(dv);

        return DHEAP_SUCCESS;
    }
}

/**
 * Demander l'accès en lecture sur une variable
 * @param nom de la variable, pointeur pour la récupérer
 * @return enum errorCodes
 */
int t_access_read(char *name, void **p){
    uint8_t msgtype;
#if DEBUG
    printf("Appel t_access_read(%s)\n", name);
#endif 
    msgtype = MSG_ACCESS_READ;
    return t_access_common(msgtype, name, p);
}

/**
 * Demander l'accès en écriture sur une variable
 * @param nom de la variable, pointeur pour la récupérer
 * @return enum errorCodes
 */
int t_access_write(char *name, void **p){
    uint8_t msgtype;
#if DEBUG
    printf("Appel t_access_write(%s)\n", name);
#endif 
    msgtype = MSG_ACCESS_WRITE;
    return t_access_common(msgtype, name, p);
}

/**
 * Rendre l'accès d'une variable
 * @param pointeur de la variable qu'on a obtenu lors de la demande d'accès
 * @return enum errorCodes
 */
int t_release(void *p){
    uint8_t msgtype;
    uint64_t offset = 0;
    struct dheapVar *dv;

#if DEBUG
    printf("Appel t_release()\n");
#endif 

    /* On traite une erreur venant du thread de la librairie */
    if ((error = checkError()) != DHEAP_SUCCESS)
        return error;

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

    /* On envoie l'offset */
    offset = (uint64_t)p - (uint64_t)(heapInfo->heapStart);
    if (write(heapInfo->sock, &offset, sizeof(offset)) == -1){
        return DHEAP_ERROR_CONNECTION;
    }

    /* On envoie la taille seulement si on était en écriture */
    if (dv->rw == DHEAPVAR_WRITE){
        if (write(heapInfo->sock, &(dv->size), sizeof(dv->size)) == -1){
            return DHEAP_ERROR_CONNECTION;
        }

        /* On envoie le contenu */
        if (write(heapInfo->sock, p, dv->size) == -1){
            return DHEAP_ERROR_CONNECTION;
        }
    }

    /* On supprime la variable de la hashtable */
    if (remove_var(p) != 0){
        exit(EXIT_FAILURE);
    }

    /* On s'occupe de l'acquittement ou de l'erreur */
    return receiveAck(MSG_RELEASE);
}
