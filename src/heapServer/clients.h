#ifndef HEAPSERVER_CLIENTS
#define HEAPSERVER_CLIENTS

struct clientChain {
    int clientId;
    int sock;
    struct clientChain *next;
};

extern int clientsConnected;
extern struct clientChain *clients;

void *clientThread(void *arg);

#endif
