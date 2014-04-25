#ifndef HEAPSERVER_DATA
#define HEAPSERVER_DATA

struct clientChainRead {
    uint16_t clientId;
    struct clientChainRead *next;
};

struct clientChainWrite {
    uint16_t clientId;
    struct clientChainWrite *next;
    pthread_cond_t cond;
};

struct serverChainRead {
    uint16_t serverId;
    struct serverChainRead *next;
};

struct serverChainWrite {
    uint16_t serverId;
    struct serverChainWrite *next;
    pthread_cond_t cond;
};

struct heapData {
    pthread_mutex_t mutex;

    char *name;
    uint64_t offset;
    uint64_t size;

    struct clientChainRead *readAccess;
    struct clientChainWrite *writeAccess;

    struct clientChainRead *readWait;
    pthread_cond_t readCond;

    struct clientChainWrite *writeWait;

    struct serverChainRead *serverReadAccess;
    struct serverChainWrite *serverWriteAccess;

    struct serverChainRead *serverReadWait;
    pthread_cond_t serverReadCond;

    struct serverChainWrite *serverWriteWait;


    struct heapData *next;
    struct heapData *nextOffset;
};

extern struct heapData **hashTable;
extern struct heapData **hashTableOffset;
extern pthread_mutex_t hashTableMutex;
extern void *theHeap;

struct heapData *get_data(char *name);
struct heapData *get_data_by_offset(uint64_t offset);
int getHashSum(char *name);
int add_data(char *name, uint64_t size);
void init_data();
int remove_data(char *name);
int acquire_read_lock(struct heapData *data);
void acquire_read_sleep(struct heapData *data, struct clientChainRead *me);
int acquire_write_lock(struct heapData *data);
void acquire_write_sleep(struct heapData *data,
                         struct clientChainWrite *me);
int release_read_lock(struct heapData *data);
int release_write_lock(struct heapData *data);

#endif
