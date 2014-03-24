#ifndef HEAPSERVER_DATA
#define HEAPSERVER_DATA

struct clientChainRead {
    int clientId;
    struct clientChain *next;
};

struct clientChainWrite {
    int clientId;
    struct clientChain *next;
    pthread_cond_t cond;
};

struct heapData {
    pthread_mutex_t mutex;
    
    char *name;
    int offset;
    int size;
    
    struct clientChainRead *readAccess;
    struct clientChainWrite *writeAccess;
    
    struct clientChainRead *readWait;
    pthread_cond_t readCond;
    
    struct clientChainWrite *writeWait;
    
    struct heapData *next;
};

extern struct heapData **hashTable;
extern pthread_mutex_t hashTableMutex;
extern void* theHeap;

struct heapData *get_data(char *name);
int getHashSum(char *name);
int add_data(char *name, int size);
void init_data();
int remove_data(char *name);
int acquire_read_lock(char *name);
void acquire_read_sleep(struct heapData *data, struct clientChainRead *me);

#endif
