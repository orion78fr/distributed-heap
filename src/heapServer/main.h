#ifndef HEAPSERVER_MAIN
#define HEAPSERVER_MAIN

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

#define PORTSERV 5500
#define HEAPSIZE 1024
#define MAX_CLIENTS 20

struct heapData{
  pthread_mutex_t mutex;
  char *name;
  int offset;
  struct clientChain *readAccess;
  struct clientChain *writeAccess;
  struct clientChain *readWait;
  struct clientChain *writeWait;
  struct heapData *next;
};

struct clientChain{
  int clientId;
  int sock;
  struct clientChain *next;
};

struct parameters{
	int port;
	int maxClients;
	int heapSize;
} parameters;

// GTU : 256 char pour un message ça me paraît suffisant non?
#define MESSAGE_SIZE 256
enum msgTypes{
	MSG_HEAP_SIZE,
	MSG_ALLOC,
	MSG_ACCESS_READ,
	MSG_ACCESS_WRITE,
	MSG_RELEASE,
	MSG_FREE,
	MSG_ERROR
};

struct message{
	int msgType;
	union{
		int asInteger;
		char asString[MESSAGE_SIZE];
	} content;
};

void *clientThread(void *arg);
int parse_args(int argc, char *argv[]);
int main(int argc, char *argv[]);

#endif
