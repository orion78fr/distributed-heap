#include "common.h"


pthread_key_t id;
struct replicationData *rep;
struct replicationAck *ack;
uint16_t numClient=1;
uint8_t numServer=1;
struct serverChain *servers;

int main(int argc, char *argv[])
{
    struct sockaddr_in sin;     /* Nom de la socket de connexion */
    int sock;                   /* Socket de connexion */
    int sclient;                /* Socket du client */
    int temp = 1;
    int sserver;                /* Socket du serveur */
    uint8_t msgType;
    servers=NULL;

    pthread_key_create(&id, NULL);


    rep = malloc(sizeof(struct replicationData));
    rep->data = NULL;
    pthread_mutex_init(&rep->mutex_server, NULL);
    pthread_cond_init(&rep->cond_server, NULL);

    ack = malloc(sizeof(struct replicationAck));
    pthread_mutex_init(&ack->mutex_server, NULL);
    pthread_cond_init(&ack->cond_server, NULL);
    ack->modification=0;

    /* Parsing des arguments */
    if (parse_args(argc, argv)) {
        perror("Wrong args\n");
        exit(EXIT_FAILURE);
    }



    if(strcmp(parameters.mainAddress,"")==0){
        parameters.backup=0;
#if DEBUG
    printf("Port : %d\n", parameters.port);
    printf("Max Clients : %d\n", parameters.maxClients);
    printf("Heap Size : %d\n", parameters.heapSize);
    printf("Hash Size : %d\n", parameters.hashSize);
    printf("backup : %d\n", parameters.backup);
#endif
        init_data();
    }

    /* Creation de la socket */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset((void *) &sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &temp, sizeof(int));

    /* Connexion au server principal */
    if(strcmp(parameters.mainAddress,"")!=0){
        struct serverChain *newServer;
        parameters.backup=1;

        /* Creation de la socket */
        if ((sserver = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("socket");
            exit(EXIT_FAILURE);
        }

        setsockopt(sserver, SOL_SOCKET, SO_REUSEADDR, &temp, sizeof(int));

        sin.sin_port = htons(parameters.mainPort);
        sin.sin_addr.s_addr=inet_addr(parameters.mainAddress);

#if DEBUG
        printf("Connecting to Main Server...\n");
        printf("@main: %s...\n",parameters.mainAddress);
        printf("@main port: %d...\n",parameters.mainPort);
        printf("@my port: %d...\n",parameters.port);
        printf("backup : %d\n", parameters.backup);
#endif
        if(connect(sserver, (struct sockaddr *)&sin, sizeof(sin)) == -1){
            return ERROR_SERVER_CONNECTION;
        }

#if DEBUG
        printf("socket du main sur le backup %d\n",sserver);
#endif

        if (read(sserver, &msgType, sizeof(msgType)) <= 0){
            return ERROR_SERVER_CONNECTION;
        }

        if(msgType!=MSG_HELLO){
            return ERROR_SERVER_CONNECTION;
        }

        /* Ajout du server dans la chaîne de socket (ajout au début pour
         * éviter le parcours) */
        newServer = malloc(sizeof(struct serverChain));
        newServer->backup = 1;
        newServer->sock=sserver;
        newServer->next=servers;
        newServer->serverAddress = malloc(strlen(parameters.mainAddress));
        strcpy(newServer->serverAddress, parameters.mainAddress);
        servers=newServer;

        msgType=MSG_HELLO_NEW_SERVER;
        if (write(sserver, &msgType, sizeof(msgType)) <= 0){
            return ERROR_SERVER_CONNECTION;
        }

#if DEBUG
        printf("envoi du type  %d\n",msgType);
#endif

        if (write(sserver, &parameters.port, sizeof(parameters.port)) <= 0){
            return ERROR_SERVER_CONNECTION;
        }

#if DEBUG
        printf("envoi du port %d\n",parameters.port);
#endif


        /* Reception du type de message (MSG_HELLO_NEW_SERVER) */
        if (read(sserver, &msgType, sizeof(msgType)) <= 0){
            return ERROR_SERVER_CONNECTION;
        }

#if DEBUG
        printf("reception du msgType %d\n",msgType);
#endif        



        if (msgType != MSG_HELLO_NEW_SERVER){
            return ERROR_UNEXPECTED_MSG;
        }


        /* Reception de l'id du serveur auquel on se connecte */
        if (read(sserver, &(newServer->serverId), sizeof(newServer->serverId)) <= 0){
            return ERROR_BACKUP_INIT;
        }

#if DEBUG
        printf("reception id du main %d\n",newServer->serverId);
#endif


        /* Réception de notre id server */
        if (read(sserver, &(parameters.serverNum), sizeof(parameters.serverNum)) <= 0){
            return ERROR_BACKUP_INIT;
        }

#if DEBUG
        printf("reception notre id %d\n",parameters.serverNum);
#endif

        /* Réception de la taille de la hashSize */
        if (read(sserver, &(parameters.hashSize), sizeof(parameters.hashSize)) <= 0){
            return ERROR_BACKUP_INIT;
        }

#if DEBUG
        printf("reception taille hashSize %d\n",parameters.hashSize);
#endif

        /* Réception de la taille heapSize */
        if (read(sserver, &(parameters.heapSize), sizeof(parameters.heapSize)) <= 0){
            return ERROR_BACKUP_INIT;
        }

#if DEBUG
        printf("reception taille heapSize %d\n",parameters.heapSize);
#endif

        init_data();

        
        if(send_data(sserver, MSG_TOTAL_REPLICATION, 0)<0){
            return EXIT_FAILURE;
        }

#if DEBUG
        printf("demande replication totale \n");
#endif

        if(rcv_total_replication(sserver)<=0){
            return EXIT_FAILURE;
        }

        /* Création d'un thread pour traiter les requêtes servers */
        pthread_create((pthread_t *) & (servers->threadId), NULL,
                        serverThread, (void *) servers);

    }

#if DEBUG
        printf("lien et écoute socket\n");
#endif

    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(parameters.port);
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
        uint16_t port;

#if DEBUG
        printf("Waiting for clients...\n");
#endif

        /* On accepte la connexion */
        if ((sclient = accept(sock, (struct sockaddr *)&addr, &len)) == -1) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

#if DEBUG
        printf("connection detecte...\n");
#endif
        msgType = MSG_HELLO;
        if (write(sclient, &msgType, sizeof(msgType)) <= 0){
                return EXIT_FAILURE;
        }

        address = inet_ntoa(addr.sin_addr); // ?? le warning

#if DEBUG
        printf("New Client... %d\n",sclient);
        printf("@... %s\n",address);
        printf("@... %d\n",strlen(address));
#endif

        if (clientsConnected > parameters.maxClients) {
            /* ERREUR */
            send_error(sclient, ERROR_SERVER_FULL);
            shutdown(sclient, 2);
            close(sclient);
            continue;
        }

#if DEBUG
        printf("on a de la place et on attend un msg hello new...\n");
#endif



        if (read(sclient,  &msgType, sizeof(msgType)) <= 0) {       /* Msg type */
            return EXIT_FAILURE;
        }

#if DEBUG
        printf("reception du msgType %d\n",msgType);
#endif

        if(msgType == MSG_HELLO_NEW){
#if DEBUG        
            printf("new client\n");
#endif            
            /* Ajout du client dans la chaîne de socket (ajout au début pour
             * éviter le parcours) */
            newClient = malloc(sizeof(struct clientChain));
            newClient->sock = sclient;
            newClient->next = clients;
            newClient->clientId = numClient;
            newClient->newC = 1;
            numClient++;
            pthread_mutex_init(&newClient->mutex_sock, NULL);
            clients = newClient;

            /* Création d'un thread pour traiter les requêtes */
            pthread_create((pthread_t *) & (newClient->threadId), NULL,
                           clientThread, (void *) newClient);
            clientsConnected++;
        }else if(msgType == MSG_HELLO_NOT_NEW){

#if DEBUG        
            printf("new client sur le backup\n");
#endif            
            /* Ajout du client dans la chaîne de socket (ajout au début pour
             * éviter le parcours) */
            newClient = malloc(sizeof(struct clientChain));
            newClient->sock = sclient;
            newClient->next = clients;
            newClient->newC = 0;

            if (read(sclient,  &newClient->clientId, sizeof(newClient->clientId)) <= 0) {
                return EXIT_FAILURE;
            }
#if DEBUG
            printf("reception de l'id client %d\n",newClient->clientId);
#endif

            if (write(sclient, &msgType, sizeof(msgType)) <= 0){
                return EXIT_FAILURE;
            }

            numClient++;
            pthread_mutex_init(&newClient->mutex_sock, NULL);
            clients = newClient;

            /* Création d'un thread pour traiter les requêtes */
            pthread_create((pthread_t *) & (newClient->threadId), NULL,
                           clientThread, (void *) newClient);
            clientsConnected++;

        }else if(msgType == MSG_HELLO_NEW_SERVER){
#if DEBUG            
            printf("new server\n");
#endif

            newServer = malloc(sizeof(struct serverChain));

            if (read(sclient,  &newServer->serverPort, sizeof(newServer->serverPort)) <= 0) {
                return EXIT_FAILURE;
            }
#if DEBUG
            printf("reception du port %d\n",newServer->serverPort);
#endif
            newServer->backup = 0;
            newServer->sock = sclient;

#if DEBUG
            printf("newServer->sock %d\n",newServer->sock);
#endif

            newServer->next = servers;
            newServer->serverAddress = malloc(strlen(address));
            strcpy(newServer->serverAddress, address);
            newServer->serverId = numServer;
            numServer++;
            servers = newServer;

            serversConnected++;

#if DEBUG
            printf("envoi du msgType\n");
            printf("envoi du mainId\n");
            printf("envoi du backupId\n");
            printf("envoi du hashSize\n");
            printf("envoi du heapSize\n");
#endif
            
            if(send_data(sclient, MSG_HELLO_NEW_SERVER, 4,
                            (DS){sizeof(parameters.serverNum), &(parameters.serverNum)},
                            (DS){sizeof(newServer->serverId), &(newServer->serverId)},
                            (DS){sizeof(parameters.hashSize), &(parameters.hashSize)},
                            (DS){sizeof(parameters.heapSize), &(parameters.heapSize)})<0){
                return EXIT_FAILURE;
            }

            if (read(sclient,  &msgType, sizeof(msgType)) <= 0) {       
                return EXIT_FAILURE;
            }

#if DEBUG
            printf("reception du msgType %d\n",msgType);
#endif


            if(msgType==MSG_TOTAL_REPLICATION){
                if(snd_total_replication(sclient)<=0){
                    return EXIT_FAILURE;
                }
            }


            if(read(sclient, &msgType, sizeof(msgType)) <= 0){
                return EXIT_FAILURE;
            }

#if DEBUG
    printf("[msgType: %d]\n",msgType);
#endif

            if(msgType!=MSG_ACK){
                return EXIT_FAILURE;
            }

            if(snd_server_to_clients(servers->serverAddress, servers->serverPort, servers->serverId) <= 0){
                return EXIT_FAILURE;
            }

            /* Création d'un thread pour traiter les requêtes servers */
            pthread_create((pthread_t *) & (servers->threadId), NULL,
                        serverThread, (void *) servers);
        }
    }

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

    if(servers!=NULL){
        pthread_join(servers->serverId,0);
    }

    shutdown(sock, 2);
    close(sock);
    return EXIT_SUCCESS;
}
