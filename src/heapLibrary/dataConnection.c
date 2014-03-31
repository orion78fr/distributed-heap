#include "distributedHeap.h"

struct heapInfo *heapInfo;
char *dheapErrorMsg;

/**
 * Initialise la connexion avec le serveur central
 * Alloue l'espace pour le tas local
 * Initialisation de la hashtable stockant les accès en cours
 * @return enum errorCodes
 */
int init_data(){
    struct sockaddr_in servaddr;
    /* struct addrinfo hints, *result; */
    int msgtype;

#if DEBUG
    printf("Appel init_data()\n");
#endif 
    
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

#if DEBUG
    printf("HeapSize: %d\n", heapInfo->heapSize);
#endif 

    /* allocation du tas dans la mémoire */
    heapInfo->heapStart = malloc(heapInfo->heapSize);
    if (heapInfo->heapStart == NULL){
        return DHEAP_ERROR_HEAP_ALLOC;
    }

    /* initialisation de la hashtable */
    init_hashtable();

    return DHEAP_SUCCESS;
}

/**
 * Ferme la connexion et libère toute les données liées au tas
 * @return enum errorCodes
 */
int close_data(){
#if DEBUG
    printf("Appel close_data()\n");
#endif 

    /* Fermeture de la connexion */
    if (close(heapInfo->sock) == -1){
        /* TODO: quelle erreur renvoyer? */
    }

    /* On vide le tas */
    free(heapInfo->heapStart);

    /* On vide la structure heapInfo */
    free(heapInfo);

    /* On supprime la hashtable */
    free_hashtable();

    return DHEAP_SUCCESS;
}
