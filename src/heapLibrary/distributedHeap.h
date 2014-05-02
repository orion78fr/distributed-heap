#define _XOPEN_SOURCE 700


/* TODO: Headers POSIX à mettre dans un common */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <poll.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

#include "dheapHashtable.h"

#define DH_SERVER_RETRY 3
#define PONG_TIMEOUT 2
#define TIMEOUT_BEFORE_PING 4


struct heapInfo {
    uint8_t mainId;
    uint64_t heapSize;
    void *heapStart;
    int sock;
    uint16_t clientId;
};

struct dheapServer {
    uint8_t id;
    int sock;
    char *address;
    int port;
    uint8_t status; /* 0 = non connecté, 1 = connecté, 2 = connecting */
    struct dheapServer *next;
    time_t lastMsgTime;
    time_t lastPing;
    time_t lastConnect;
};

extern struct heapInfo *heapInfo;
extern struct dheapServer *dheapServers;
extern int countServersOnline;
extern struct pollfd *poll_list;
extern uint8_t *dheapErrorNumber; /* Utilisé pour passer une erreur au client */
extern uint8_t msgtypeClient;
extern pthread_t *dheap_tid;
extern pthread_mutex_t readlock;
extern pthread_mutex_t writelock;
extern pthread_mutex_t mainlock;
extern pthread_mutex_t readylock;
extern pthread_mutex_t polllock;
extern pthread_cond_t readcond;



/* TODO: enum partagé avec le serveur */
enum errorCodes {
    /* Codes d'erreur venant du serveur */
    DHEAP_ERROR_SERVER_FULL,
    DHEAP_ERROR_HEAP_FULL,
    DHEAP_ERROR_VAR_DOESNT_EXIST,
    DHEAP_ERROR_VAR_NAME_EXIST,
    DHEAP_ERROR_NOT_LOCKED,
    ERROR_UNKNOWN_ERROR,

    /* Codes d'erreur propres au client */
    DHEAP_SUCCESS,
    DHEAP_ERROR_CONNECTION,
    DHEAP_ERROR_UNEXPECTED_MSG,
    DHEAP_ERROR_HEAP_ALLOC,
    DHEAP_ERROR_BAD_POINTER,
    DHEAP_BAD_SERVER_ADDRESS,
    DHEAP_BAD_SERVER_PORT,

    /* utilisation interne */
    DHEAP_RETRY
};

/* TODO: enum partagé avec le serveur */
enum msgTypes {
    MSG_HELLO,
    MSG_HELLO_NEW,
    MSG_HELLO_NEW_SERVER,
    MSG_HELLO_NOT_NEW,
    MSG_ALLOC,
    MSG_ACCESS_READ,
    MSG_ACCESS_READ_BY_OFFSET,
    MSG_ACCESS_READ_MODIFIED,
    MSG_ACCESS_WRITE,
    MSG_ACCESS_WRITE_BY_OFFSET,
    MSG_ACCESS_WRITE_MODIFIED,
    MSG_RELEASE,
    MSG_FREE,
    MSG_ERROR,
    MSG_DISCONNECT,
    MSG_DISCONNECT_RELEASE_ALL,
    MSG_PING,
    MSG_ADD_SERVER,
    MSG_REMOVE_SERVER,
    MSG_RETRY,
    MSG_TOTAL_REPLICATION,
    MSG_PARTIAL_REPLICATION,
    MSG_DATA_REPLICATION,
    MSG_FREE_REPLICATION,
    MSG_RELEASE_REPLICATION,
    MSG_MAJ_ACCESS_READ,
    MSG_MAJ_ACCESS_WRITE,
    MSG_MAJ_WAIT_READ,
    MSG_MAJ_WAIT_WRITE,
    MSG_ADD_CLIENT,
    MSG_RMV_CLIENT,
    MSG_ACK,
    MSG_DEFRAG_REPLICATION,
    MSG_TYPE_NULL /* Utilisé entre le thread de la librairie et le thread client */
};

/* dataConnection.c */
int init_data(char *address, int port);
int close_data();
/* allocation.c */
int t_malloc(uint64_t size, char *name);
int t_free(char *name);
/* access.c */
int t_access_read(char *name, void **p);
int t_access_write(char *name, void **p);
int t_access_read_byoffset(uint64_t offset, void **p);
int t_access_write_byoffset(uint64_t offset, void **p);
int t_access_common(uint8_t msgtype, char *name, void **p, uint64_t offset);
int t_release(void *p);
/* ack.c */
int receiveAck(uint8_t msgtype);
int receiveAckPointer(uint8_t *msgtypeP);
/* thread.c */
void *data_thread(void *arg);
void exit_data_thread(uint8_t e);
/* distributedHeap.c */
int checkError();
void setError(uint8_t e);
void unlockAndSignal();
void setDownAndSwitch(uint8_t sid);
/* servers.c */
int addserver(uint8_t id, char *address, int port);
void cleanServers();
int connectToServer(char *address, int port, int block);
void reconnectServers();
void helloNotNew(uint8_t sid);
/* serversPoll.c */
void buildPollList();
void setServerDownInternal(uint8_t id, int doBuildPollList);
void setServerDown(uint8_t id);
void setServerDownNoRebuild(uint8_t id);
int switchMain();
/* serversTools.c */
int removeServer(uint8_t sid);
void setTime(uint8_t sid);
uint8_t getServerIdBySock(int sock);
struct dheapServer* getServerBySock(int sock);
struct dheapServer* getServerById(uint8_t sid);
