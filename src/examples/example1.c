#include <stdio.h>
#include <stdlib.h>
#include "distributedHeap.h"

/* Ceci est un exemple simple d'utilisation du tas réparti
 * écrivant simplement une variable, avant de la lire quelques
 * secondes plus tard */

int main(int argc, char *args[])
{
    int *i, *j;
    int error;

    i = NULL;
    j = NULL;

    printf("Welcome in example number 1\n");

    printf("--------------------------- init_data()\n");
    if ((error = init_data("127.0.1.1", 6969)) != DHEAP_SUCCESS){
        switch(error){
            case DHEAP_ERROR_CONNECTION:
                printf("DHEAP_ERROR_CONNECTION\n");
                return EXIT_FAILURE;
                break;
            default:
                printf("Unhandeled error... number: %d\n", error);
                return EXIT_FAILURE;
        }
    }

    printf("--------------------------- t_malloc(sizeof(int), 'i')\n");
    if ((error = t_malloc(sizeof(int), "i")) != DHEAP_SUCCESS){
        printf("Unhandeled error... number: %d\n", error);
        return EXIT_FAILURE;
    }

    printf("--------------------------- t_access_write('i', (void**) &i))\n");
    if ((error = t_access_write("i",(void**) &i)) != DHEAP_SUCCESS){
        printf("Unhandeled error... number: %d\n", error);
        return EXIT_FAILURE;
    }

    *i = 4;
    printf("--------------------------- i devient localement 4 : %d\n", *i);

    printf("--------------------------- t_release(i)\n");
    if ((error = t_release(i)) != DHEAP_SUCCESS){
        printf("Unhandeled error... number: %d\n", error);
        return EXIT_FAILURE;
    }

    printf("//sleep(4)\n");
    /* sleep(4); */

    printf("t_access(read('i', (void*) j))\n");
    if ((error = t_access_read("i",(void**) &j)) != DHEAP_SUCCESS){
        printf("Unhandeled error... number: %d\n", error);
        return EXIT_FAILURE;
    }

    printf("Voici le i qu'on lit: %d\n", *j);

    printf("t_free('i')\n");
    if ((error = t_free("i")) != DHEAP_SUCCESS){
        printf("Unhandeled error... number: %d\n", error);
        return EXIT_FAILURE;
    }

    printf("close_data()\n");
    if ((error = close_data()) != DHEAP_SUCCESS){
        printf("Unhandeled error... number: %d\n", error);
        return EXIT_FAILURE;
    }

    printf("Success!\n");
    return EXIT_SUCCESS;
}
