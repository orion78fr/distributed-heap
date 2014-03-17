#ifndef HEAPSERVER_DATA
#define HEAPSERVER_DATA

#define HASHSIZE 256

struct heapData {
    pthread_mutex_t mutex;
    char *name;
    int offset;
    int size;
    struct clientChain *readAccess;
    struct clientChain *writeAccess;
    struct clientChain *readWait;
    struct clientChain *writeWait;
    struct heapData *next;
};

extern struct heapData *hashTable[];

struct heapData *get_data(char *name);
int getHashSum(char *name);
int add_data(char *name, int size);
void init_data();
int remove_data(char *name);

#endif
