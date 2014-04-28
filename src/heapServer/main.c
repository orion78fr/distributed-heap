#include "common.h"

struct replicationData *rep;

int main(int argc, char *argv[])
{
    struct sockaddr_in sin;     /* Nom de la socket de connexion */
    int sock;                   /* Socket de connexion */
    int sclient;                /* Socket du client */
    int sserver;                /* Socket du serveur */
    uint16_t numClient=0, numServer=1;
    
    rep = malloc(sizeof(struct replicationData));
    rep->data = NULL;
    rep->mutex_server = PTHREAD_MUTEX_INITIALIZER;
    rep->cond_server = PTHREAD_COND_INITIALIZER;



    /* Parsing des arguments */
    if (parse_args(argc, argv)) {
        perror("Wrong args\n");
        exit(EXIT_FAILURE);
    }

#if DEBUG
    printf("Port : %d\n", parameters.port);
    printf("Max Clients : %d\n", parameters.maxClients);
    printf("Heap Size : %d\n", parameters.heapSize);
    printf("Hash Size : %d\n", parameters.hashSize);
    printf("Server Num : %d\n", parameters.serverNum);
    printf("Address : %d\n", parameters.address);
#endif

    init_data();

    /* Creation de la socket */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset((void *) &sin, 0, sizeof(sin));
    sin.sin_port = htons(parameters.port);
    sin.sin_family = AF_INET;

    /* setsockopt(sc, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
     * GTU : Est-ce nécessaire? */




    /* Connexion au server principal */
    if(parameters.address!=""){
        struct serverChain *newServer;
        uint8_t msgtype = MSG_HELLO_NEW_SERVER;

        sin.sin_addr.s_addr=inet_addr(parameters.address);

#if DEBUG
        printf("Connecting to Main Server...\n");
#endif
        if(connect(sserver, (struct sockaddr *)&sin, sizeof(sin)) == -1){
            return ERROR_SERVER_CONNECTION;
        }



        /* Ajout du server dans la chaîne de socket (ajout au début pour
         * éviter le parcours) */
        newServer = malloc(sizeof(struct serverChain));
        newServer->sock=sserver;
        newServer->next=servers;
        newServer->serverAddress=parameters.address;
        servers=newServer;

        /* Ajout du main à la poll_list */
        poll_list=NULL;
        poll_list=malloc(sizeof(struct pollfd));
        poll_list[0].fd = newServer->sock;
        poll_list[0].events = POLLHUP | POLLIN | POLLNVAL;

        /*Mis à jour du tas, des clients, des servers */
        msgtype = MSG_HELLO_NEW_SERVER;
        if (write(sserver, &msgtype, sizeof(msgtype)) <= 0){
            return ERROR_SERVER_CONNECTION;
        }

        if (write(sserver, , sizeof(msgtype)) <= 0){
            return ERROR_SERVER_CONNECTION;
        }

        /* Reception du type de message (MSG_HELLO_NEW_SERVER) */          
        if (read(sserver, &msgtype, sizeof(msgtype)) <= 0){
            return ERROR_SERVER_CONNECTION;
        }

        if (msgtype != MSG_HELLO_NEW_SERVER){
            return ERROR_UNEXPECTED_MSG;
        }

        /* Reception de l'id du serveur auquel on se connecte */
        if (read(sserver, &(newServer->serverId), sizeof(newServer->serverId)) <= 0){
            return ERROR_BACKUP_INIT;
        }

        /* Réception de notre id server */
        if (read(sserver, &(parameters.serverNum), sizeof(parameters.serverNum)) <= 0){
            return ERROR_BACKUP_INIT;
        }

        /* demande de mis à jour du tas et des locks et du nombre de serveur */
        msgtype = MSG_TOTAL_REPLICATION;
        if (write(sserver, &msgtype, sizeof(msgtype)) <= 0){
            return ERROR_SERVER_CONNECTION;
        }

        rcv_total_replication(sserver);

    }else{
        servers=NULL;
    }

    /* Création d'un thread pour traiter les requêtes servers */
    pthread_create((pthread_t *) & (parameters.serverNum), NULL,
                    serverThread, (void *) servers);

    sin.sin_addr.s_addr = htonl(INADDR_ANY);

    /* Lien et écoute de la socket */
    if (bind(sock, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    listen(sock, parameters.maxClients);

    for (;;) {
        struct clientChain *newClient;
        struct serverChain *newServer;
        char address[14];
        struct sockaddr addr;

#if DEBUG
        printf("Waiting for clients...\n");
#endif

        /* On accepte la connexion */
        if ((sclient = accept(sock, (struct sockaddr*)&addr, sizeof(addr))) == -1) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        address = inet_ntoa (addr.sin_addr);

#if DEBUG
        printf("New Client...\n");
        /* GTU : Ajouter des getname etc... pour le debug (savoir
         * qui se connecte) */
#endif

        if (clientsConnected > parameters.maxClients) {
            /* ERREUR */
            send_error(sclient, ERROR_SERVER_FULL);
            shutdown(sclient, 2);
            close(sclient);
            continue;
        }

        int test=do_inquire(sclient, &address);

        /* client detected */
        if(test=1{
            /* Ajout d'un client */
        }else if(test==2){ 
            /* server detected */  
        }else{ /* error detected */

        }

        
    }

    /* GTU : Et comment on sort de là? Signaux puis envoi d'un END
     * a tout les clients */

    while (clients != NULL) {
        pthread_join(clients->clientId, 0);
        clients = clients->next;
    }

    /*
    while (servers != NULL) {
        pthread_join(servers->serverId, 0);
        servers = servers->next;
    }
    */

    pthread_join(serverId,0);

    shutdown(sock, 2);
    close(sock);
    return EXIT_SUCCESS;
}
