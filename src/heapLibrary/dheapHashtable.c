#include "dheapHashtable.h"

/* TODO: Headers POSIX à mettre dans un common */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


/* Hashtable:
 * cette hashtable permet de garder en mémoire la taille et la
 * localisation des différentes variables auxquelles on a acces
 */


struct dheapVar **dheapHashtable;

/**
 * Initialisation de la hashtable
 */
void init_hashtable(){
    int i;
    dheapHashtable = malloc(DHEAP_HASHTABLE_SIZE * sizeof(struct dheapVar *));
    for (i = 0; i < DHEAP_HASHTABLE_SIZE; i++){
        dheapHashtable[i] = NULL;
    }
}

/**
 * Libère les données liées à la hashtable
 */
void free_hashtable(){
    int i;
    for (i = 0; i < DHEAP_HASHTABLE_SIZE; i++){
        struct dheapVar *tmpNext1, *tmpNext2;
        tmpNext1 = dheapHashtable[i];
        while (tmpNext1 != NULL){
            tmpNext2 = tmpNext1->next;
            free(tmpNext1);
            tmpNext1 = tmpNext2;
        }
    }
    free(dheapHashtable);
    dheapHashtable = NULL;
}

/**
 * Génère un hash lié à un pointeur donné
 * @param pointeur à hasher
 * @return hash
 */
int getDHTsum(void *p){
    int q = (intptr_t)p;
    return abs(q)%DHEAP_HASHTABLE_SIZE;
}

/**
 * Ajoute une variable de type dheapVar dans la hashtable
 * @param struct dheapVar à ajouter
 * @return 0 si succès
 */
int add_var(struct dheapVar *dv){
    int hash;

    hash = getDHTsum(dv->p);

    if (dheapHashtable[hash] == NULL){
        dheapHashtable[hash] = dv;
    } else {
        struct dheapVar *dvtmp;
        dvtmp = dheapHashtable[hash];
        while (dvtmp->next != NULL){
            dvtmp = dvtmp->next;
        }
        dvtmp->next = dv;
    }

    return 0;
}

/**
 * Retire une variable dheapVar lié à un pointeur donné de la hashtable
 * @param pointeur d'une donnée du tas dont on veut supprimer la dheapVar
 * @return 0 si succès, -1 si une erreur est survenue
 */
int remove_var(void *p){
    int hash;
    struct dheapVar *dvtmp, *dvtmpprev;

    hash = getDHTsum(p);
    
    dvtmp = dheapHashtable[hash];
    if (dvtmp == NULL)
        return -1;

    while (dvtmp->p != p && dvtmp->next != NULL){
        dvtmpprev = dvtmp;
        dvtmp = dvtmp->next;
    }

    if (dvtmp->p != p)
        return -1;

    if (dvtmp == dheapHashtable[hash]){
        if (dvtmp->next != NULL)
            dheapHashtable[hash] = dvtmp->next;
        else
            dheapHashtable[hash] = NULL;
    } else {
        if (dvtmp->next != NULL)
            dvtmpprev->next = dvtmp->next;
        else
            dvtmpprev->next = NULL;
    }
    free(dvtmp);
    
    return 0;
}

/**
 * Recherche une variable dheapVar à partir d'un pointeur d'une donnée du tas
 * @param pointeur d'une donnée du tas
 * @return struct dheapVar correspondant au pointeur, NULL sinon
 */
struct dheapVar* getVarFromPointer(void *p){
    int hash;
    struct dheapVar *dv;

    hash = getDHTsum(p);

    dv = dheapHashtable[hash];

    if (dv == NULL)
        return NULL;

    while (dv->p != p && dv->next != NULL){
        dv = dv->next;
    }

    if (dv->p != p)
        return NULL;
    else
        return dv;

}
