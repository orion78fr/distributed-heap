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

#include "dheapHashtable.h"

#define DH_SERVER_RETRY 3


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
extern int isReqCurrently; /* 0 = NON, 1 = OUI */ 



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
    MSG_HELLO_NEW,
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
    MSG_PING,
    MSG_ADD_SERVER,
    MSG_REMOVE_SERVER,
    MSG_RETRY,
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
int t_access_common(uint8_t msgtype, char *name, void **p);
int t_release(void *p);
/* distributedHeap.c */
int receiveAck(uint8_t msgtype);
int receiveAckPointer(uint8_t *msgtypeP);
void *data_thread(void *arg);
void exit_data_thread(int e);
int checkError();
void setError(uint8_t e);
void unlockAndSignal();
void setDownAndSwitch(uint8_t sid);
/* servers.c */
int addserver(uint8_t id, char *address, int port);
int switchMain();
void cleanServers();
int connectToServer(char *address, int port, int block);
void buildPollList();
void setServerDown(uint8_t id);
uint8_t getServerIdBySock(int sock);
struct dheapServer* getServerBySock(int sock);
struct dheapServer* getServerById(uint8_t sid);
void helloNotNew(uint8_t sid);
