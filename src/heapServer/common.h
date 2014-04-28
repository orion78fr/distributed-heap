#ifndef HEAPSERVER_COMMON
#define HEAPSERVER_COMMON

/* GTU : Pour simplifier on va faire les includes de tout les .h dans tout les
 * .c, du moment que ça ne pose pas de conflits (Cela allonge la compilation
 * mais pas l'exécution) */

#define _XOPEN_SOURCE 700

/* Headers POSIX */
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
#include <stdarg.h>
#include <inttypes.h>

/* Headers perso */
#include "data.h"
#include "parameters.h"
#include "clients.h"
#include "servers.h"
#include "messages.h"
#include "error.h"
#include "freelist.h"

#endif
