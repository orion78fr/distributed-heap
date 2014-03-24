#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <getopt.h>
#include <arpa/inet.h>

#include "dheapHashtable.h"

/* TODO: Ajouter des if pour gérer les erreurs et rendre le test automatique */

int main(int argc, char **args){
    struct dheapVar *dv, *dv2, *dvb, *dvb2, *dvc, *dvc2;
    
    printf("Tests for the client Hashtable:\n");
    
    init_hashtable();
    
    printf("Le pointeur est %d et son sum est %d\n", 1, getDHTsum((void*)1));
    printf("Le pointeur est %d et son sum est %d\n", 2, getDHTsum((void*)2));
    printf("Le pointeur est %d et son sum est %d\n", 257, getDHTsum((void*)257));

    /* On alloue le premier */
    dv = malloc(sizeof(struct dheapVar));
    dv->p = (void*)1;
    dv->size = 4;
    dv->rw = DHEAPVAR_READ;
    dv->next = NULL;

    /* On alloue le deuxième */
    dvb = malloc(sizeof(struct dheapVar));
    dvb->p = (void*)2;
    dvb->size = 8;
    dvb->rw = DHEAPVAR_WRITE;
    dvb->next = NULL;

    /* On alloue le troixième */
    dvc = malloc(sizeof(struct dheapVar));
    dvc->p = (void*)257;
    dvc->size = 6;
    dvc->rw = DHEAPVAR_READ;
    dvc->next = NULL;

    printf("On fait les add_var()\n");
    add_var(dv);
    add_var(dvb);
    add_var(dvc);

    printf("On getVarFromPointer()\n");
    dv2 = getVarFromPointer((void*)1);
    dvb2 = getVarFromPointer((void*)2);
    dvc2 = getVarFromPointer((void*)257);

    printf("On affiche la valeur de 1: %d, sa taille: %d, son enum: %d\n", ((int)(dv2->p)), dv2->size, dv2->rw);
    printf("On affiche la valeur de 2: %d, sa taille: %d, son enum: %d\n", ((int)(dvb2->p)), dvb2->size, dvb2->rw);
    printf("On affiche la valeur de 256: %d, sa taille: %d, son enum: %d\n", ((int)(dvc2->p)), dvc2->size, dvc2->rw);

    printf("On free la hashtable\n");

    free_hashtable();

    return EXIT_SUCCESS;
}
