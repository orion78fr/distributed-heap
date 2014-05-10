#include "common.h"

int snd_total_replication(int sock){
    struct  serverChain *servTemp = servers;
    struct heapData *data=NULL;
    uint8_t tailleNom, msgType=MSG_REP_STOP;
#if DEBUG
    printf("debut snd total rep\n");
#endif
    pthread_mutex_lock(&hashTableMutex);
    int j=0,i=0;
    while(data==NULL && j<parameters.hashSize){
        data = hashTable[j];
        j++;
    }
    if(data!=NULL){
        msgType=MSG_REP_CONTINUE;
    }

    while(msgType==MSG_REP_CONTINUE){
        struct clientChainRead *readTemp;
        struct clientChainWrite *writeTemp;

#if DEBUG
    printf("data %s\n",data->name);
#endif

        tailleNom = strlen(data->name);
        if(send_data(servTemp->sock, msgType, 5,
                            (DS){sizeof(tailleNom), &tailleNom},
                            (DS){tailleNom, data->name},
                            (DS){sizeof(data->offset), &data->offset},
                            (DS){sizeof(data->size), &data->size},
                            (DS){data->size, theHeap + data->offset})<0){
            goto disconnect;
        }
        
        if(write(sock, &data->readAccessSize, sizeof(data->readAccessSize)) <= 0){
                goto disconnect;
        }
        readTemp = data->readAccess;
        for(i=0; i<data->readAccessSize; i++){
            if(write(sock, &readTemp->clientId, sizeof(readTemp->clientId)) <= 0){
                goto disconnect;
            }
            readTemp = readTemp->next;
        }

        if(write(sock, &data->readWaitSize, sizeof(data->readWaitSize)) <= 0){
                goto disconnect;
        }
        readTemp = data->readWait;
        for(i=0; i<data->readWaitSize; i++){
            if(write(sock, &readTemp->clientId, sizeof(readTemp->clientId)) <= 0){
                goto disconnect;
            }
            readTemp = readTemp->next;
        }

        if(write(sock, &data->writeAccessSize, sizeof(data->writeAccessSize)) <= 0){
                goto disconnect;
        }
        writeTemp = data->writeAccess;
        for(i=0; i<data->writeAccessSize; i++){
            if(write(sock, &writeTemp->clientId, sizeof(writeTemp->clientId)) <= 0){
                goto disconnect;
            }
            writeTemp = writeTemp->next;
        }

        if(write(sock, &data->writeWaitSize, sizeof(data->writeWaitSize)) <= 0){
                goto disconnect;
        }
        writeTemp = data->writeWait;
        for(i=0; i<data->writeWaitSize; i++){
            if(write(sock, &writeTemp->clientId, sizeof(writeTemp->clientId)) <= 0){
                goto disconnect;
            }
            writeTemp = writeTemp->next;
        }

        data = data->next;

        while(data==NULL && j<parameters.hashSize){
            data = hashTable[j];
            j++;
        }
        if(data==NULL){
            //continuer=0;
            msgType=MSG_REP_STOP;
        }
    }

    pthread_mutex_unlock(&hashTableMutex);

    if(write(sock, &msgType, sizeof(msgType)) <= 0){
        goto disconnect;
    }

#if DEBUG
    printf("fin snd_total_replication\n");
#endif

    return 1;
    disconnect:
    return -1;


}

int snd_server_to_client(int sock, struct clientChain *client){
    uint8_t msgType = MSG_ADD_SERVER;
    uint8_t taille = strlen(servers->serverAddress);

#if DEBUG
    printf("debut snd server %d, @: %s, port: %d to clients\n",servers->serverId,servers->serverAddress,servers->serverPort);
#endif

        pthread_mutex_lock(&client->mutex_sock);

        if(send_data(client->sock, msgType, 4,
                    (DS){sizeof(servers->serverId),&servers->serverId},
                    (DS){sizeof(taille),&taille},
                    (DS){taille,servers->serverAddress},
                    (DS){sizeof(servers->serverPort),&servers->serverPort})<0){
            goto disconnect;
        }

        pthread_mutex_unlock(&client->mutex_sock);


#if DEBUG
    printf("fin snd server to clients\n");
#endif

    return 1;
    disconnect:
    return -1;

}

int snd_server_to_clients(char *address, uint16_t port, uint8_t serverId){
    struct clientChain *client = clients;
    uint8_t msgType = MSG_ADD_SERVER;
    uint8_t taille = strlen(address);

#if DEBUG
    printf("debut snd server %d, @: %s, port: %d to clients\n",serverId,address,port);
#endif


    while(client!=NULL){

#if DEBUG
    printf("client id: %d\n",client->clientId);
#endif

        pthread_mutex_lock(&client->mutex_sock);

        if(send_data(client->sock, msgType, 4,
                    (DS){sizeof(serverId),&serverId},
                    (DS){sizeof(taille),&taille},
                    (DS){taille,address},
                    (DS){sizeof(port),&port})<0){
            goto disconnect;
        }

        pthread_mutex_unlock(&client->mutex_sock);
        client=client->next;
    }

#if DEBUG
    printf("fin snd server to clients\n");
#endif

    return 1;
    disconnect:
    return -1;
}

int snd_maj_client(struct replicationData *rep){
    struct serverChain *servTemp = servers;
    switch (rep->modification) {
        case ADD_CLIENT:
            if(send_data(servTemp->sock, MSG_ADD_CLIENT, 1,
                            (DS){sizeof(rep->clientId),&rep->clientId})<0){
                goto disconnect;
            }
            break;
        case REMOVE_CLIENT:
            if(send_data(servTemp->sock, MSG_RMV_CLIENT, 1,
                            (DS){sizeof(rep->clientId),&rep->clientId})<0){
                goto disconnect;
            }
            break;
        default:
            return 0;
    }
    rep->data=NULL;
    rep->clientId=0;
    return 1;
    disconnect:
    return -1;
}

int snd_data_replication(struct replicationData *rep){
    struct serverChain *servTemp = servers;
    uint8_t tailleNom;
    switch (rep->modification) {
        case DEFRAG:
            tailleNom = strlen(rep->data->name);
            if(send_data(servTemp->sock, MSG_DEFRAG_REPLICATION, 3,
                            (DS){sizeof(tailleNom),&tailleNom},
                            (DS){tailleNom, rep->data->name},
                            (DS){sizeof(rep->data->offset),&rep->data->offset})<0){
                goto disconnect;
            }
            break;
        case MAJ_ACCESS_READ:
            tailleNom = strlen(rep->data->name);
            if(send_data(servTemp->sock, MSG_MAJ_ACCESS_READ, 3,
                            (DS){sizeof(tailleNom),&tailleNom},
                            (DS){tailleNom, rep->data->name},
                            (DS){sizeof(rep->clientId),&rep->clientId})<0){
                goto disconnect;
            }
            break;
        case MAJ_ACCESS_WRITE:
            tailleNom = strlen(rep->data->name);
            if(send_data(servTemp->sock, MSG_MAJ_ACCESS_WRITE, 3,
                            (DS){sizeof(tailleNom),&tailleNom},
                            (DS){tailleNom, rep->data->name},
                            (DS){sizeof(rep->clientId),&rep->clientId})<0){
                goto disconnect;
            }
            break;
        case MAJ_WAIT_READ:
            tailleNom = strlen(rep->data->name);
            if(send_data(servTemp->sock, MSG_MAJ_WAIT_READ, 3,
                            (DS){sizeof(tailleNom),&tailleNom},
                            (DS){tailleNom, rep->data->name},
                            (DS){sizeof(rep->clientId),&rep->clientId})<0){
                goto disconnect;
            }
            break;
        case MAJ_WAIT_WRITE:
            tailleNom = strlen(rep->data->name);
            if(send_data(servTemp->sock, MSG_MAJ_WAIT_WRITE, 3,
                            (DS){sizeof(tailleNom),&tailleNom},
                            (DS){tailleNom, rep->data->name},
                            (DS){sizeof(rep->clientId),&rep->clientId})<0){
                goto disconnect;
            }
            break;
        case FREE_DATA:
            tailleNom = strlen(rep->data->name);
            if(send_data(servTemp->sock, MSG_FREE_REPLICATION, 2,
                            (DS){sizeof(tailleNom), &tailleNom},
                            (DS){tailleNom, rep->data->name})<0){
                goto disconnect;
            }
            break;
        case MAJ_DATA:
            tailleNom = strlen(rep->data->name);
            if(send_data(servTemp->sock, MSG_DATA_REPLICATION, 5,
                            (DS){sizeof(tailleNom), &tailleNom},
                            (DS){tailleNom, rep->data->name},
                            (DS){sizeof(rep->data->offset), &rep->data->offset},
                            (DS){sizeof(rep->data->size), &rep->data->size},
                            (DS){rep->data->size, theHeap + rep->data->offset})<0){
                goto disconnect;
            }
            break;
        case RELEASE_DATA_WRITE:
            tailleNom = strlen(rep->data->name);
            if(send_data(servTemp->sock, MSG_RELEASE_WRITE_REPLICATION, 3,
                            (DS){sizeof(tailleNom), &tailleNom},
                            (DS){tailleNom, rep->data->name},
                            (DS){sizeof(rep->clientId),&rep->clientId})<0){
                goto disconnect;
            }
            break;
        case RELEASE_DATA_READ:
            tailleNom = strlen(rep->data->name);
            if(send_data(servTemp->sock, MSG_RELEASE_READ_REPLICATION, 3,
                            (DS){sizeof(tailleNom), &tailleNom},
                            (DS){tailleNom, rep->data->name},
                            (DS){sizeof(rep->clientId),&rep->clientId})<0){
                goto disconnect;
            }
            break;
        default:                /* Unknown message code, version problem? */
            return 0;
    }
    rep->data=NULL;
    rep->clientId=0;
    return 1;
    disconnect:
    return -1;
}