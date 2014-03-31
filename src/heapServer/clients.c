#include "common.h"

int clientsConnected = 0;
struct clientChain *clients = NULL;

struct tempCorrespondance{
    int offset;
    char *name;
    int write;
    struct tempCorrespondance *next;
} *corresp = NULL;

/**
 * Thread du client
 * @param arg Socket de communication du client
 */
void *clientThread(void *arg)
{
    int sock = (int) arg;

    void *content = NULL;
    int msgType;
    int temp;
    int erreur;

#if DEBUG
    printf("[Client %d] Connexion\n", pthread_self());
#endif

    /* Envoi de la taille du stack */
	if(send_data(sock, MSG_HEAP_SIZE, 1, 
                    (DS){sizeof(int), &(parameters.heapSize)})<0){
		goto disconnect;
	}

    /* Boucle principale */
    for (;;) {
        if (read(sock, (void *) &msgType, sizeof(msgType)) <= 0) {       /* Msg type */
            goto disconnect;
        }

        /* Switch pour les différents types de messages */
        switch (msgType) {
        case MSG_ALLOC: /* Allocation d'une variable */
            if (read(sock, (void *) &temp, sizeof(temp)) <= 0) { /* Name size */
                goto disconnect;
            }
            content = malloc(temp * sizeof(char) + 1);
            ((char *) content)[temp] = '\0';
            if (read(sock, content, temp) <= 0) {        /* Name */
                goto disconnect;
            }
            if (read(sock, (void *) &temp, sizeof(temp)) <= 0) { /* Var size */
                goto disconnect;
            }
#if DEBUG
            printf("[Client %d] Allocation de %s de taille %d\n",
                   pthread_self(), content, temp);
#endif

            if (add_data(content, temp) != 0) {
                /* ERREUR */
                int errType=ERROR_VAR_NAME_EXIST;
                int messageSize=0;
                if(send_data(sock, MSG_ERROR, 2, 
                                (DS){sizeof(int), &errType},
                                (DS){sizeof(int), &messageSize})<0){
                    goto disconnect;
                }
            } else {
                /* OK */
                content = NULL;
                if(send_data(sock, MSG_ALLOC, 0)<0){
                    goto disconnect;
                }
            }
            break;
        case MSG_ACCESS_READ:   /* Demande d'accès en lecture */
            if (read(sock, (void *) &temp, sizeof(temp)) <= 0) { /* Name size */
                goto disconnect;
            }
            content = malloc(temp * sizeof(char) + 1);
            ((char *) content)[temp] = '\0';
            if (read(sock, content, temp) <= 0) {        /* Name */
                goto disconnect;
            }
#if DEBUG
            printf("[Client %d] Demande d'accès en lecture de %s\n",
                   pthread_self(), content);
#endif
            
            if(acquire_read_lock(content) != 0) {
                /* ERREUR */
                int errType=ERROR_VAR_DOESNT_EXIST;
                int messageSize=0;
                if(send_data(sock, MSG_ERROR, 2, 
                                (DS){sizeof(int), &errType},
                                (DS){sizeof(int), &messageSize})<0){
                    goto disconnect;
                }
            } else {
                struct heapData *data = get_data(content);
                int offset = data->offset;
                int size = data->size;
                
                struct tempCorrespondance *newData = malloc(sizeof(struct tempCorrespondance));
                newData->offset = offset;
                newData->name = content;
                content = NULL;
                newData->next = corresp;
                newData->write = 0;
                corresp = newData;
                
                /* OK */
                if(send_data(sock, MSG_ACCESS_READ_MODIFIED, 3, 
                                (DS){sizeof(int), &offset},
                                (DS){sizeof(int), &size},
                                (DS){size, theHeap + data->offset})<0){
                    goto disconnect;
                }
            }
            break;
        case MSG_ACCESS_WRITE:  /* Demande d'accès en écriture */
            if (read(sock, (void *) &temp, sizeof(temp)) <= 0) { /* Name size */
                goto disconnect;
            }
            content = malloc(temp * sizeof(char) + 1);
            ((char *) content)[temp] = '\0';
            if (read(sock, content, temp) <= 0) {        /* Name */
                goto disconnect;
            }
#if DEBUG
            printf("[Client %d] Demande d'accès en écriture de %s\n",
                   pthread_self(), content);
#endif

            if(acquire_write_lock(content) != 0) {
                /* ERREUR */
                int errType=ERROR_VAR_DOESNT_EXIST;
                int messageSize=0;
                if(send_data(sock, MSG_ERROR, 2, 
                                (DS){sizeof(int), &errType},
                                (DS){sizeof(int), &messageSize})<0){
                    goto disconnect;
                }
            } else {
                struct heapData *data = get_data(content);
                int offset = data->offset;
                int size = data->size;
                
                struct tempCorrespondance *newData = malloc(sizeof(struct tempCorrespondance));
                newData->offset = offset;
                newData->name = content;
                content = NULL;
                newData->next = corresp;
                newData->write = 1;
                corresp = newData;
                
                /* OK */
                if(send_data(sock, MSG_ACCESS_WRITE_MODIFIED, 3, 
                                (DS){sizeof(int), &offset},
                                (DS){sizeof(int), &size},
                                (DS){size, theHeap + data->offset})<0){
                    goto disconnect;
                }
            }

            break;

        case MSG_RELEASE:       /* Relachement de la variable */
            if (read(sock, (void *) &temp, sizeof(temp)) <= 0) { /* Offset */
                goto disconnect;
            } else {
                struct tempCorrespondance *prevData = NULL, *data = corresp;
                while(data->offset != temp){
                    prevData = data;
                    data = data->next;
                }
#if DEBUG
                printf("[Client %d] Libération de %s\n", pthread_self(),
                   data->name);
#endif
                content = malloc(sizeof(int));
                if (read(sock, content, sizeof(int)) <= 0) {        /* Taille */
                    goto disconnect;
                }
                if (read(sock, theHeap+temp, *(int*)content) <= 0) {        /* Contenu */
                    goto disconnect;
                }
                if(data->write){
                    release_write_lock(data->name);
                } else {
                    release_read_lock(data->name);
                }
                if(prevData == NULL){
                    corresp = data->next;
                } else {
                    prevData->next = data->next;
                }
                free(data->name);
                free(data);
                if(send_data(sock, MSG_RELEASE, 0)<0){
                    goto disconnect;
                }
            }
            
            break;

        case MSG_FREE:          /* Désallocation d'une variable */
            if (read(sock, (void *) &temp, sizeof(temp)) <= 0) { /* Name size */
                goto disconnect;
            }
            content = malloc(temp * sizeof(char) + 1);
            ((char *) content)[temp] = '\0';
            if (read(sock, content, temp) <= 0) {        /* Name */
                goto disconnect;
            }
#if DEBUG
            printf("[Client %d] Désallocation de %s\n", pthread_self(),
                   content);
#endif

            if(remove_data(content) != 0){
                /* ERREUR */
                goto disconnect;
            } else {
                /* OK */
                if(send_data(sock, MSG_FREE, 0)<0){
                    goto disconnect;
                }
            }

            break;
            
        case MSG_HEAP_SIZE:     /* Clients should not send that */
        case MSG_ERROR: /* Message d'erreur */
        case MSG_DISCONNECT:    /* Message de déconnection */
        default:                /* Unknown message code, version problem? */
            goto disconnect;
        }

        if (content != NULL) {
            free(content);
            content = NULL;
        }
    }

disconnect:

#if DEBUG
    printf("[Client %d] Déconnexion\n", pthread_self());
#endif

    if (content != NULL) {
        free(content);
    }
    /* Fermer la connexion */
    shutdown(sock, 2);
    close(sock);
    clientsConnected--;
    pthread_exit(NULL);
    return NULL;
}