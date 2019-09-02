#ifndef memserver_h
#define memserver_h

#include <sys/select.h>
#include "memcache.h"
#include "Threadpool.h"
using namespace std;

class CacheServer {
 public:
  CacheServer() {
  } 
  CacheServer(string port, ThreadPool* pool, Cache* memcache) {
    port_ = port;
    pool_ = pool;
    memcache_ = memcache;
  }
  int init();
  void WaitForClientRequests();
 private:
  void *get_in_server_addr(struct sockaddr *sa);
  int GetData(int socket);
  fd_set master_;
  fd_set read_fds_;
  int fdmax_;
  string port_;
  int listener_;
  ThreadPool *pool_;
  Cache *memcache_; 
};
#endif
