#include "common.h"

/**
 * Envoie sur la socket sock les données passées en argument
 * @param sock Socket de communication
 * @param msgType Le type de message (voir messages.h)
 * @param nb Nombre de données à envoyer
 * @param datas Données à envoyer
 * @param tailles Tailles des données à envoyer
 * @return 0 si succès
 */
int send_data(int sock, int msgType, int nb, void **datas, int *tailles){
	int i;
	if(write(sock, (void *) &msgType, sizeof(msgType)) <= 0){
		return -1;
	}
	for(i=0; i<nb; i++){
		if(write(sock, datas[i], tailles[i]) <= 0){
			return -1;
		}
	}
	return 0;
}

/**
 * Reçoit un message sur la socket
 * @param sock Socket de communication
 * @param taille Taille de la donnée à reçevoir
 * @return Pointeur sur la donnée lue, NULL sinon
 */
void *recv_data(int sock, int taille){
	void *data = malloc(taille);
	if(read(sock, data, taille) <= 0){
		free(data);
		return NULL;
	}
	return data;
}