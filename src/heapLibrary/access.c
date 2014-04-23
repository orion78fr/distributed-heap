#include "distributedHeap.h"

/**
 * Partie commune à t_access_read et t_access_write
 * @param type du message, nom de la variable, pointeur pour la récupérer
 * @return enum errorCodes
 */
int t_access_common(uint8_t msgtype, char *name, void **p){
    uint8_t namelen, retack;
    int ret;
    int done = 0;

#if DEBUG
    printf("Appel t_access_common(%d, %s, p)\n", msgtype, name);
#endif

    while (done <= 0){
        pthread_mutex_lock(&mainlock);
        pthread_mutex_lock(&writelock);
        
        /* On traite une erreur venant du thread de la librairie */
        if ((ret = checkError()) != DHEAP_SUCCESS){
            pthread_mutex_unlock(&mainlock);
            pthread_mutex_unlock(&writelock);
            return ret;
        }
        
        if (done == -1){
            msgtype = MSG_RETRY;
            if (write(heapInfo->sock, &msgtype, sizeof(msgtype)) == -1){
                setDownAndSwitch(heapInfo->mainId);
                done = -1;
                continue;
            }
        }

        /* On envoie le msgtype */
        if (write(heapInfo->sock, &msgtype, sizeof(msgtype)) == -1){
            setDownAndSwitch(heapInfo->mainId);
            done = -1;
            continue;
        }

        /* On envoie la longueur du nom qu'on veut lire */
        namelen = strlen(name);
        if (write(heapInfo->sock, &namelen, sizeof(namelen)) == -1){
            setDownAndSwitch(heapInfo->mainId);
            done = -1;
            continue;
        }

        /* On envoie le nom */
        if (write(heapInfo->sock, name, namelen) == -1){
            setDownAndSwitch(heapInfo->mainId);
            done = -1;
            continue;
        }

        pthread_mutex_unlock(&writelock);

        /* On traite le retour en cas d'erreur */
        if ((retack = receiveAckPointer(&msgtype)) != DHEAP_SUCCESS){
            if (retack == DHEAP_RETRY){
                done = -1;
                continue;
            } else {
                pthread_mutex_unlock(&mainlock);
                unlockAndSignal();
                return retack;
            }
        } else {
            /* On traite le retour en cas de success */
            uint64_t offset, tailleContent;
            struct dheapVar *dv;
            /* On récupère l'offset ou est située la variable */
            if (read(heapInfo->sock, &offset, sizeof(offset)) <= 0){
                setDownAndSwitch(heapInfo->mainId);
                done = -1;
                continue;
            }

            /* On récupère la taille */
            if (read(heapInfo->sock, &tailleContent, sizeof(tailleContent)) <= 0){
                setDownAndSwitch(heapInfo->mainId);
                done = -1;
                continue;
            }

            /* On place le pointeur au bon endroit */
            *p = heapInfo->heapStart + offset;

            /* Si vrai: */
            if (msgtype == MSG_ACCESS_READ_MODIFIED || msgtype == MSG_ACCESS_WRITE_MODIFIED) {
                /* On récupère le contenu directement dans le pointeur */
                if (read(heapInfo->sock, *p, tailleContent) <= 0){
                    setDownAndSwitch(heapInfo->mainId);
                    done = -1;
                    continue;
                }
            }

            done = 1;

            /* On ajoute la variable dans la hashtable */
            dv = malloc(sizeof(struct dheapVar));
            dv->p = *p;
            dv->size = tailleContent;
            dv->next = NULL;

            pthread_mutex_unlock(&mainlock);
            unlockAndSignal();

            if (msgtype == MSG_ACCESS_READ || msgtype == MSG_ACCESS_READ_MODIFIED)
                dv->rw = DHEAPVAR_READ;
            else if (msgtype == MSG_ACCESS_WRITE || msgtype == MSG_ACCESS_WRITE_MODIFIED)
                dv->rw = DHEAPVAR_WRITE;
            else
                return DHEAP_ERROR_UNEXPECTED_MSG;
            add_var(dv);

            return DHEAP_SUCCESS;
        }
    }

    /* Whaaaaaaaaaaaaaaaaaat ???? */
    return ERROR_UNKNOWN_ERROR;
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
    int ret;
    int done = 0;

#if DEBUG
    printf("Appel t_release()\n");
#endif 

    while (done <= 0){
        pthread_mutex_lock(&mainlock);
        pthread_mutex_lock(&writelock);

        /* On traite une erreur venant du thread de la librairie */
        if ((ret = checkError()) != DHEAP_SUCCESS){
            pthread_mutex_unlock(&mainlock);
            pthread_mutex_unlock(&writelock);
            return ret;
        }

        if (done == -1){
            msgtype = MSG_RETRY;
            if (write(heapInfo->sock, &msgtype, sizeof(msgtype)) == -1){
                setDownAndSwitch(heapInfo->mainId);
                done = -1;
                continue;
            }
        }

        /* On vérifie que le pointeur passé est bien dans la zone du tas réparti */
        if ( p < heapInfo->heapStart || p > (heapInfo->heapStart + heapInfo->heapSize)){
            pthread_mutex_lock(&mainlock);
            pthread_mutex_lock(&writelock);
            return DHEAP_ERROR_BAD_POINTER;
        }

        /* On récupère le dheapVar dans la hashtable correspondant à cet accès */
        dv = getVarFromPointer(p);
        if (dv == NULL){
            pthread_mutex_lock(&mainlock);
            pthread_mutex_lock(&writelock);
            return DHEAP_ERROR_BAD_POINTER;
        }

        /* On envoie le type de message (RELEASE) */
        msgtype = MSG_RELEASE;
        if (write(heapInfo->sock, &msgtype, sizeof(msgtype)) == -1){
            setDownAndSwitch(heapInfo->mainId);
            done = -1;
            continue;
        }

        /* On envoie l'offset */
        offset = (uint64_t)p - (uint64_t)(heapInfo->heapStart);
        if (write(heapInfo->sock, &offset, sizeof(offset)) == -1){
            setDownAndSwitch(heapInfo->mainId);
            done = -1;
            continue;
        }

        /* On envoie le contenu seulement si on était en écriture */
        if (dv->rw == DHEAPVAR_WRITE){
#if DEBUG
            printf("Envoie du contenu (taille = )\n");
#endif 
            if (write(heapInfo->sock, p, dv->size) == -1){
                setDownAndSwitch(heapInfo->mainId);
                done = -1;
                continue;
            }
        }
        
        pthread_mutex_unlock(&writelock);

        /* On s'occupe de l'acquittement ou de l'erreur */
        ret = receiveAck(MSG_RELEASE);
        if (ret == DHEAP_RETRY){
            done = -1;
            continue;
        }
        
        /* On supprime la variable de la hashtable */
        if (remove_var(p) != 0){
            exit(EXIT_FAILURE);
        }

        pthread_mutex_unlock(&mainlock);

        done = 1;

    }
    unlockAndSignal();
    return ret;
}
