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
  struct clientChain *next;
};
