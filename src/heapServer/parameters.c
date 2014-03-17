#include "common.h"

struct parameters parameters;

/**
 * Parse les arguments du programme
 * @param argc Argc du main
 * @param argv Argv du main
 * @return 0 si succès, le nombre d'erreur sinon
 */
int parse_args(int argc, char *argv[])
{
    int c, option_index, returnValue = 0;

    set_defaults();

    static struct option long_options[] = {
	{"port", required_argument, 0, 'p'},
	{"maxClient", required_argument, 0, 'n'},
	{"heapSize", required_argument, 0, 's'},
	{0, 0, 0, 0}
    };

    while ((c =
	    getopt_long(argc, argv, "p:n:s:", long_options,
			&option_index)) != -1) {
	switch (c) {
	case 0:
	    /* Flag option */
	    break;
	case 'p':
	    parameters.port = atoi(optarg);
	    break;
	case 'n':
	    parameters.maxClients = atoi(optarg);
	    break;
	case 's':
	    parameters.heapSize = atoi(optarg);
	    break;
	case '?':
	    /* Erreur, déjà affiché par getopt */
	    returnValue++;
	    break;
	default:
	    returnValue++;
	    abort();
	}
    }
    return returnValue;
}

void set_defaults()
{
    parameters.port = PORTSERV;
    parameters.maxClients = MAX_CLIENTS;
    parameters.heapSize = HEAPSIZE;
}
