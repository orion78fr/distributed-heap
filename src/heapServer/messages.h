#ifndef HEAPSERVER_MESSAGES
#define HEAPSERVER_MESSAGES

enum msgTypes {
    MSG_HEAP_SIZE,
    MSG_ALLOC,
    MSG_ACCESS_READ,
	MSG_ACCESS_READ_MODIFIED,
    MSG_ACCESS_WRITE,
	MSG_ACCESS_WRITE_MODIFIED,
    MSG_RELEASE,
    MSG_FREE,
    MSG_ERROR,
    MSG_DISCONNECT
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
 *      type         |            envoi             |         réponse
 * ----------------------------------------------------------------------------
 * MSG_HEAP_SIZE     | taille du tas (int64)        |        /
 *     S -> C        |                              |
 * ----------------------------------------------------------------------------
 * MSG_ALLOC         | taille du nom (int8)         |
 *     C -> S        | nom (taille * char8)         |        /
 *                   | taille de la variable (int64)|
 * ----------------------------------------------------------------------------
 * MSG_ACCESS_READ   | taille du nom (int8)         | offset (int64)
 *     C -> S        | nom (taille * char8)         | taille (int64)
 *                   |                              | si modif (READ_MODIFIED)
 *                   |                              |   contenu (taille * char8)
 * ----------------------------------------------------------------------------
 * MSG_ACCESS_WRITE  | taille du nom (int8)         | offset (int64)
 *     C -> S        | nom (taille * char8)         | taille (int64)
 *                   |                              | si modif (WRITE_MODIFIED)
 *                   |                              |   contenu (taille * char8)
 * ----------------------------------------------------------------------------
 * MSG_RELEASE       | offset (int64)               |        
 *     C -> S        | taille (int64)               |        /
 *                   | contenu (taille * char8)     |
 * ----------------------------------------------------------------------------
 * MSG_FREE          | taille du nom (int8)         |        /
 *     C -> S        | nom (taille * char8)         |        
 * ----------------------------------------------------------------------------
 * MSG_ERROR         | type d'erreur (int8)         |
 *     S <-> C       | taille du message (int8)     |        /
 *                   | message (taille * char8)     |
 * ----------------------------------------------------------------------------
 * MSG_DISCONNECT    |         /                    |        /
 *     S <-> C       |                              |
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
