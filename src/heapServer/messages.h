#ifndef HEAPSERVER_MESSAGES
#define HEAPSERVER_MESSAGES

enum msgTypes {
    MSG_HELLO_NEW,
    MSG_HELLO_NOT_NEW,
    MSG_ALLOC,
    MSG_ACCESS_READ,
    MSG_ACCESS_READ_MODIFIED,
    MSG_ACCESS_WRITE,
    MSG_ACCESS_WRITE_MODIFIED,
    MSG_RELEASE,
    MSG_FREE,
    MSG_ERROR,
    MSG_DISCONNECT,
    MSG_PING,
    MSG_ADD_SERVER,
    MSG_REMOVE_SERVER
};

typedef struct dataSend{
    int taille;
    void *data;
} DS;


int send_data(int sock, uint8_t msgType, int nb, ...);
int send_error(int sock, uint8_t errType);

int do_greetings(int sock);
int do_alloc(int sock);
int do_access_read(int sock);
int do_access_write(int sock);
int do_release(int sock);
int do_free(int sock);

/*
 * Chaque échange commence par le type de message (uint8)
 * Puis, (super tableau en ASCII-art)
 *
 *      type         |            envoi              |         réponse
 * -----------------------------------------------------------------------------
 * MSG_HELLO_NEW     |         /                     | Server id (uint8)
 *     C -> S        |                               | Client id (uint16)
 *                   |                               | Heap size (uint64)
 * -----------------------------------------------------------------------------
 * MSG_HELLO_NOT_NEW | Client id (uint16)            |        /
 *     C -> S        |                               |
 * -----------------------------------------------------------------------------
 * MSG_ALLOC         | taille du nom (uint8)         |
 *     C -> S        | nom (taille*char8)            |        /
 *                   | taille de la variable (uint64)|
 * -----------------------------------------------------------------------------
 * MSG_ACCESS_READ   | taille du nom (uint8)         | offset (uint64)
 *     C -> S        | nom (taille * char8)          | taille (uint64)
 *                   |                               | si modif (READ_MODIFIED)
 *                   |                               |   contenu (taille*char8)
 * -----------------------------------------------------------------------------
 * MSG_ACCESS_WRITE  | taille du nom (uint8)         | offset (uint64)
 *     C -> S        | nom (taille*char8)            | taille (uint64)
 *                   |                               | si modif (WRITE_MODIFIED)
 *                   |                               |   contenu (taille*char8)
 * -----------------------------------------------------------------------------
 * MSG_RELEASE       | offset (uint64)               |
 *     C -> S        | si accès en write             |
 *                   |   taille (uint64)             |        /
 *                   |   contenu (taille*char8)      |
 * -----------------------------------------------------------------------------
 * MSG_FREE          | taille du nom (uint8)         |        /
 *     C -> S        | nom (taille*char8)            |
 * -----------------------------------------------------------------------------
 * MSG_ERROR         | type d'erreur (uint8)         |        /
 *     S <-> C       |                               |
 * -----------------------------------------------------------------------------
 * MSG_DISCONNECT    |         /                     |        /
 *     S <-> C       |                               |
 * -----------------------------------------------------------------------------
 * MSG_ADD_SERVER    | id du serveur (uint8)         |        /
 *     S -> C        | taille de l'adresse (uint8)   |
 *                   | adresse (taille*char8)        |
 *                   | port (uint16)                 |
 * -----------------------------------------------------------------------------
 * MSG_REMOVE_SERVER | id du serveur (uint8)         |        /
 *     S -> C        |                               |
 * -----------------------------------------------------------------------------
 * MSG_PING          |         /                     |        /
 *     S <-> C       |                               |
 * -----------------------------------------------------------------------------
 *
 * ATTENTION, on risque d'avoir des problèmes en utilisant des types genre int
 *  à cause des différences des machine (32 bit, 64 bit). On devrait utiliser
 *  des types spécifiques (indiquant le nombre de bits exact).
 *  Voir : http://www.nongnu.org/avr-libc/user-manual/group__avr__stdint.html
 *
 * Les réponses doivent aussi contenir le type de message (int) pour permettre
 *  la gestion des erreurs. Le type vaut celui de la demande si OK, le type
 *  MSG_ERROR sinon.
 */

#endif
