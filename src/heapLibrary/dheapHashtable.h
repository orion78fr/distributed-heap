/* pour la taille de la hashtable on choisit un nombre premier,
 * en effet les allocations étant souvent des nombres pairs on
 * veut quand même garder une uniformité dans la répartition des
 * variables dans la hashtable étant donné que celles-ci sont
 * liés à des pointeurs */
#define DHEAP_HASHTABLE_SIZE 257

#include <inttypes.h>

struct dheapVar {
    void *p;
    uint64_t size;
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
