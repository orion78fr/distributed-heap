#ifndef HEAPSERVER_SERVERS
#define HEAPSERVER_SERVERS

struct serverChain {
    pthread_t serverId;
    int sock;
    struct serverChain *next;
};

extern int serversConnected;
extern struct serverChain *servers;

void *serverThread(void *arg);

#endif