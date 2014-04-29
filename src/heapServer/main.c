#include "common.h"


pthread_key_t id;
struct replicationData *rep;
struct replicationAck *ack;
uint16_t numClient=1;
uint8_t numServer=1;

int main(int argc, char *argv[])
{
    struct sockaddr_in sin;     /* Nom de la socket de connexion */
    int sock;                   /* Socket de connexion */
    int sclient;                /* Socket du client */
    int temp = 1;
    int sserver;                /* Socket du serveur */
    uint8_t msgType;

    pthread_key_create(&id, NULL);

    rep = malloc(sizeof(struct replicationData));
    rep->data = NULL;
    pthread_mutex_init(&rep->mutex_server, NULL);
    pthread_cond_init(&rep->cond_server, NULL);

    ack = malloc(sizeof(struct replicationAck));
    pthread_mutex_init(&ack->mutex_server, NULL);
    pthread_cond_init(&ack->cond_server, NULL);

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
    printf("Address : %s\n", parameters.mainAddress);
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

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &temp, sizeof(int));
     /* GTU : Est-ce nécessaire? */

    /* Connexion au server principal */
    if(strcmp(parameters.mainAddress,"")!=0){
        struct serverChain *newServer;
        uint8_t msgtype = MSG_HELLO_NEW_SERVER;

        sin.sin_addr.s_addr=inet_addr(parameters.mainAddress);

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
        strcpy(newServer->serverAddress, parameters.mainAddress);
        servers=newServer;


        /*Mis à jour du tas, des clients, des servers */
        msgtype = MSG_HELLO_NEW_SERVER;
        if (write(sserver, &msgtype, sizeof(msgtype)) <= 0){
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
        char *address;
        struct sockaddr_in addr;
        socklen_t len = sizeof(struct sockaddr_in);

#if DEBUG
        printf("Waiting for clients...\n");
#endif

        /* On accepte la connexion */
        if ((sclient = accept(sock, (struct sockaddr *)&addr, &len)) == -1) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        address = inet_ntoa(addr.sin_addr); // ?? le warning

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



        if (read(sock, (void *) &msgType, sizeof(msgType)) <= 0) {       /* Msg type */
            return EXIT_FAILURE;
        }

        if(msgType == MSG_HELLO_NEW_CLIENT){
            /* Ajout du client dans la chaîne de socket (ajout au début pour
             * éviter le parcours) */
            newClient = malloc(sizeof(struct clientChain));
            newClient->sock = sock;
            newClient->next = clients;
            newClient->clientId = numClient;
            numClient++;
            clients = newClient;

            /* Création d'un thread pour traiter les requêtes */
            pthread_create((pthread_t *) & (newClient->threadId), NULL,
                           clientThread, (void *) newClient);
            clientsConnected++;
        }else if(msgType == MSG_HELLO_NEW_SERVER){

            newServer = malloc(sizeof(struct serverChain));
            newServer->sock = sock;
            newServer->next = servers;
            newServer->serverAddress = address;
            newServer->serverId = numServer;
            numServer++;
            servers = newServer;

            serversConnected++;

            if(send_data(sock, MSG_HELLO_NEW_SERVER, 3,
                            (DS){sizeof(parameters.serverNum), &(parameters.serverNum)},
                            (DS){sizeof(newServer->serverId), &(newServer->serverId)})<0){
                return EXIT_FAILURE;
            }
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

    pthread_join(servers->serverId,0);

    shutdown(sock, 2);
    close(sock);
    return EXIT_SUCCESS;
}
