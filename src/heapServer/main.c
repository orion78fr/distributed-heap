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
    struct serverChain *servers=NULL;

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
    //sin.sin_port = htons(parameters.port);
    sin.sin_family = AF_INET;

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &temp, sizeof(int));
     /* GTU : Est-ce nécessaire? */

    /* Connexion au server principal */
    if(strcmp(parameters.mainAddress,"")!=0){
        struct serverChain *newServer;
        //uint8_t msgtype = MSG_HELLO_NEW_SERVER;

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
/*
        if (write(sserver, &msgtype, sizeof(msgtype)) <= 0){
            return ERROR_SERVER_CONNECTION;
        }

#if DEBUG
        printf("1 envoi du type MSG_HELLO_NEW_SERVER %d\n",msgtype);
#endif
*/
        msgType=MSG_HELLO_NEW_SERVER;
        if (write(sserver, &msgType, sizeof(msgType)) <= 0){
            return ERROR_SERVER_CONNECTION;
        }

#if DEBUG
        printf("1 envoi du type  %d\n",msgType);
        printf("size  %d\n",sizeof(msgType));
#endif

        if (write(sserver, &parameters.port, sizeof(parameters.port)) <= 0){
            return ERROR_SERVER_CONNECTION;
        }

#if DEBUG
        printf("2 envoi du port %d\n",parameters.port);
        printf("size  %d\n",sizeof(parameters.port));
#endif



        /* Reception du type de message (MSG_HELLO_NEW_SERVER) */
        /*
        if (read(sserver, &msgtype, sizeof(msgtype)) <= 0){
            return ERROR_SERVER_CONNECTION;
        }

#if DEBUG
        printf("1 reception du msgType %d\n",msgtype);
        printf("sizeof msgType %d\n",sizeof(msgtype));
#endif
        */

        /* Reception du type de message (MSG_HELLO_NEW_SERVER) */
        if (read(sserver, &msgType, sizeof(msgType)) <= 0){
            return ERROR_SERVER_CONNECTION;
        }

#if DEBUG
        printf("1 reception du msgType %d\n",msgType);
        printf("sizeof msgType %d\n",sizeof(msgType));
#endif        



        if (msgType != MSG_HELLO_NEW_SERVER){
            return ERROR_UNEXPECTED_MSG;
        }


        /* Reception de l'id du serveur auquel on se connecte */
        if (read(sserver, &(newServer->serverId), sizeof(newServer->serverId)) <= 0){
            return ERROR_BACKUP_INIT;
        }

#if DEBUG
        printf("2 reception id du main %d\n",newServer->serverId);
        printf("sizeof id du main %d\n",sizeof(newServer->serverId));
#endif


        /* Réception de notre id server */
        if (read(sserver, &(parameters.serverNum), sizeof(parameters.serverNum)) <= 0){
            return ERROR_BACKUP_INIT;
        }

#if DEBUG
        printf("3 reception notre id %d\n",parameters.serverNum);
        printf("sizeof notre id %d\n",sizeof(parameters.serverNum));
#endif
        
        if(send_data(sserver, MSG_TOTAL_REPLICATION, 0)<0){
            return EXIT_FAILURE;
        }

#if DEBUG
        printf("3 demande replication totale \n");
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

        //address=malloc(14);
        address = inet_ntoa(addr.sin_addr); // ?? le warning
        //port = ntohs(addr.sin_port);
#if DEBUG
        printf("New Client... %d\n",sclient);
        printf("@... %s\n",address);
        printf("@... %d\n",strlen(address));
        //printf("port... %d\n",port);
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
        printf("1 reception du msgType %d\n",msgType);
        printf("size %d\n",sizeof(msgType));
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
            printf("2 reception du port %d\n",newServer->serverPort);
            printf("size %d\n",sizeof(newServer->serverPort));
            printf("port backup %d\n",newServer->serverPort);
#endif
            newServer->backup = 0;
            newServer->sock = sclient;

#if DEBUG
        printf("sclient %d\n",sclient);
        printf("newServer->sock %d\n",newServer->sock);
#endif

            newServer->next = servers;
            newServer->serverAddress = malloc(strlen(address));
            strcpy(newServer->serverAddress, address);
            newServer->serverId = numServer;
            numServer++;
            servers = newServer;

            serversConnected++;
/*
#if DEBUG
            printf("1 envoi du msgType\n");
            printf("2 envoi du mainId\n");
            printf("3 envoi du backupId\n");
#endif
            
            if(send_data(newServer->sock, MSG_HELLO_NEW_SERVER, 2,
                            (DS){sizeof(parameters.serverNum), &(parameters.serverNum)},
                            (DS){sizeof(newServer->serverId), &(newServer->serverId)})<0){
                return EXIT_FAILURE;
            }*/
#if DEBUG
        printf("1 envoi du msgType %d\n",msgType);
        printf("size %d\n",sizeof(msgType));
#endif
            if (write(sclient, &msgType, sizeof(msgType)) <= 0){
                return EXIT_FAILURE;
            }

#if DEBUG
        printf("2 envoi du servNum %d\n",parameters.serverNum);
        printf("size %d\n",sizeof(parameters.serverNum));
#endif
            if (write(sclient, &parameters.serverNum, sizeof(parameters.serverNum)) <= 0){
                return EXIT_FAILURE;
            }

#if DEBUG
        printf("3 envoi du serverId %d\n",newServer->serverId);
        printf("size %d\n",sizeof(newServer->serverId));
#endif
            if (write(sclient, &newServer->serverId, sizeof(newServer->serverId)) <= 0){
                return EXIT_FAILURE;
            }

            msgType = MSG_TOTAL_REPLICATION;


            if (read(sclient,  &msgType, sizeof(msgType)) <= 0) {       
                return EXIT_FAILURE;
            }

#if DEBUG
            printf("3 reception du msgType %d\n",msgType);
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
    printf("size %d\n",sizeof(msgType));
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

    if(servers!=NULL){
        pthread_join(servers->serverId,0);
    }

    shutdown(sock, 2);
    close(sock);
    return EXIT_SUCCESS;
}
