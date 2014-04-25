#ifndef HEAPSERVER_CLIENTS
#define HEAPSERVER_CLIENTS

struct clientChain {
	uint16_t clientId;
    pthread_t threadId;
    int sock;
    struct clientChain *next;
};

extern int clientsConnected;
extern struct clientChain *clients;

void *clientThread(void *arg);

#endif
