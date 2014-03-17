#ifndef HEAPSERVER_PARAMETERS
#define HEAPSERVER_PARAMETERS

#define PORTSERV 6969
#define HEAPSIZE 1024
#define MAX_CLIENTS 20

struct parameters {
    int port;
    int maxClients;
    int heapSize;
};

extern struct parameters parameters;

int parse_args(int argc, char *argv[]);
void set_defaults();

#endif
