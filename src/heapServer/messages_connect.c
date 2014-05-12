#include "common.h"

int do_greetings_backup(struct serverChain *backup, struct serverChain *server){
    uint8_t msgType;
    int size=0;
    int sock = backup->sock;
    struct serverChain *temp;

#if DEBUG
    printf("envoi du msgType\n");
    printf("envoi du mainId\n");
    printf("envoi du backupId\n");
    printf("envoi du hashSize\n");
    printf("envoi du heapSize\n");
#endif

    if(send_data(sock, MSG_HELLO_NEW_BACKUP, 4,
        (DS){sizeof(parameters.serverNum), &(parameters.serverNum)},
        (DS){sizeof(backup->serverId), &(backup->serverId)},
        (DS){sizeof(parameters.hashSize), &(parameters.hashSize)},
        (DS){sizeof(parameters.heapSize), &(parameters.heapSize)})<0){
        return EXIT_FAILURE;
    }   

    temp=servers;
    while(temp->next!=NULL){
        size++;
        temp=temp->next;
    }
    if (write(sock, &size, sizeof(size)) <= 0){
        return EXIT_FAILURE;
    }
    if(size!=0){
        temp=servers;
        while(temp->next!=NULL){
            uint8_t taille = strlen(temp->serverAddress);
            if(send_data_rep(sock, 4,
                (DS){sizeof(temp->serverId), &(temp->serverId)},
                (DS){sizeof(taille), &(taille)},
                (DS){taille, temp->serverAddress},
                (DS){sizeof(temp->serverPort), &(temp->serverPort)})<0){
                return EXIT_FAILURE;
            }
        temp=temp->next;
        }
    }  

    if (read_rep(sock,  &msgType, sizeof(msgType)) <= 0) {       
        return EXIT_FAILURE;
    }
    if(msgType!=MSG_ACK){
        return 0;
    }
    return 1;
}

int do_greetings_server(struct serverChain *server){
    uint8_t msgType;
    int sock = server->sock; 

#if DEBUG
    printf("envoi du msgType\n");
    printf("envoi du mainId\n");
    printf("envoi du backupId\n");
    printf("envoi du hashSize\n");
    printf("envoi du heapSize\n");
#endif

    if(send_data(sock, MSG_HELLO_NEW_SERVER, 4,
            (DS){sizeof(parameters.serverNum), &(parameters.serverNum)},
            (DS){sizeof(server->serverId), &(server->serverId)},
            (DS){sizeof(parameters.hashSize), &(parameters.hashSize)},
            (DS){sizeof(parameters.heapSize), &(parameters.heapSize)})<0){
            return EXIT_FAILURE;
        }

    if (read_rep(sock,  &msgType, sizeof(msgType)) <= 0) {       
        return EXIT_FAILURE;
    }

#if DEBUG
    printf("reception du msgType %d\n",msgType);
#endif


    if(msgType==MSG_TOTAL_REPLICATION){
        if(snd_total_replication(sock)<=0){
            return EXIT_FAILURE;
        }
    }


    if(read_rep(sock, &msgType, sizeof(msgType)) <= 0){
        return EXIT_FAILURE;
    }

#if DEBUG
    printf("[msgType: %d]\n",msgType);
#endif

    if(msgType!=MSG_ACK){
        return EXIT_FAILURE;
    }

    if(snd_server_to_clients(server->serverAddress, server->serverPort, server->serverId) <= 0){
        return EXIT_FAILURE;
    }
    return 1;
}

int rcv_greetings_backup(struct serverChain *server){
    uint8_t msgType;
    int sock = server->sock;
    int size=0;
    struct serverChain *newServer, *prev=NULL;;
    uint8_t taille;

    /* Reception de l'id du serveur auquel on se connecte */
    if (read_rep(sock, &(server->serverId), sizeof(server->serverId)) <= 0){
        return ERROR_BACKUP_INIT;
    }

#if DEBUG
    printf("reception id du main %d\n",server->serverId);
#endif


    /* Réception de notre id server */
    if (read_rep(sock, &(parameters.serverNum), sizeof(parameters.serverNum)) <= 0){
        return ERROR_BACKUP_INIT;
    }

#if DEBUG
    printf("reception notre id %d\n",parameters.serverNum);
#endif

    /* Réception de la taille de la hashSize */
    if (read_rep(sock, &(parameters.hashSize), sizeof(parameters.hashSize)) <= 0){
        return ERROR_BACKUP_INIT;
    }

#if DEBUG
    printf("reception taille hashSize %d\n",parameters.hashSize);
#endif

    /* Réception de la taille heapSize */
    if (read_rep(sock, &(parameters.heapSize), sizeof(parameters.heapSize)) <= 0){
        return ERROR_BACKUP_INIT;
    }

#if DEBUG
    printf("reception taille heapSize %d\n",parameters.heapSize);
#endif

    init_data();

    /* Réception nbr server secondaire */
    if (read_rep(sock, &(size), sizeof(size)) <= 0){
        return ERROR_BACKUP_INIT;
    }

    if(size!=0){
        struct sockaddr_in sin;
        prev=server;
        for(int i=0; i<size; i++){
            newServer = malloc(sizeof(struct serverChain));
            newServer->backup = 1;
            prev->next = newServer;
            
            if (read_rep(sock, &(newServer->serverId), sizeof(newServer->serverId)) <= 0){
                return ERROR_BACKUP_INIT;
            }

            if (read_rep(sock, &(taille), sizeof(taille)) <= 0){
                return ERROR_BACKUP_INIT;
            }

            newServer->serverAddress = malloc(taille);
            if (read_rep(sock, newServer->serverAddress, taille) <= 0){
                return ERROR_BACKUP_INIT;
            }

            if (read_rep(sock, &(newServer->serverPort), sizeof(newServer->serverPort)) <= 0){
                return ERROR_BACKUP_INIT;
            }

            /* Creation de la socket */
            if ((newServer->sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                perror("socket");
                exit(EXIT_FAILURE);
            }
            sin.sin_family = AF_INET;
            sin.sin_port = htons(newServer->port);
            sin.sin_addr.s_addr=inet_addr(newServer->serverAddress);

            if(connect(newServer->sock, (struct sockaddr *)&sin, sizeof(sin)) == -1){
                return ERROR_SERVER_CONNECTION;
            }

            if (read(newServer->sock, &msgType, sizeof(msgType)) <= 0){
                return ERROR_SERVER_CONNECTION;
            }

            if(msgType!=MSG_HELLO){
                return ERROR_SERVER_CONNECTION;
            }

            msgType=MSG_HELLO_NEW_BACKUP;
            if(send_data(sock, msgType, 4,
                (DS){sizeof(parameters.serverNum), &(parameters.serverNum)},
                (DS){sizeof(parameters.port), &(parameters.port)})<0){
                return EXIT_FAILURE;
            }

        pthread_create((pthread_t *) & (newServer->threadId), NULL,
            serverThread, (void *) newServer);

        prev = newServer;
        }
    }

    pthread_create((pthread_t *) & (server->threadId), NULL,
        serverThread, (void *) server);

    if(send_data(sock, MSG_ACK, 0)<0){
        return EXIT_FAILURE;
    }
    return 1;
}

int rcv_greetings_server(struct serverChain *server){
    uint8_t msgType;
    int sock = server->sock;


    /* Reception de l'id du serveur auquel on se connecte */
    if (read_rep(sock, &(server->serverId), sizeof(server->serverId)) <= 0){
        return ERROR_BACKUP_INIT;
    }

#if DEBUG
    printf("reception id du main %d\n",server->serverId);
#endif


    /* Réception de notre id server */
    if (read_rep(sock, &(parameters.serverNum), sizeof(parameters.serverNum)) <= 0){
        return ERROR_BACKUP_INIT;
    }

#if DEBUG
    printf("reception notre id %d\n",parameters.serverNum);
#endif

    /* Réception de la taille de la hashSize */
    if (read_rep(sock, &(parameters.hashSize), sizeof(parameters.hashSize)) <= 0){
        return ERROR_BACKUP_INIT;
    }

#if DEBUG
    printf("reception taille hashSize %d\n",parameters.hashSize);
#endif

    /* Réception de la taille heapSize */
    if (read_rep(sock, &(parameters.heapSize), sizeof(parameters.heapSize)) <= 0){
        return ERROR_BACKUP_INIT;
    }

#if DEBUG
    printf("reception taille heapSize %d\n",parameters.heapSize);
#endif

    init_data();

    if(send_data(sock, MSG_TOTAL_REPLICATION, 0)<0){
        return EXIT_FAILURE;
    }

#if DEBUG
    printf("demande replication totale \n");
#endif

    if(rcv_total_replication(sock)<=0){
        return EXIT_FAILURE;
    }

    /* Création d'un thread pour traiter les requêtes servers */
    pthread_create((pthread_t *) & (server->threadId), NULL,
        serverThread, (void *) server);
    return 1;
}

int init_client_server(int sock, uint8_t msgType, char *address){
    struct clientChain *newClient;
    struct serverChain *newServer;

    if(msgType == MSG_HELLO_NEW){
#if DEBUG        
        printf("new client\n");
#endif            
        /* Ajout du client dans la chaîne de socket (ajout au début pour
         * éviter le parcours) */
        newClient = malloc(sizeof(struct clientChain));
        newClient->sock = sock;
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
        newClient->sock = sock;
        newClient->next = clients;
        newClient->newC = 0;

        if (read(sock,  &newClient->clientId, sizeof(newClient->clientId)) <= 0) {
            return EXIT_FAILURE;
        }
#if DEBUG
        printf("reception de l'id client %d\n",newClient->clientId);
#endif

        if (write(sock, &msgType, sizeof(msgType)) <= 0){
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
        struct serverChain *temp;
#if DEBUG            
        printf("new server\n");
#endif

        newServer = malloc(sizeof(struct serverChain));

        if (read(sock,  &newServer->serverPort, sizeof(newServer->serverPort)) <= 0) {
            return EXIT_FAILURE;
        }
#if DEBUG
        printf("reception du port %d\n",newServer->serverPort);
#endif
        newServer->backup = 0;
        newServer->sock = sock;

#if DEBUG
        printf("newServer->sock %d\n",newServer->sock);
#endif


        newServer->serverAddress = malloc(strlen(address));
        strcpy(newServer->serverAddress, address);
        newServer->serverId = numServer;
        numServer++;
        if(servers!=NULL){
            while(temp->next!=NULL){
                temp=temp->next;
            }
            temp->next = newServer;

            if(do_greetings_backup(newServer, servers)<=0){
                return EXIT_FAILURE;
            }
            continue;
        }else{
            newServer->next = servers;
            servers = newServer;
            serversConnected++;
        }

        if(do_greetings_server(servers)<=0){
            return EXIT_FAILURE;
        }

            /* Création d'un thread pour traiter les requêtes servers */
        pthread_create((pthread_t *) & (servers->threadId), NULL,
                        serverThread, (void *) servers);

    }else if(msgType == MSG_HELLO_NEW_BACKUP){
        struct serverChain *temp=backups;
#if DEBUG            
        printf("new backup\n");
#endif

        newServer = malloc(sizeof(struct serverChain));

        if (read(sock,  &newServer->serverId, sizeof(newServer->serverId)) <= 0) {
            return EXIT_FAILURE;
        }

#if DEBUG
        printf("reception de l'id %d\n",newServer->serverId);
#endif

        if (read(sock,  &newServer->serverPort, sizeof(newServer->serverPort)) <= 0) {
            return EXIT_FAILURE;
        }
#if DEBUG
        printf("reception du port %d\n",newServer->serverPort);
#endif
        newServer->backup = 1;
        newServer->sock = sock;

#if DEBUG
        printf("newServer->sock %d\n",newServer->sock);
#endif


        newServer->serverAddress = malloc(strlen(address));
        strcpy(newServer->serverAddress, address);

        while(temp->next!=NULL){
            temp=temp->next;
        }
        temp->next = newServer;
        serversConnected++;

            /* Création d'un thread pour traiter les requêtes servers */
        pthread_create((pthread_t *) & (servers->threadId), NULL,
                        serverThread, (void *) servers);
    }
    return 1;
}