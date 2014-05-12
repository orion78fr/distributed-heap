#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "distributedHeap.h"

int main(int argc, char *args[])
{
    int error, i, k, nbClients;
    int *tailleMatrice = NULL, *ic = NULL, *jc = NULL;
    int n, icw, jcw;
    int nb_proc;

    nbClients = 15;
    srand(time(NULL));

    MPI_Init(&argc, &args);
    MPI_Comm_size(MPI_COMM_WORLD, &nb_proc);
    MPI_Comm_rank(MPI_COMM_WORLD, &k);

    /* Initialisation de la connexion au tas reparti */
    printf("init_data() (numero client: %d)\n", k);
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

    while(1){
        char name[30];
        int *a = NULL, *b = NULL, *tmp = NULL;
        int result;

        /* On récupère ic et jc */
        if ((error = t_access_write("ic", (void**) &ic)) != DHEAP_SUCCESS){
            printf("ERROR T_ACCESS_WRITE ic: %d\n", error);
            exit(EXIT_FAILURE);
        }
        if ((error = t_access_write("jc", (void**) &jc)) != DHEAP_SUCCESS){
            printf("ERROR T_ACCESS_WRITE jc: %d\n", error);
            exit(EXIT_FAILURE);
        }
        icw = *ic;
        jcw = *jc;

        if (icw == n && jcw == 0){
            goto release;
        }

        (*jc)++;
        if (*jc == n){
            *jc = 0;
            (*ic)++;
        }

release:
        if ((error = t_release((void*) ic)) != DHEAP_SUCCESS){
            printf("ERROR RELEASE: %d , exiting...\n", error);
            exit(EXIT_FAILURE);
        }
        if ((error = t_release((void*) jc)) != DHEAP_SUCCESS){
            printf("ERROR RELEASE: %d , exiting...\n", error);
            exit(EXIT_FAILURE);
        }

        if (icw == n && jcw == 0){
            break;
        }

        result = 0;

        /* On calcul la case */
        for (i=0; i<n; i++){
            sprintf(name, "a%d.%d", i, jcw);
            if ((error = t_access_read(name, (void**) &a)) != DHEAP_SUCCESS){
                printf("ERROR T_ACCESS_READ a: %d\n", error);
                exit(EXIT_FAILURE);
            }
            sprintf(name, "a%d.%d", icw, i);
            if ((error = t_access_read(name, (void**) &b)) != DHEAP_SUCCESS){
                printf("ERROR T_ACCESS_READ b: %d\n", error);
                exit(EXIT_FAILURE);
            }
            result += (*a * *b);
            if ((error = t_release((void*) a)) != DHEAP_SUCCESS){
                printf("ERROR RELEASE a: %d , exiting...\n", error);
                exit(EXIT_FAILURE);
            }
            if ((error = t_release((void*) b)) != DHEAP_SUCCESS){
                printf("ERROR RELEASE b: %d , exiting...\n", error);
                exit(EXIT_FAILURE);
            }
        }

        /* On écrit le résultat */
        sprintf(name, "r%d.%d", icw, jcw);
        if ((error = t_malloc(sizeof(int), name)) != DHEAP_SUCCESS){
            printf("ERROR T_MALLOC: %d -- %s -- icw: %d -- jcw: %d\n", error, name, icw, jcw);
            exit(EXIT_FAILURE);
        }
        if ((error = t_access_write(name, (void**) &tmp)) != DHEAP_SUCCESS){
            printf("ERROR T_ACCESS_WRITE: %d\n", error);
            exit(EXIT_FAILURE);
        }
        *tmp = result;
        if ((error = t_release((void*) tmp)) != DHEAP_SUCCESS){
            printf("ERROR RELEASE: %d , exiting...\n", error);
            exit(EXIT_FAILURE);
        }
        printf("################ RES: %d (fils %d)\n", result, k);

    }

    /* On ferme la connexion */
    printf("close_data() du fils numero %d\n", k);
    if ((error = close_data()) != DHEAP_SUCCESS){
        printf("Unhandeled error... number: %d\n", error);
    }

    printf("------------------------------- CALC OK -----------------\n");

    MPI_Finalize();
    exit(EXIT_SUCCESS);
}
