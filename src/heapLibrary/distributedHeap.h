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

#include "dheapHashtable.h"


struct heapInfo {
    uint8_t mainId;
    uint64_t heapSize;
    void *heapStart;
    int sock;
};

struct dheapServer {
    uint8_t id;
    int sock;
    char *address;
    int port;
    struct dheapServer *next;
};

extern struct heapInfo *heapInfo;
extern struct dheapServer *dheapServers;
extern int countServers;
extern struct pollfd *poll_list;
extern char *dheapErrorMsg; /* Utilisé pour le ERROR_UNKNOWN_ERROR */
extern int *dheapErrorNumber; /* Utilisé pour passer une erreur au client */
extern uint8_t msgtypeClient;



/* TODO: enum partagé avec le serveur */
enum errorCodes {
    /* Codes d'erreur venant du serveur */
    DHEAP_ERROR_SERVER_FULL,
    DHEAP_ERROR_HEAP_FULL,
    DHEAP_ERROR_VAR_DOESNT_EXIST,
    DHEAP_ERROR_NOT_LOCKED,
    ERROR_UNKNOWN_ERROR,

    /* Codes d'erreur propres au client */
    DHEAP_SUCCESS,
    DHEAP_ERROR_CONNECTION,
    DHEAP_ERROR_UNEXPECTED_MSG,
    DHEAP_ERROR_HEAP_ALLOC,
    DHEAP_ERROR_BAD_POINTER,
    DHEAP_BAD_SERVER_ADDRESS,
    DHEAP_BAD_SERVER_PORT
};

/* TODO: enum partagé avec le serveur */
enum msgTypes {
    MSG_HEAP_SIZE,
    MSG_SERVER_ID,
    MSG_ALLOC,
    MSG_ACCESS_READ,
    MSG_ACCESS_READ_MODIFIED,
    MSG_ACCESS_WRITE,
    MSG_ACCESS_WRITE_MODIFIED,
    MSG_RELEASE,
    MSG_FREE,
    MSG_ERROR,
    MSG_DISCONNECT,
    MSG_PING,
    MSG_ADD_SERVER,
    MSG_CHANGE_MAIN,
    MSG_TYPE_NULL /* Utilisé entre le thread de la librairie et le thread client */
};

/* dataConnection.c */
int init_data(char *ip, int port);
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
/* servers.c */
int addserver(uint8_t id, char *address, int port);
int switchMain(uint8_t id);
void cleanServers();
