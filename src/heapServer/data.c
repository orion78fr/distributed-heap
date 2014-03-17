#include "common.h"

struct heapData *hashTable[HASHSIZE];

struct heapData *get_data(char *name)
{
    int sum = getHashSum(name);
    struct *heapData data = hashTable[sum];
    
    while(data != NULL){
        if(strcmp(name, data->name) == 0){
            return data;
        } else {
            data = data->next;
        }
    }

    return NULL;
}

int getHashSum(char *name){
    int sum = 0;
    int size = strlen(name);
    int i;

    for(i = 0; i < size; i++){
        sum += name[i];
    }

    return sum % HASHSIZE;
}

int add_data(char *name, int size){
    int sum = getHashSum(name);
    
    if(get_data(name) != NULL){
        return -1;
    } else {
        struct heapData *newData = malloc(sizeof(struct heapData));
        newdata->name = name;
        newdata->size = size;
        newdata->readAccess = NULL;
        newdata->writeAccess = NULL;
        newdata->readWait = NULL;
        newdata->writeWait = NULL;
        
        newdata->offset = alloc_space(size);
        
        newdata->next = hashTable[sum];
        hashTable[sum] = newdata;
    }
    return 0;
}
