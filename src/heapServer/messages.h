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
 *
 *      type         |            envoi             |         réponse
 * ----------------------------------------------------------------------------
 * MSG_HEAP_SIZE     | taille du tas (int)          |        /
 *     S -> C        |                              |
 * ----------------------------------------------------------------------------
 * MSG_ALLOC         | taille du nom (int)          |
 *     C -> S        | nom (taille * char)          |        /
 *                   | taille de la variable (int)  |
 * ----------------------------------------------------------------------------
 * MSG_ACCESS_READ   | taille du nom (int)          | offset (int)
 *     C -> S        | nom (taille * char)          | taille (int)
 *                   |                              | si modif (READ_MODIFIED)
 *                   |                              |   contenu (taille * char)
 * ----------------------------------------------------------------------------
 * MSG_ACCESS_WRITE  | taille du nom (int)          | offset (int)
 *     C -> S        | nom (taille * char)          | taille (int)
 *                   |                              | si modif (WRITE_MODIFIED)
 *                   |                              |   contenu (taille * char)
 * ----------------------------------------------------------------------------
 * MSG_RELEASE       | offset (int)                 |        
 *     C -> S        | taille (int)                 |        /
 *                   | contenu (taille * char)      |
 * ----------------------------------------------------------------------------
 * MSG_FREE          | taille du nom (int)          |        /
 *     C -> S        | nom (taille * char)          |        
 * ----------------------------------------------------------------------------
 * MSG_ERROR         | type d'erreur (int)          |
 *     S <-> C       | taille du message (int)      |        /
 *                   | message (taille * char)      |
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
