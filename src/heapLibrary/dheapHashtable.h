#define DHEAP_HASHTABLE_SIZE 256

struct dheapVar {
    void *p;
    int size;
    enum rw { DHEAPVAR_READ, DHEAPVAR_WRITE } rw;
    struct dheapVar *next;
};

extern struct dheapVar **dheapHashtable;

void init_hashtable();
void free_hashtable();
int add_var(struct dheapVar *dv);
int getDHTsum(void *p);
int remove_var(void *p);
struct dheapVar* getVarFromPointer(void *p);
