#include "common.h"

/**
 * Envoie sur la socket sock les données passées en argument
 * @param sock Socket de communication
 * @param msgType Le type de message (voir messages.h)
 * @param nb Nombre de données à envoyer
 * @param ... les structures dataSend à envoyer
 * @return 0 si succès
 */
int send_data(int sock, int msgType, int nb, ...){
    va_list ap;
    int i;
    DS snd = {0,NULL};

    if(write(sock, &msgType, sizeof(msgType)) <= 0){
        return -1;
    }

    va_start(ap, nb);

    for(i=0;i<nb;i++){
        snd = va_arg(ap, DS);
        if(write(sock, snd.data, snd.taille) <= 0){
            return -1;
        }
    }

    va_end(ap);

    return 0;
}
