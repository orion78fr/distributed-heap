#ifndef HEAPSERVER_PARAMETERS
#define HEAPSERVER_PARAMETERS

#define PORTSERV 6969
#define HEAPSIZE 1024
#define MAX_CLIENTS 20
#define HASHSIZE 257

struct parameters {
    int port;
    int maxClients;
    uint64_t heapSize;
    int hashSize;
    uint8_t serverNum;
};

extern struct parameters parameters;

int parse_args(int argc, char *argv[]);
void set_defaults();

#endif
