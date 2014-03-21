#include <stdio.h>
#include <stdlib.h>
#include "distributedHeap.h"

int main(int argc, char *args[])
{
    int *i;
    int error;
    
    printf("Hello World\n");

    if ((error = init_data()) != DHEAP_SUCCESS){
        switch(error){
            case DHEAP_ERROR_CONNECTION:
                printf("DHEAP_ERROR_CONNECTION\n");
                break;
            default:
                printf("Unhandeled error... number: %d\n", error);
        }
    }
    
    if ((error = t_malloc(sizeof(int), "i")) != DHEAP_SUCCESS){

    }

    if ((error = t_access_read("i",(void**) &i)) != DHEAP_SUCCESS){

    }

    if ((error = t_release(i)) != DHEAP_SUCCESS){

    }

    if ((error = t_free("i")) != DHEAP_SUCCESS){

    }

    /* sleep(15); */

    if ((error = close_data()) != DHEAP_SUCCESS){

    }
    
    printf("Success!\n");


    return EXIT_SUCCESS;
}
