#ifndef HEAPSERVER_MESSAGES
#define HEAPSERVER_MESSAGES

enum msgTypes {
    MSG_HEAP_SIZE,
    MSG_SERVER_ID,
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
    MSG_CHANGE_MAIN
};

typedef struct dataSend{
    int taille;
    void *data;
} DS;


int send_data(int sock, int msgType, int nb, ...);
void *recv_data(int sock, int taille);

/*
 * Chaque échange commence par le type de message (int)
 * Puis, (super tableau en ASCII-art)
 * MSG TYPE -> int8
 *
 *      type         |            envoi              |         réponse
 * -----------------------------------------------------------------------------
 * MSG_HEAP_SIZE     | taille du tas (uint64)        |        /
 *     S -> C        |                               |
 * -----------------------------------------------------------------------------
 * MSG_SERVER_ID     | id du serveur (uint8)         |        /
 *     S -> C        |                               |
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
 *     C -> S        | si WR: taille (uint64)        |        /
 *                   | si WR: contenu (taille*char8) |
 * -----------------------------------------------------------------------------
 * MSG_FREE          | taille du nom (uint8)         |        /
 *     C -> S        | nom (taille*char8)            |
 * -----------------------------------------------------------------------------
 * MSG_ERROR         | type d'erreur (uint8)         |
 *     S <-> C       | taille du message (uint8)     |        /
 *                   | message (taille*char8)        |
 * -----------------------------------------------------------------------------
 * MSG_DISCONNECT    |         /                     |        /
 *     S <-> C       |                               |
 * -----------------------------------------------------------------------------
 * MSG_ADD_SERVER     | id du serveur (uint8)        |        /
 *     S -> C        | taille de l'adresse (uint8)   |
 *                   | adresse (taille*char8)        |
 *                   | port (uint16)                 |
 * -----------------------------------------------------------------------------
 * MSG_PING          |         /                     |        /
 *     S <-> C       |                               |
 * -----------------------------------------------------------------------------
 * MSG_CHANGE_MAIN   | id du nouveau main (uint8)    | ack (uint8) -> OK
 *     S -> C        |                               | ou -> UNKNOWN SERVER ID
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
