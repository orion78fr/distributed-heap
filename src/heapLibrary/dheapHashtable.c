#include "distributedHeap.h"

struct dheapVar **dheapHashtable;

void init_hashtable(){
    int i;
    dheapHashtable = malloc(DHEAP_HASHTABLE_SIZE * sizeof(struct dheapVar *));
    for (i = 0; i < DHEAP_HASHTABLE_SIZE; i++){
        dheapHashtable[i] = NULL;
    }
}

void free_hashtable(){
    int i;
    for (i = 0; i < DHEAP_HASHTABLE_SIZE; i++){
        struct dheapVar *tmpNext1, *tmpNext2;
        tmpNext1 = dheapHashtable[i];
        while (tmpNext1 != NULL){
            tmpNext2 = tmpNext1->next;
            free(tmpNext1);
            tmpNext1 = tmpNext2;
        }
    }
    free(dheapHashtable);
} 
