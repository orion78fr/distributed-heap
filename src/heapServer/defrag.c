#include "common.h"

uint64_t defrag_if_possible(uint64_t size){
    uint64_t taille_restante = 0;
    struct freeListChain *tempFreeList;
    struct heapData *data, *prevData;
    uint64_t oldOffset, newOffset;

    pthread_mutex_lock(&freeListMutex);
    tempFreeList = freeList;
    while(tempFreeList != NULL){
        taille_restante += tempFreeList->size;
        tempFreeList = tempFreeList->next;
    }
    if(taille_restante<size){
        pthread_mutex_unlock(&freeListMutex);
        return -1;
    }

    /* On a assez de taille, on défragmente */
    tempFreeList = freeList;
    while(tempFreeList->size < size){
        data = get_data_by_offset(tempFreeList->startOffset + tempFreeList->size);
        acquire_write_lock(data);
        pthread_mutex_lock(&(data->mutex));

        /* On déplace au début du trou */
        memmove(theHeap+tempFreeList->startOffset, theHeap+data->offset, data->size);
        oldOffset = data->offset;
        data->offset = tempFreeList->startOffset;
        newOffset = data->offset;
        tempFreeList->startOffset += data->size;

        pthread_mutex_lock(&hashTableMutex);
        /* On met a jour la hashTable */
        prevData = NULL;
        data = hashTableOffset[oldOffset%parameters.hashSize];

        while(data != NULL && data->offset != newOffset){
            prevData = data;
            data = data->nextOffset;
        }
        if(data == NULL){
            /* WHAT ???!!!??? */
            return -42;
        }
        /* On retire de l'ancienne */
        if(prevData == NULL){
            hashTableOffset[oldOffset%parameters.hashSize] = data->nextOffset;
        } else {
            prevData->nextOffset = data->nextOffset;
        }
        /* On met dans la nouvelle */
        data->nextOffset = hashTableOffset[newOffset%parameters.hashSize];
        hashTableOffset[newOffset%parameters.hashSize] = data;

        pthread_mutex_unlock(&hashTableMutex);
        pthread_mutex_unlock(&(data->mutex));
        release_write_lock(data);

        if((tempFreeList->startOffset + tempFreeList->size) == tempFreeList->next->startOffset){
            /* On fusionne */
            tempFreeList->size += tempFreeList->next->size;
            tempFreeList->next = tempFreeList->next->next;
            free(tempFreeList->next);
        }
    }

    pthread_mutex_unlock(&freeListMutex);
    return tempFreeList->startOffset;
}
