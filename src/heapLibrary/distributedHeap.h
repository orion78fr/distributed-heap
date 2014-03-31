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

#include "dheapHashtable.h"


#define DHEAP_SERVER_ADDRESS "127.0.1.1"
#define DHEAP_SERVER_PORT 6969

struct heapInfo {
    int heapSize;
    void *heapStart;
    int sock;
};

extern struct heapInfo *heapInfo;
extern char *dheapErrorMsg;


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
    DHEAP_ERROR_BAD_POINTER
};

/* TODO: enum partagé avec le serveur */
enum msgTypes {
    MSG_HEAP_SIZE,
    MSG_ALLOC,
    MSG_ACCESS_READ,
    MSG_ACCESS_READ_MODIFIED,
    MSG_ACCESS_WRITE,
    MSG_ACCESS_WRITE_MODIFIED,
    MSG_RELEASE,
    MSG_FREE,
    MSG_ERROR,
    MSG_DISCONNECT
};

int init_data();
int close_data();
int t_malloc(int size, char *name);
int t_access_read(char *name, void **p);
int t_access_write(char *name, void **p);
int t_access_common(int msgtype, char *name, void **p);
int t_release(void *p);
int t_free(char *name);
int receiveAck(int msgtype);
int receiveAckPointer(int *msgtypeP);
