#ifndef HEAPSERVER_DATA
#define HEAPSERVER_DATA

enum modification {
    MAJ_ACCESS_READ,
    MAJ_ACCESS_WRITE,
    MAJ_WAIT_READ,
    MAJ_WAIT_WRITE,
    MAJ_DATA,
    FREE_DATA,
    RELEASE_DATA,
    ADD_CLIENT,
    REMOVE_CLIENT,
    ACK
};

struct replicationData {
    uint8_t modification;
    uint16_t clientId;
    struct heapData *data;
    pthread_mutex_t mutex_server;
    pthread_cond_t cond_server;
};

struct replicationAck {
    uint8_t modification;
    pthread_mutex_t mutex_server;
    pthread_cond_t cond_server;
};

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

    uint8_t readAccessSize;
    struct clientChainRead *readAccess;
    uint8_t writeAccessSize;
    struct clientChainWrite *writeAccess;
    uint8_t readWaitSize;
    struct clientChainRead *readWait;
    pthread_cond_t readCond;

    uint8_t writeWaitSize;
    struct clientChainWrite *writeWait;

    uint8_t serverReadAccessSize;
    struct serverChainRead *serverReadAccess;
    uint8_t serverWriteAccessSize;
    struct serverChainWrite *serverWriteAccess;
    uint8_t serverReadWaitSize;
    struct serverChainRead *serverReadWait;
    pthread_cond_t serverReadCond;

    uint8_t serverWriteWaitSize;
    struct serverChainWrite *serverWriteWait;


    struct heapData *next;
    struct heapData *nextOffset;
};

extern struct heapData **hashTable;
extern struct heapData **hashTableOffset;
extern pthread_mutex_t hashTableMutex;
extern void *theHeap;
extern struct replicationData *rep;
extern struct replicationAck *ack;
extern pthread_key_t id;

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
