#ifndef HEAPSERVER_MESSAGES
#define HEAPSERVER_MESSAGES

/* GTU : 256 char pour un message ça me paraît suffisant non? */
#define MESSAGE_SIZE 256
enum msgTypes {
    MSG_HEAP_SIZE,
    MSG_ALLOC,
    MSG_ACCESS_READ,
    MSG_ACCESS_WRITE,
    MSG_RELEASE,
    MSG_FREE,
    MSG_ERROR
};

struct message {
    int msgType;
    union {
	int asInteger;
	char asString[MESSAGE_SIZE];
    } content;
};
/* GTU : Peut-être ne pas utiliser de messages mais utiliser la socket en mode 
 * flux pour lire au fur et à mesure (et donc ne plus se préoccuper d'une 
 * taille max mais utiliser une taille envoyée pour lire la suite) */

#endif
