#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "distributedHeap.h"

int main(int argc, char *args[])
{
    int error;
    int *tailleMatrice = NULL;
    int n, ic, jc;

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

    /* On récupère la taille de la matrice */
    if ((error = t_access_read("tailleMatrice", (void**) &tailleMatrice)) != DHEAP_SUCCESS){
        printf("ERROR T_ACCESS_READ tailleMatrice: %d\n", error);
        exit(EXIT_FAILURE);
    }
    n = *tailleMatrice;
    if ((error = t_release((void*) tailleMatrice)) != DHEAP_SUCCESS){
        printf("ERROR RELEAS tailleMatrice: %d , exiting...\n", error);
        exit(EXIT_FAILURE);
    }

    /* Matrice de base */
    printf("========== Matrice de départ =========\n");
    for (ic=0; ic<n; ic++){
        for (jc=0; jc<n; jc++){
            char name[30];
            int *res;
            sprintf(name, "a%d.%d", ic, jc);
            if ((error = t_access_read(name, (void**) &res)) != DHEAP_SUCCESS){
            printf("ERROR T_ACCESS_READ res: %d\n", error);
                exit(EXIT_FAILURE);
            }
            printf("| %d ", *res);
            if ((error = t_release((void*) res)) != DHEAP_SUCCESS){
                printf("ERROR RELEASE res: %d , exiting...\n", error);
                exit(EXIT_FAILURE);
            }
        }
        printf("\n");
    }

    /* Matrice resultat */
    printf("\n\n========== Matrice au carré =========\n");
    for (ic=0; ic<n; ic++){
        for (jc=0; jc<n; jc++){
            char name[30];
            int *res;
            sprintf(name, "r%d.%d", ic, jc);
            if ((error = t_access_read(name, (void**) &res)) != DHEAP_SUCCESS){
            printf("ERROR T_ACCESS_READ res: %d\n", error);
                exit(EXIT_FAILURE);
            }
            printf("| %d ", *res);
            if ((error = t_release((void*) res)) != DHEAP_SUCCESS){
                printf("ERROR RELEASE res: %d , exiting...\n", error);
                exit(EXIT_FAILURE);
            }
        }
        printf("\n");
    }

    /* On ferme la connexion */
    printf("close_data()\n");
    if ((error = close_data()) != DHEAP_SUCCESS){
        printf("Unhandeled error... number: %d\n", error);
    }

    printf("------------------------------- RESULT OK -----------------\n");

    exit(EXIT_SUCCESS);
}
