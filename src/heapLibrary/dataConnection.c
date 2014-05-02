#include "distributedHeap.h"

struct heapInfo *heapInfo;
struct dheapServer *dheapServers;
pthread_t *dheap_tid;
uint8_t msgtypeClient, *dheapErrorNumber;
int countServersOnline;
struct pollfd *poll_list;
pthread_mutex_t readlock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t writelock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mainlock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t readylock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t polllock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t readcond = PTHREAD_COND_INITIALIZER;

/**
 * Initialise la connexion avec le serveur central
 * Alloue l'espace pour le tas local
 * Initialisation de la hashtable stockant les accès en cours
 * @param ip du serveur, port du serveur
 * @return enum errorCodes
 */
int init_data(char *address, int port){
    uint8_t msgtype;

#if DEBUG
    printf("Appel init_data(%s, %d)\n", address, port);
#endif

    /* Appel du close_data() au cas ou on aurait été déconnecté
     * et que l'on relance la connexion */
    close_data();

    /* TODO: vérifier la validité du port et de l'ip */
    
    heapInfo = malloc(sizeof(struct heapInfo));
    heapInfo->mainId = 0;
    dheapServers = malloc(sizeof(struct dheapServer));
    dheapServers->id = 0;
    dheapServers->address = malloc(sizeof(char)*strlen(address));
    strncpy(dheapServers->address, address, strlen(address));
    dheapServers->port = port;
    dheapServers->next = NULL;
    dheapServers->lastConnect = time(NULL);
    poll_list = NULL;

    dheapErrorNumber = NULL;


    if ((heapInfo->sock = connectToServer(address, port, 1)) == -1){
        return DHEAP_ERROR_CONNECTION;
    }
    
    /* Reception du type de message (MSG_HELLO) */			
    if (read(heapInfo->sock, &msgtype, sizeof(msgtype)) <= 0){
        return DHEAP_ERROR_CONNECTION;
    }
    if (msgtype != MSG_HELLO){
        close_data();
        return DHEAP_ERROR_CONNECTION;
    }

    dheapServers->status = 1;
    dheapServers->sock = heapInfo->sock;

    /* on indique au serveur qu'on est nouveau dans le système */
    msgtype = MSG_HELLO_NEW;
    if (write(heapInfo->sock, &msgtype, sizeof(msgtype)) <= 0){
        return DHEAP_ERROR_CONNECTION;
    }

    /* Reception du type de message (MSG_HELLO_NEW) */			
    if (read(heapInfo->sock, &msgtype, sizeof(msgtype)) <= 0){
        return DHEAP_ERROR_CONNECTION;
    }

    if (msgtype != MSG_HELLO_NEW){
        return DHEAP_ERROR_UNEXPECTED_MSG;
    }

    /* Reception de l'id du serveur auquel on se connecte */
    if (read(heapInfo->sock, &(heapInfo->mainId), sizeof(heapInfo->mainId)) <= 0){
        return DHEAP_ERROR_CONNECTION;
    }
    dheapServers->id = heapInfo->mainId;

    /* Réception de notre id client */
    if (read(heapInfo->sock, &(heapInfo->clientId), sizeof(heapInfo->clientId)) <= 0){
        return DHEAP_ERROR_CONNECTION;
    }

    /* Reception de la taille du tas */			
    if (read(heapInfo->sock,&(heapInfo->heapSize),sizeof(heapInfo->heapSize)) <= 0){
        return DHEAP_ERROR_CONNECTION;
    }

    dheapServers->lastMsgTime = time(NULL);
    dheapServers->lastPing = 0;

#if DEBUG
    printf("HeapSize: %" PRIu64 "\n", heapInfo->heapSize);
    printf("ClientID: %" PRIu16 "\n", heapInfo->clientId);
#endif 

    /* allocation du tas dans la mémoire */
    heapInfo->heapStart = malloc(heapInfo->heapSize);
    if (heapInfo->heapStart == NULL){
        return DHEAP_ERROR_HEAP_ALLOC;
    }

    /* initialisation de la hashtable */
    init_hashtable();

    /* création du thread client */
    dheap_tid = malloc(sizeof(pthread_t));
    if (pthread_create(dheap_tid, NULL, data_thread, NULL) == -1){
        perror("pthread_create"); 
        exit(EXIT_FAILURE);
    }

    pthread_mutex_lock(&readlock);
    pthread_cond_wait(&readcond, &readlock);
    pthread_cond_signal(&readcond);
    pthread_mutex_unlock(&readlock);

#if DEBUG
    printf("Succes init_data()\n");
#endif 

    return DHEAP_SUCCESS;
}

/**
 * Ferme la connexion et libère toute les données liées au tas
 * @return enum errorCodes
 */
int close_data(){
    void *threadStatus;
    if (heapInfo == NULL){
#if DEBUG
    printf("Appel close_data() mais heapInfo NULL\n");
#endif 
        return DHEAP_SUCCESS;
    }

#if DEBUG
    printf("Appel close_data()\n");
#endif

    /* Fermeture du thread client */
    if (dheap_tid != NULL && pthread_equal(pthread_self(), *dheap_tid)){
        if (pthread_cancel(*dheap_tid) != 0){
            perror("pthread_cancel"); 
            exit(EXIT_FAILURE);
        }
        if (pthread_join(*dheap_tid, (void**) &threadStatus) != 0){
            perror("pthread_join");
            exit(EXIT_FAILURE);
        }
        if (threadStatus != PTHREAD_CANCELED){
            exit(EXIT_FAILURE); /* TODO: erreur à revoir */
        }
        free(dheap_tid);
    }

    /* Fermeture des connexions */
    cleanServers();

    /* On vide le tas */
    free(heapInfo->heapStart);

    /* On vide la structure heapInfo */
    free(heapInfo);
    heapInfo = NULL;

    /* On supprime la hashtable */
    free_hashtable();

    /* On vide le numero d'erreur */
    if (dheapErrorNumber != NULL && msgtypeClient != MSG_DISCONNECT_RELEASE_ALL){
        free(dheapErrorNumber);
        dheapErrorNumber = NULL;
    }

    countServersOnline = 0;
    msgtypeClient = MSG_TYPE_NULL;

    if (poll_list != NULL){
        free(poll_list);
        poll_list = NULL;
    }

    return DHEAP_SUCCESS;
}
