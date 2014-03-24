#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "distributedHeap.h"

/* Ceci est un exemple utilisant 3 clients :
 * - les deux premiers clients demandent la variable "i" en écriture
 *   si celle-ci n'existe pas, alors un malloc est créé
 *   si celle-ci est vide (0) alors elle est initalisé à 1
 *   si elle existe et n'est pas vide, alors on incrémente de 1
 *       (On incrémente seulement si la valeur est différente de celle précédemment connue)
 *   puis on release et on sleep pendant 1 à 5 secondes (aléatoire)
 * - le troixième client, fais un access_read, affiche la variabe
 *   puis release toute les 1 à 3 secondes (aléatoire) (voir moins pour qu'il s'intercale entre les write)
 * - lorsque la variable atteint 100, on la remet à 1
 */

int main(int argc, char *args[])
{
    /* Initialisation */
    int *i;
    int cli, count, randn, error, error2;
    i = NULL;
    srand(time(NULL));

    cli = atoi(args[1]);
    count = atoi(args[2]);
    if (cli < 0 || cli > 100 || count < 0 || count > 100){
        printf("Usage: %s <number> <maxtick> (with number & maxtick between 0 and 100)\n", args[0]);
        return EXIT_FAILURE;
    }

    printf("---- Welcome in example number 2\n");
    if (cli < 2){
        printf("---- I am a writer\n");
    } else {
        printf("---- I am a reader\n");
    }

    /* Initialisation de la connexion au tas reparti */
    printf("init_data()\n");
    if ((error = init_data()) != DHEAP_SUCCESS){
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

    if (cli < 2){
        /* ---------- WRITER ----------- */
        while (count > 0){
            randn = rand() % 5;
            sleep(randn);

            /* On essaye de récupérer l'acces en écriture */
            if ((error = t_access_write("i", (void*) i)) != DHEAP_SUCCESS){
                printf("ERROR T_ACCESS_WRITE: %d\n", error);
                switch (error){
                    /* Si la variable n'existe pas, on fait le malloc */
                    case DHEAP_ERROR_VAR_DOESNT_EXIST:
                        printf("La variable i n'existe pas, je fais le malloc\n");
                        if ((error2 = t_malloc(sizeof(int), "i")) != DHEAP_SUCCESS){
                            /* Ici, quelqu'un a pu faire le malloc au meme moment */
                            /* TODO: traiter le var already exist */
                            printf("ERROR T_MALLOC: %d\n", error2);
                        }
                        break;
                    default:
                        printf("Unhandeled error T_ACCESS_WRITE... number: %d\n", error);
                        break;
                }
                /* On est dans le cas d'une erreur donc on décrémente le tick et on continue */
                count--;
                continue;
            }

            /* On incrémente ou on réinitialise i */
            if (*i > 100)
                *i = 1;
            else
                *i = *i + 1;
            printf("J'ai incrémenté i: %d\n", *i);

            /* On release i */
            if ((error = t_release((void*) i)) != DHEAP_SUCCESS){
                printf("ERROR RELEASE: %d , exiting...\n", error);
                return EXIT_FAILURE;
            }

            /* On décrémente le tick et on repart */
            count--;
        }
    } else {
        /* ---------- READER ----------- */
        while (count > 0){ 
            randn = rand() % 3;
            sleep(randn);

            /* On récupère l'access en lecture */
            if ((error = t_access_read("i", (void*) i)) != DHEAP_SUCCESS){
                /* Ici peut etre que la variable n'existe pas encore */
                printf("ERROR T_ACCESS_READ: %d\n", error);
                count--;
                continue;
            }
            printf("Je lis %d\n", *i);

            /* On release, on décrémente le tick et on repart */
            if ((error = t_release((void*) i)) != DHEAP_SUCCESS){
                printf("ERROR RELEASE: %d , exiting...\n", error);
                return EXIT_FAILURE;
            }
            count--;
        }
    } 

    /* On ferme la connexion et le programme se termine */
    printf("close_data()\n");
    if ((error = close_data()) != DHEAP_SUCCESS){
        printf("Unhandeled error... number: %d\n", error);
    }

    printf("Success!\n");
    return EXIT_SUCCESS;
}
