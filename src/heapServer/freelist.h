#ifndef HEAPSERVER_FREELIST
#define HEAPSERVER_FREELIST

struct freeListChain {
    int startOffset;
    int size;
    struct freeListChain *next;
};

extern struct freeListChain *freeList;

int alloc_space(int size);

#endif
