#include <stdio.h>
#include <stlib.h>
#include "distributedHeap.h"

int main(char *args[])
{
    int *i;
    int error;
    
    printf("Hello World\n");

    if ((error = init_data()) != 0){
        switch(error){
            case DHEAP_SERVER_FULL:
                break;
            default:
                printf("Unhandeled error... number: %d\n", error);
        }
    }
    
    if ((error = t_malloc(sizeof(int), "i")) != 0){

    }

    if ((error = t_read_access("i", &i)) != 0){

    }

    if ((error = t_release(i)) != 0){

    }

    if (error = (t_free("i")) != 0){

    }

    if ((error = close_data()) != 0){

    }

    return EXIT_SUCCESS;
}
