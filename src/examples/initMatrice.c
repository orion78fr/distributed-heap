#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "distributedHeap.h"

int main(int argc, char *args[])
{
    int error, i, j, tailleMatriceArg;
    int *tailleMatrice = NULL, *ic = NULL, *jc = NULL;

    if (argc != 2)
        exit(EXIT_FAILURE);

    tailleMatriceArg = atoi(args[1]);
    srand(time(NULL));

    /* Initialisation de la connexion au tas reparti */
    printf("init_data()\n");
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

    /* Taille de la matrice */
    if ((error = t_malloc(sizeof(int), "tailleMatrice")) != DHEAP_SUCCESS){
        printf("ERROR T_MALLOC: %d\n", error);
        exit(EXIT_FAILURE);
    }
    if ((error = t_access_write("tailleMatrice", (void**) &tailleMatrice)) != DHEAP_SUCCESS){
        printf("ERROR T_ACCESS_WRITE: %d\n", error);
        exit(EXIT_FAILURE);
    }
    *tailleMatrice = tailleMatriceArg;
    if ((error = t_release((void*) tailleMatrice)) != DHEAP_SUCCESS){
        printf("ERROR RELEASE: %d , exiting...\n", error);
        exit(EXIT_FAILURE);
    }

    /* Matrice à calculer */
    for (i=0; i<tailleMatriceArg; i++){
        for (j=0; j<tailleMatriceArg; j++){
            char name[30];
            int *a = NULL;
            sprintf(name, "a%d.%d", i, j);
            if ((error = t_malloc(sizeof(int), name)) != DHEAP_SUCCESS){
                printf("ERROR T_MALLOC: %d\n", error);
                exit(EXIT_FAILURE);
            }
            if ((error = t_access_write(name, (void**) &a)) != DHEAP_SUCCESS){
                printf("ERROR T_ACCESS_WRITE: %d\n", error);
                exit(EXIT_FAILURE);
            }
            *a = rand() % 255;
            if ((error = t_release((void*) a)) != DHEAP_SUCCESS){
                printf("ERROR RELEASE: %d , exiting...\n", error);
                exit(EXIT_FAILURE);
            }

        }
    }

    /* On alloue le x et y courants */
    if ((error = t_malloc(sizeof(int), "ic")) != DHEAP_SUCCESS){
        printf("ERROR T_MALLOC: %d\n", error);
        exit(EXIT_FAILURE);
    }
    if ((error = t_access_write("ic", (void**) &ic)) != DHEAP_SUCCESS){
        printf("ERROR T_ACCESS_WRITE: %d\n", error);
        exit(EXIT_FAILURE);
    }
    *ic = 0;
    if ((error = t_release((void*) ic)) != DHEAP_SUCCESS){
        printf("ERROR RELEASE: %d , exiting...\n", error);
        exit(EXIT_FAILURE);
    }
    if ((error = t_malloc(sizeof(int), "jc")) != DHEAP_SUCCESS){
        printf("ERROR T_MALLOC: %d\n", error);
        exit(EXIT_FAILURE);
    }
    if ((error = t_access_write("jc", (void**) &jc)) != DHEAP_SUCCESS){
        printf("ERROR T_ACCESS_WRITE: %d\n", error);
        exit(EXIT_FAILURE);
    }
    *jc = 0;
    if ((error = t_release((void*) jc)) != DHEAP_SUCCESS){
        printf("ERROR RELEASE: %d , exiting...\n", error);
        exit(EXIT_FAILURE);
    }

    /* On ferme la connexion */
    printf("close_data() du init\n");
    if ((error = close_data()) != DHEAP_SUCCESS){
        printf("Unhandeled error... number: %d\n", error);
    }

    printf("------------------------------- INIT OK -----------------\n");

    exit(EXIT_SUCCESS);
}