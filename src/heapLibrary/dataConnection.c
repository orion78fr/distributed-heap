#include "distributedHeap.h"

struct heapInfo *heapInfo;
char *dheapErrorMsg;
pthread_t *dheap_tid;
uint8_t *msgtypeClient, *dheapErrorNumber;

/**
 * Initialise la connexion avec le serveur central
 * Alloue l'espace pour le tas local
 * Initialisation de la hashtable stockant les accès en cours
 * @param ip du serveur, port du serveur
 * @return enum errorCodes
 */
int init_data(char *ip, int port){
    struct sockaddr_in servaddr;
    /* struct addrinfo hints, *result; */
    uint8_t msgtype;

#if DEBUG
    printf("Appel init_data(%s, %d)\n", ip, port);
#endif

    /* Appel du close_data() au cas ou on aurait été déconnecté
     * et que l'on relance la connexion */
    close_data();

    /* TODO: vérifier la validité du port et de l'ip */
    
    heapInfo = malloc(sizeof(struct heapInfo));
    dheapErrorMsg = NULL;
    dheapErrorNumber = NULL;

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
    servaddr.sin_addr.s_addr=inet_addr(ip);
    servaddr.sin_port=htons(port);

    if (connect(heapInfo->sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1){
        return DHEAP_ERROR_CONNECTION;
    }

    /* Reception du type de message (MSG_HEAP_SIZE) */			
    if (read(heapInfo->sock, &msgtype, sizeof(msgtype)) <= 0){
        return DHEAP_ERROR_CONNECTION;
    }

    if (msgtype != MSG_HEAP_SIZE){
        return DHEAP_ERROR_UNEXPECTED_MSG;
    }

    /* Reception de la taille du tas */			
    if (read(heapInfo->sock,&(heapInfo->heapSize),sizeof(heapInfo->heapSize)) <= 0){
        return DHEAP_ERROR_CONNECTION;
    }

#if DEBUG
    printf("HeapSize: %" PRIu64 "\n", heapInfo->heapSize);
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

    return DHEAP_SUCCESS;
}

/**
 * Ferme la connexion et libère toute les données liées au tas
 * @return enum errorCodes
 */
int close_data(){
    int threadStatus;
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
    if (pthread_cancel(*dheap_tid)) != 0){
        perror("pthread_cancel"); 
        exit(EXIT_FAILURE);
    }
    if (pthread_join(*dheap_tid, &threadStatus)) != PTHREAD_CANCELED){
        perror("pthread_join");
        exit(EXIT_FAILURE);
    }
    free(dheap_tid);

    /* Fermeture de la connexion */
    if (close(heapInfo->sock) == -1){
        perror("close");
        exit(EXIT_FAILURE);
    }

    /* On vide le tas */
    free(heapInfo->heapStart);

    /* On vide la structure heapInfo */
    free(heapInfo);
    heapInfo = NULL;

    /* On supprime la hashtable */
    free_hashtable();

    /* On vide le message d'erreur et le numero d'erreur */
    if (dheapErrorMsg != NULL){
        free(dheapErrorMsg);
        dheapErrorMsg = NULL;
    }
    if (dheapErrorNumber != NULL){
        free(dheapErrorNumber);
        dheapErrorNumber = NULL;
    }

    return DHEAP_SUCCESS;
}
