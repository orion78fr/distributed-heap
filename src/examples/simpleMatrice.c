#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *args[])
{
    int i, j, k;
    int tailleMatrice;
    int **matrice, **result;

    if (argc != 2)
        exit(EXIT_FAILURE);

    tailleMatrice = atoi(args[1]);
    matrice = (int**)malloc(sizeof(int*)*tailleMatrice);
    result = (int**)malloc(sizeof(int*)*tailleMatrice);
    for (i=0; i<tailleMatrice; i++){
        matrice[i] = (int*)malloc(sizeof(int)*tailleMatrice);
        result[i] = (int*)malloc(sizeof(int)*tailleMatrice);
    }
    srand(time(NULL));

    /* Matrice à calculer */
    for (i=0; i<tailleMatrice; i++){
        for (j=0; j<tailleMatrice; j++){
            matrice[i][j] = rand() % 255;
        }
    }

    /* calcul */
    for (i=0; i<tailleMatrice; i++){
        for (j=0; j<tailleMatrice; j++){
            int somme = 0;
            for (k=0; k<tailleMatrice; k++){
                somme = somme + matrice[i][k]*matrice[k][j];
            }
            result[i][j] = somme;
        }
    }

    printf("============= matrice de base ============\n");
    for (i=0; i<tailleMatrice; i++){
        for (j=0; j<tailleMatrice; j++){
            printf("| %d ", matrice[i][j]);
        }
        printf("\n");
    }

    printf("\n\n============= matrice résultat ============\n");
    for (i=0; i<tailleMatrice; i++){
        for (j=0; j<tailleMatrice; j++){
            printf("| %d ", result[i][j]);
        }
        printf("\n");
    }

    free(matrice);
    free(result);

    printf("------------------------------- Calcul OK -----------------\n");

    exit(EXIT_SUCCESS);
}