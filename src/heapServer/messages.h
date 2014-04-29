#ifndef HEAPSERVER_MESSAGES
#define HEAPSERVER_MESSAGES

enum msgTypes {
    MSG_HELLO_NEW_CLIENT,
    MSG_HELLO_NEW_SERVER,
    MSG_HELLO_NOT_NEW,
    MSG_ALLOC,
    MSG_ACCESS_READ,
    MSG_ACCESS_READ_BY_OFFSET,
    MSG_ACCESS_READ_MODIFIED,
    MSG_ACCESS_WRITE,
    MSG_ACCESS_WRITE_BY_OFFSET,
    MSG_ACCESS_WRITE_MODIFIED,
    MSG_RELEASE,
    MSG_FREE,
    MSG_ERROR,
    MSG_DISCONNECT,
    MSG_PING,
    MSG_ADD_SERVER,
    MSG_REMOVE_SERVER,
    MSG_RETRY,
    MSG_TOTAL_REPLICATION,
    MSG_PARTIAL_REPLICATION,
    MSG_DATA_REPLICATION,
    MSG_FREE_REPLICATION,
    MSG_RELEASE_REPLICATION,
    MSG_MAJ_ACCESS_READ,
    MSG_MAJ_ACCESS_WRITE,
    MSG_MAJ_WAIT_READ,
    MSG_MAJ_WAIT_WRITE,
    MSG_ADD_CLIENT,
    MSG_RMV_CLIENT,
    MSG_ACK,
    MSG_DEFRAG_REPLICATION
};

typedef struct dataSend{
    int taille;
    void *data;
} DS;

extern struct replicationData *rep;
extern struct replicationAck *ack;
extern pthread_key_t id;
extern struct clientChain *clients;

int send_data(int sock, uint8_t msgType, int nb, ...);
int send_error(int sock, uint8_t errType);

int do_greetings(int sock);
int do_alloc(int sock);
int do_access_read(int sock);
int do_access_read_by_offset(int sock);
int do_access_read_common(int sock, struct heapData *data);
int do_access_write(int sock);
int do_access_write_by_offset(int sock);
int do_access_write_common(int sock, struct heapData *data);
int do_release(int sock);
int do_free(int sock);
int snd_total_replication(int sock);
int rcv_total_replication(int sock);
int snd_partial_replication(struct heapData *data);
int rcv_partial_replication(int sock);
int snd_data_replication(struct replicationData *rep);
int rcv_data_replication(int sock);
int rcv_release_replication(int sock);
int rcv_free_replication(int sock);
int rcv_new_client(int sock);
int rcv_rmv_client(int sock);
int rcv_maj_access_read(int sock);
int rcv_maj_access_write(int sock);
int rcv_maj_wait_read(int sock);
int rcv_maj_wait_write(int sock);
int snd_server_to_clients(char *address, uint16_t port);
int snd_defrag_replication(struct replicationData *rep);
int rcv_defrag_replication(int sock);


/*
 * Chaque échange commence par le type de message (uint8)
 * Puis, (super tableau en ASCII-art)
 *
 *      type               |            envoi              |         réponse
 * -----------------------------------------------------------------------------
 * MSG_HELLO_NEW_CLIENT    |             /                 | Server id (uint8)
 *     C -> S              |                               | Client id (uint16)
 *                         |                               | Heap size (uint64)
 * -----------------------------------------------------------------------------
 * MSG_HELLO_NEW_SERVER    |             /                 | Server id (uint8)
 *     S -> S              |                               | Server id (uint16)
 * -----------------------------------------------------------------------------
 * MSG_HELLO_NOT_NEW       | Client id (uint16)            |          /
 *     C -> S              |                               |
 * -----------------------------------------------------------------------------
 * MSG_ALLOC               | taille du nom (uint8)         |
 *     C -> S              | nom (taille*char8)            |          /
 *                         | taille de la variable (uint64)|
 * -----------------------------------------------------------------------------
 * MSG_ACCESS_READ         | taille du nom (uint8)         | offset (uint64)
 *     C -> S              | nom (taille * char8)          | taille (uint64)
 *                         |                               | si modif (READ_MODIFIED)
 *                         |                               |   contenu (taille*char8)
 * -----------------------------------------------------------------------------
 * MSG_ACCESS_WRITE        | taille du nom (uint8)         | offset (uint64)
 *     C -> S              | nom (taille*char8)            | taille (uint64)
 *                         |                               | si modif (WRITE_MODIFIED)
 *                         |                               |   contenu (taille*char8)
 * -----------------------------------------------------------------------------
 * MSG_RELEASE             | offset (uint64)               |
 *     C -> S              | si accès en write             |          /
 *                         |   contenu (taille*char8)      |
 * -----------------------------------------------------------------------------
 * MSG_FREE                | taille du nom (uint8)         |          /
 *     C -> S              | nom (taille*char8)            |
 * -----------------------------------------------------------------------------
 * MSG_ERROR               | type d'erreur (uint8)         |          /
 *     S <-> C             |                               |
 * -----------------------------------------------------------------------------
 * MSG_DISCONNECT          |             /                 |          /
 *     S <-> C             |                               |
 * -----------------------------------------------------------------------------
 * MSG_ADD_SERVER          | id du serveur (uint8)         |          /
 *     S -> C              | taille de l'adresse (uint8)   |
 *                         | adresse (taille*char8)        |
 *                         | port (uint16)                 |
 * -----------------------------------------------------------------------------
 * MSG_REMOVE_SERVER       | id du serveur (uint8)         |          /
 *     S -> C              |                               |
 * -----------------------------------------------------------------------------
 * MSG_PING                |             /                 |          /
 *     S <-> C             |                               |
 * -----------------------------------------------------------------------------
 * MSG_TOTAL_REPLICATION   |             /                 | nombre servers
 *     S <-> S             |             /                 | id serveur
 *                         |             /                 | taille @ serveur
 *                         |             /                 | @ serveur
 *                         |             /                 | continue (uint8)
 *                         |             /                 | taille nom
 *                         |             /                 | nom
 *                         |             /                 | offset
 *                         |             /                 | size
 *                         |             /                 | lock en attente
 * -----------------------------------------------------------------------------
 * MSG_PARTIAL_REPLICATION |  taille du nom (uint8)        |          /
 *     S <-> S             |  nom (taille*char8)           |          /
 *                         |  offset (uint64)              |          /
 *                         |  taille (uint64)              |          /
 *                         |  contenu (taille*char8)       |          /
 *                         |  lock en attente              |          /
 * -----------------------------------------------------------------------------
 * MSG_MAJ_ACCESS          |  taille du nom (uint8)        |          /
 *     S <-> S             |  nom (taille*char8)           |          /
 *                         |  lock en read ou write        |          /
 * -----------------------------------------------------------------------------
 * MSG_MAJ_WAIT            |  taille du nom (uint8)        |          /
 *     S <-> S             |  nom (taille*char8)           |          /
 *                         |  wait en read ou write        |          /
 * -----------------------------------------------------------------------------
 * MSG_DATA_REPLICATION    |  taille du nom (uint8)        |          /
 *     S <-> S             |  nom (taille*char8)           |          /
 *                         |  offset (uint64)              |          /
 *                         |  taille (uint64)              |          /
 *                         |  contenu (taille*char8)       |          /
 * -----------------------------------------------------------------------------
 * MSG_RETRY               | Le message à réessayer        |          /
 *     C -> S              |                               |
 * -----------------------------------------------------------------------------
 * MSG_DEFRAG_REPLICATION  |  taille du nom (uint8)        |          /
 *     S <-> S             |  nom (taille*char8)           |          /
 *                         |  nouvel offset (uint64)       |          /
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
