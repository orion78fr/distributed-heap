#ifndef HEAPSERVER_SERVERS
#define HEAPSERVER_SERVERS

struct serverChain {
	uint16_t serverId;
    pthread_t threadId;
    int sock;
    struct serverChain *next;
    char *serverAddress;
};

extern int serversConnected;
extern struct serverChain *servers;
extern struct pollfd *poll_list;


void *serverThread(void *arg);

#endif