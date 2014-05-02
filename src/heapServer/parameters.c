#include "common.h"

struct parameters parameters;

/**
 * Parse les arguments du programme
 * @param argc Argc du main
 * @param argv Argv du main
 * @return 0 si succès, le nombre d'erreur sinon
 * @see http://www.gnu.org/software/libc/manual/html_node/Getopt.html#Getopt
 */
int parse_args(int argc, char *argv[])
{
    int c, option_index, returnValue = 0;

    set_defaults();

    static struct option long_options[] = {
        {"port", required_argument, 0, 'p'},
        {"maxClient", required_argument, 0, 'n'},
        {"heapSize", required_argument, 0, 's'},
        {"hashSize", required_argument, 0, 'h'},
        {"timeOut", required_argument, 0, 't'},
        {"mainAddress", required_argument, 0, 'a'},
        {"mainPort", required_argument, 0, 'm'},
        {0, 0, 0, 0}
    };

    while ((c =
            getopt_long(argc, argv, "p:n:s:h:t:a:m:", long_options,
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
        case 'h':
            parameters.hashSize = atoi(optarg);
            break;
        case 't':
            parameters.timeOut = atoi(optarg);
            break;
        case 'a':
            parameters.mainAddress = optarg;
            //strcpy(parameters.mainAddress, optarg);
            break;
        case 'm':
            parameters.mainPort = atoi(optarg);
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

/**
 * Initialise les valeurs par défaut de parameters
 */
void set_defaults()
{
    parameters.port = PORTSERV;
    parameters.maxClients = MAX_CLIENTS;
    parameters.heapSize = HEAPSIZE;
    parameters.hashSize = HASHSIZE;
    parameters.serverNum = 0;
    parameters.mainAddress = "";
    parameters.mainPort = 0;
    parameters.timeOut = TIMEOUT;
}
