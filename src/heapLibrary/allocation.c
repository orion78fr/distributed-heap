#include "distributedHeap.h"

/**
 * Alloue de la mémoire pour un nom de variable donné dans le tas
 * @param taille à allouer (en octet), nom de la variable
 * @return enum errorCodes
 */
int t_malloc(uint64_t size, char *name){
    uint8_t msgtype, namelen;
    int ret;
    int done = 0;

#if DEBUG
    printf("Appel t_malloc(%" PRIu64 ", %s)\n", size, name);
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
#if DEBUG
            printf("Envoie de MSG RETRY -> %" PRIu8 "\n", heapInfo->mainId);
#endif
            if (write(heapInfo->sock, &msgtype, sizeof(msgtype)) == -1){
                setDownAndSwitch(heapInfo->mainId);
                done = -1;
                continue;
            }
        }

        /* On envoie le type de message (ALLOC) */
        msgtype = MSG_ALLOC;
        if (write(heapInfo->sock, &msgtype, sizeof(msgtype)) == -1){
            setDownAndSwitch(heapInfo->mainId);
            done = -1;
            continue;
        }

        /* On envoie la longueur du nom à allouer */
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

        /* On envoie la taille qu'on veut allouer */
        if (write(heapInfo->sock, &size, sizeof(size)) == -1){
            setDownAndSwitch(heapInfo->mainId);
            done = -1;
            continue;
        }

        pthread_mutex_unlock(&writelock);

        ret = receiveAck(MSG_ALLOC);
        if (ret == DHEAP_RETRY){
            done = -1;
            continue;
        }
        pthread_mutex_unlock(&mainlock);

        done = 1;
    }

    
    unlockAndSignal();
    return ret;
}

/**
 * Libérer l'espace mémoire lié à une variable du tas
 * @param nom de la variable
 * @return enum errorCodes
 */
int t_free(char *name){
    uint8_t msgtype, namelen;
    int ret;
    int done = 0;

#if DEBUG
    printf("Appel t_free(%s)\n", name);
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
#if DEBUG
            printf("Envoie de MSG RETRY -> %" PRIu8 "\n", heapInfo->mainId);
#endif
            if (write(heapInfo->sock, &msgtype, sizeof(msgtype)) == -1){
                setDownAndSwitch(heapInfo->mainId);
                done = -1;
                continue;
            }
        }

        /* On envoie le type de message (FREE) */
        msgtype = MSG_FREE;
        if (write(heapInfo->sock, &msgtype, sizeof(msgtype)) == -1){
            setDownAndSwitch(heapInfo->mainId);
            done = -1;
            continue;
        }

        /* On envoie la longueur du nom à free */
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

        ret = receiveAck(MSG_FREE);
        if (ret == DHEAP_RETRY){
            done = -1;
            continue;
        }
        pthread_mutex_unlock(&mainlock);

        done = 1;

    }

    unlockAndSignal();
    return ret;
}
