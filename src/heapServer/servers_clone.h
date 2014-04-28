#ifndef HEAPSERVER_SERVERS
#define HEAPSERVER_SERVERS

struct serverChain {
	int backup; // 0: main server, 1: backup server
	uint16_t serverId;
    pthread_t threadId;
    int sock;
    char *serverAddress;
};

extern int serversConnected;
extern struct serverChain *servers;

void *serverThread(void *arg);

#endif