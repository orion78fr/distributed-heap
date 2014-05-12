#include "common.h"


pthread_key_t id;
struct replicationData *rep;
struct replicationAck *ack;
uint16_t numClient=1;
uint8_t numServer=1;
struct clientChain *clients;
struct serverChain *servers;
struct serverChain *backups;

int main(int argc, char *argv[])
{
    struct sockaddr_in sin;     /* Nom de la socket de connexion */
    int sock;                   /* Socket de connexion */
    int sclient;                /* Socket du client */
    int temp = 1;
    int sserver;                /* Socket du serveur */
    uint8_t msgType;
    servers=NULL;
    backups=NULL;
    clients=NULL;

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
        parameters.idle=0;
#if DEBUG
        printf("Port : %d\n", parameters.port);
        printf("Max Clients : %d\n", parameters.maxClients);
        printf("Heap Size : %d\n", parameters.heapSize);
        printf("Hash Size : %d\n", parameters.hashSize);
        printf("backup : %d\n", parameters.backup);
#endif
        init_data();
    }else{
        parameters.backup=1;
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
    if(parameters.backup){
        struct serverChain *newServer;

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


        /* Reception du type de message NEW_SERVER ou NEW_BACKUP */
        if (read(sserver, &msgType, sizeof(msgType)) <= 0){
            return ERROR_SERVER_CONNECTION;
        }

#if DEBUG
        printf("reception du msgType %d\n",msgType);
#endif        

        if(msgType== MSG_HELLO_NEW_BACKUP){
            parameters.idle=1;
            if(rcv_greetings_backup(servers)<=0){
                return ERROR_SERVER_CONNECTION;
            }
        }else if(msgType==MSG_HELLO_NEW_SERVER){
            parameters.idle=0;
            if(rcv_greetings_server(servers)<=0){
                return ERROR_SERVER_CONNECTION;
            }
        }else{
            return ERROR_UNEXPECTED_MSG;
        }
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

        if(init_client_server(sclient, msgType, address)<=0){
            return EXIT_FAILURE;
        }
        
    }

    while (clients != NULL) {
        pthread_join(clients->clientId, 0);
        clients = clients->next;
    }

    
    while (servers != NULL) {
        pthread_join(servers->serverId, 0);
        servers = servers->next;
    }
    

    while (backups != NULL) {
        pthread_join(backups->serverId, 0);
        backups = backups->next;
    }

    shutdown(sock, 2);
    close(sock);
    return EXIT_SUCCESS;
}
