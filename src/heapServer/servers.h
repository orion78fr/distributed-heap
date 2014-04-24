#ifndef HEAPSERVER_SERVERS
#define HEAPSERVER_SERVERS

struct serverChain {
    pthread_t serverId;
    int sock;
    struct serverChain *next;
};

extern int serversConnected;
extern struct serverChain *servers;
extern struct pollfd *poll_list;


void *serverThread(void *arg);

#endif