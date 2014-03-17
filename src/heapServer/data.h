#ifndef HEAPSERVER_DATA
#define HEAPSERVER_DATA

struct heapData {
    pthread_mutex_t mutex;
    char *name;
    int offset;
    struct clientChain *readAccess;
    struct clientChain *writeAccess;
    struct clientChain *readWait;
    struct clientChain *writeWait;
    struct heapData *next;
};

#endif
