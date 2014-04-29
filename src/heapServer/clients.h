#ifndef HEAPSERVER_CLIENTS
#define HEAPSERVER_CLIENTS

struct clientChain {
	uint16_t clientId;
    pthread_t threadId;
    int sock;
    struct clientChain *next;
};

extern int numClient;
extern int clientsConnected;
extern struct clientChain *clients;
extern struct replicationData *rep;
extern struct replicationAck *ack;
extern pthread_key_t id;

void *clientThread(void *arg);

#endif
