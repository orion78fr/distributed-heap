#ifndef HEAPSERVER_FREELIST
#define HEAPSERVER_FREELIST

struct freeListChain {
    uint64_t startOffset;
    uint64_t size;
    struct freeListChain *next;
};

extern struct freeListChain *freeList;
extern pthread_mutex_t freeListMutex;

uint64_t alloc_space(uint64_t size);
void free_space(uint64_t offset, uint64_t size);
uint64_t defrag_if_possible(uint64_t size);

#endif
