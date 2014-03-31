#ifndef HEAPSERVER_FREELIST
#define HEAPSERVER_FREELIST

struct freeListChain {
    int startOffset;
    int size;
    struct freeListChain *next;
};

extern struct freeListChain *freeList;
extern pthread_mutex_t freeListMutex;

int alloc_space(int size);
void free_space(int offset, int size);

#endif
