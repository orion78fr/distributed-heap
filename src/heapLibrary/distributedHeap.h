#define _XOPEN_SOURCE 700

/* Headers POSIX */
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

#define DHEAP_SERVER_ADDRESS "localhost"
#define DHEAP_SERVER_PORT 6969

enum errorCodes {
    /* Codes d'erreur venant du serveur */
    DHEAP_ERROR_SERVER_FULL,
    DHEAP_ERROR_HEAP_FULL,
    DHEAP_ERROR_VAR_DONT_EXIST,
    DHEAP_ERROR_NOT_LOCKED,

    /* Codes d'erreur propres au client */
    DHEAP_ERROR_CONNECTION
};


int init_data();
int close_data();
int t_malloc(int size, char *name);
int t_access_read(char *name, void *p);
int t_access_write(char *name, void *p);
int t_release(void *p);
int t_free(char *name);

struct heapInfo {
    int heapSize;
    char *heapStart;
    int sock;
};

extern struct heapInfo heapInfo;
