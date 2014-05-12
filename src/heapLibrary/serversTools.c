#include "distributedHeap.h"

/**
 * Supprime un serveur de la liste de serveurs
 * @param serveur id
 * @return 0 si réussi, -1 sinon
 */
int removeServer(uint8_t sid){
    struct dheapServer *tmp, *prev;

    tmp = dheapServers;
    prev = NULL;

    while (tmp != NULL){
        if (tmp->id == sid){
            setServerDown(sid);
            if (prev == NULL)
                dheapServers = tmp->next;
            else
                prev->next = tmp->next;
            free(tmp->address);
            free(tmp);
            return 0;
        }
        prev = tmp;
        tmp = tmp->next;
    }

    return -1;
}

/**
 * Retourne l'id d'un serveur depuis sa socket
 * @param socket
 * @return serveur id
 */
uint8_t getServerIdBySock(int sock){
    struct dheapServer *tmp;

    tmp = dheapServers;

    while (tmp->sock != sock && tmp != NULL)
        tmp = tmp->next;

    if (tmp == NULL)
        exit(EXIT_FAILURE);

    return tmp->id;
}

/**
 * Retourne la structure d'un serveur depuis sa socket
 * @param socket
 * @return structure dheapServer associé à la socket
 */
struct dheapServer* getServerBySock(int sock){
    struct dheapServer *tmp;

    tmp = dheapServers;

    while (tmp->sock != sock && tmp != NULL)
        tmp = tmp->next;

    if (tmp == NULL)
        exit(EXIT_FAILURE);

    return tmp;
}

/**
 * Retourne la structure d'un serveur depuis son id
 * @param serveur id
 * @return structure dheapServer associé à l'id
 */
struct dheapServer* getServerById(uint8_t sid){
    struct dheapServer *tmp;

    tmp = dheapServers;

    while (tmp->id != sid && tmp != NULL)
        tmp = tmp->next;

    if (tmp == NULL)
        exit(EXIT_FAILURE);

    return tmp;
}

/**
 * Actualise l'heure du dernier message reçu d'un serveur
 * @param serveur id
 */
void setTime(uint8_t sid){
    struct dheapServer *ds;
    ds = getServerById(sid);
    ds->lastMsgTime = time(NULL);
}

/**
 * Fais un poll avant un read, permettant d'avoir un timeout sur le read
 * @param socket, buffer, taille
 * @return taille récupéré ou erreur (voir man 2 read)
 */
ssize_t readWithPoll(int fd, void *buf, size_t count){
    struct pollfd poll_list[1];
    int retval;

    poll_list[0].fd = fd;
    poll_list[0].events = POLLIN;
    retval = poll(poll_list, 1, PONG_TIMEOUT);
    if (retval < 0){
        exit(EXIT_FAILURE);
    } else if (retval == 0){
        return -1;
    } else {
        return read(fd, buf, count);
    }
}
