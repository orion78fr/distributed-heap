#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "distributedHeap.h"

int main(int argc, char *args[])
{
    int error, i, j, k, tailleMatriceArg, nbClients;
    int *tailleMatrice = NULL, *ic = NULL, *jc = NULL;
    pid_t *pids;

    if (argc != 3)
        exit(EXIT_FAILURE);

    tailleMatriceArg = atoi(args[1]);
    nbClients = atoi(args[2]);
    pids = malloc(sizeof(pid_t)*nbClients);
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
    sleep(1);

    /* On calcul la matrice avec des forks */
    for (k=0; k<nbClients; k++){
        pid_t p = 0;
        sleep(1);

        if ((p = fork()) == 0){
            int n, icw, jcw;

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
                printf("ERROR T_ACCESS_WRITE: %d\n", error);
                exit(EXIT_FAILURE);
            }
            n = *tailleMatrice;
            if ((error = t_release((void*) tailleMatrice)) != DHEAP_SUCCESS){
                printf("ERROR RELEASE: %d , exiting...\n", error);
                exit(EXIT_FAILURE);
            }

            while(1){
                char name[30];
                int *a = NULL, *b = NULL, *tmp = NULL;
                int result;

                /* On récupère ic et jc */
                if ((error = t_access_write("ic", (void**) &ic)) != DHEAP_SUCCESS){
                    printf("ERROR T_ACCESS_WRITE: %d\n", error);
                    exit(EXIT_FAILURE);
                }
                if ((error = t_access_write("jc", (void**) &jc)) != DHEAP_SUCCESS){
                    printf("ERROR T_ACCESS_WRITE: %d\n", error);
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
                        printf("ERROR T_ACCESS_READ: %d\n", error);
                        exit(EXIT_FAILURE);
                    }
                    sprintf(name, "a%d.%d", icw, i);
                    if ((error = t_access_read(name, (void**) &b)) != DHEAP_SUCCESS){
                        printf("ERROR T_ACCESS_READ: %d\n", error);
                        exit(EXIT_FAILURE);
                    }
                    result += (*a * *b);
                    if ((error = t_release((void*) a)) != DHEAP_SUCCESS){
                        printf("ERROR RELEASE: %d , exiting...\n", error);
                        exit(EXIT_FAILURE);
                    }
                    if ((error = t_release((void*) b)) != DHEAP_SUCCESS){
                        printf("ERROR RELEASE: %d , exiting...\n", error);
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
            exit(EXIT_SUCCESS);
        } else {
            pids[k] = p;
        }
    }

    for (k=0; k<nbClients; k++){
        waitpid(pids[k], NULL, 0);
    }

    printf("------------------------------- MAIN OK -----------------\n");

    free(pids);

    exit(EXIT_SUCCESS);
}
