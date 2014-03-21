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

int getDHTsum(void *p){
    int q = (int)p;
    return abs(q)%DHEAP_HASHTABLE_SIZE;
}

int add_var(struct dheapVar *dv){
    int hash;

    hash = getDHTsum(dv->p);

    if (dheapHashtable[hash] == NULL){
        dheapHashtable[hash] = dv;
    } else {
        struct dheapVar *dvtmp;
        dvtmp = dheapHashtable[hash];
        while (dvtmp->next != NULL){
            dvtmp = dvtmp->next;
        }
        dvtmp->next = dv;
    }

    return 0;
}

int remove_var(void *p){
    int hash;
    struct dheapVar *dvtmp, *dvtmpprev;

    hash = getDHTsum(p);
    
    dvtmp = dheapHashtable[hash];
    if (dvtmp == NULL)
        return -1;

    while (dvtmp->p != p && dvtmp->next != NULL){
        dvtmpprev = dvtmp;
        dvtmp = dvtmp->next;
    }

    if (dvtmp->p != p)
        return -1;

    if (dvtmp == dheapHashtable[hash]){
        dheapHashtable[hash] = NULL;
    } else {
        if (dvtmp->next != NULL)
            dvtmpprev->next = dvtmp->next;
        else
            dvtmpprev->next = NULL;
    }
    free(dvtmp);
    
    return 0;
}

struct *dheapVar getVarFromPointer(void *p){
    int hash;
    struct dheapVar *dv;

    hash = getDHTsum(p);

    dv = dheapHashtable[hash];

    if (dv == NULL)
        return NULL;

    while (dv->p != p && dv->next != NULL){
        dv = dv->next;
    }

    if (dv-> != p)
        return NULL;
    else
        return dv;

}
