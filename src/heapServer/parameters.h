#ifndef HEAPSERVER_PARAMETERS
#define HEAPSERVER_PARAMETERS

#define PORTSERV 6969
/* 100 Mo */
#define HEAPSIZE 1024*1024*100
#define MAX_CLIENTS 20
#define HASHSIZE 257
#define TIMEOUT 10

struct parameters {
    uint16_t port;
    int maxClients;
    uint64_t heapSize;
    int hashSize;
    uint8_t serverNum;
    char *mainAddress;
    uint16_t mainPort;
    int timeOut;
    uint8_t backup;
    uint8_t idle;
};

extern struct parameters parameters;

int parse_args(int argc, char *argv[]);
void set_defaults();

#endif
