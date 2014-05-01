#include <stdio.h>
#include <stdlib.h>
#include "distributedHeap.h"

/* Ceci est un exemple simple d'utilisation du tas réparti
 * écrivant simplement une variable, avant de la lire quelques
 * secondes plus tard */

int main(int argc, char *args[])
{
    int *a, *c, *e, *f;
    int error;


    printf("Welcome in example number 3\n");
    printf("HeapSize must be 20\n");

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

    printf("--------------------------- t_malloc(4, 'a')\n");
    if ((error = t_malloc(4, "a")) != DHEAP_SUCCESS){
        printf("Unhandeled error... number: %d\n", error);
        return EXIT_FAILURE;
    }
    printf("--------------------------- t_malloc(4, 'b')\n");
    if ((error = t_malloc(4, "b")) != DHEAP_SUCCESS){
        printf("Unhandeled error... number: %d\n", error);
        return EXIT_FAILURE;
    }
    printf("--------------------------- t_malloc(4, 'c')\n");
    if ((error = t_malloc(4, "c")) != DHEAP_SUCCESS){
        printf("Unhandeled error... number: %d\n", error);
        return EXIT_FAILURE;
    }
    printf("--------------------------- t_malloc(4, 'd')\n");
    if ((error = t_malloc(4, "d")) != DHEAP_SUCCESS){
        printf("Unhandeled error... number: %d\n", error);
        return EXIT_FAILURE;
    }
    printf("--------------------------- t_malloc(4, 'e')\n");
    if ((error = t_malloc(4, "e")) != DHEAP_SUCCESS){
        printf("Unhandeled error... number: %d\n", error);
        return EXIT_FAILURE;
    }

    printf("--------------------------- t_access_write('a', (void**) &a))\n");
    if ((error = t_access_write("a",(void**) &a)) != DHEAP_SUCCESS){
        printf("Unhandeled error... number: %d\n", error);
        return EXIT_FAILURE;
    }
    printf("--------------------------- t_access_write('c', (void**) &c))\n");
    if ((error = t_access_write("c",(void**) &c)) != DHEAP_SUCCESS){
        printf("Unhandeled error... number: %d\n", error);
        return EXIT_FAILURE;
    }
    printf("--------------------------- t_access_write('e', (void**) &e))\n");
    if ((error = t_access_write("e",(void**) &e)) != DHEAP_SUCCESS){
        printf("Unhandeled error... number: %d\n", error);
        return EXIT_FAILURE;
    }

    *a = 1;
    *c = 2;
    *e = 3;
    printf("--------------------------- Numéros attribués...\n");

    printf("--------------------------- t_release(a)\n");
    if ((error = t_release(a)) != DHEAP_SUCCESS){
        printf("Unhandeled error... number: %d\n", error);
        return EXIT_FAILURE;
    }
    printf("--------------------------- t_release(c)\n");
    if ((error = t_release(c)) != DHEAP_SUCCESS){
        printf("Unhandeled error... number: %d\n", error);
        return EXIT_FAILURE;
    }
    printf("--------------------------- t_release(e)\n");
    if ((error = t_release(e)) != DHEAP_SUCCESS){
        printf("Unhandeled error... number: %d\n", error);
        return EXIT_FAILURE;
    }

    printf("--------------------------- t_free('b')\n");
    if ((error = t_free("b")) != DHEAP_SUCCESS){
        printf("Unhandeled error... number: %d\n", error);
        return EXIT_FAILURE;
    }
    printf("--------------------------- t_free('d')\n");
    if ((error = t_free("d")) != DHEAP_SUCCESS){
        printf("Unhandeled error... number: %d\n", error);
        return EXIT_FAILURE;
    }

    printf("--------------------------- t_malloc(8, 'f')\n");
    if ((error = t_malloc(8, "f")) != DHEAP_SUCCESS){
        printf("Unhandeled error... number: %d\n", error);
        return EXIT_FAILURE;
    }

    printf("--------------------------- t_access_read('f', (void**) &f))\n");
    if ((error = t_access_read("f",(void**) &f)) != DHEAP_SUCCESS){
        printf("Unhandeled error... number: %d\n", error);
        return EXIT_FAILURE;
    }


    printf("--------------------------- t_access_read('a', (void**) &a))\n");
    if ((error = t_access_read("a",(void**) &a)) != DHEAP_SUCCESS){
        printf("Unhandeled error... number: %d\n", error);
        return EXIT_FAILURE;
    }
    printf("--------------------------- t_access_read('c', (void**) &c))\n");
    if ((error = t_access_read("c",(void**) &c)) != DHEAP_SUCCESS){
        printf("Unhandeled error... number: %d\n", error);
        return EXIT_FAILURE;
    }
    printf("--------------------------- t_access_read('e', (void**) &e))\n");
    if ((error = t_access_read("e",(void**) &e)) != DHEAP_SUCCESS){
        printf("Unhandeled error... number: %d\n", error);
        return EXIT_FAILURE;
    }

    printf("A: %d\n", *a);
    printf("C: %d\n", *c);
    printf("E: %d\n", *e);

    printf("--------------------------- t_release(a)\n");
    if ((error = t_release(a)) != DHEAP_SUCCESS){
        printf("Unhandeled error... number: %d\n", error);
        return EXIT_FAILURE;
    }
    printf("--------------------------- t_release(c)\n");
    if ((error = t_release(c)) != DHEAP_SUCCESS){
        printf("Unhandeled error... number: %d\n", error);
        return EXIT_FAILURE;
    }
    printf("--------------------------- t_release(e)\n");
    if ((error = t_release(e)) != DHEAP_SUCCESS){
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
