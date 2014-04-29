#ifndef HEAPSERVER_SERVERS
#define HEAPSERVER_SERVERS

struct serverChain {
	int backup; // 0: main server, 1: backup server
	uint8_t serverId;
    pthread_t threadId;
    int sock;
    char *serverAddress;
    uint16_t serverPort;
    struct serverChain *next;
};

extern uint8_t numServer;
extern int serversConnected;
extern struct serverChain *servers;
extern struct replicationData *rep;
extern struct replicationAck *ack;
extern pthread_key_t id;

void *serverThread(void *arg);

#endif
