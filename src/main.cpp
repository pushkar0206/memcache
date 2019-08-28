#include <cstdio>
#include <stdlib.h>
#include <string.h>

#include "mylib.h"
#include "config.h"
#include "memcache.h"
#include "memserver.h"

#define PORT "11211"   // port we're listening on

int main(void)
{
  std::unique_ptr<ThreadPool> pool;
  std::unique_ptr<Cache> memcache;
  std::unique_ptr<CacheServer> memserver;
  
  // create a threadpool
  pool = std::make_unique<ThreadPool>(12);
  if (pool.get() == nullptr) {
    printf("Failed to create threadpool\n");
    return 0;
  }  
  printf("Creating threadpool of size 12\n");
  pool->init();

  // create the cache object
  memcache = std::make_unique<Cache>();
  if (memcache.get() == nullptr) {
    printf("Failed to create cache\n");
    return 0;
  }
  
  // create the server
  memserver = std::make_unique<CacheServer>(std::string(PORT), pool.get(), memcache.get());
  if (memserver.get() == nullptr) {
    printf("Failed to create server\n");
    return 0;
  }
  
  // initialize the server
  if (memserver->init() < 0) {
    printf("Failed to start the listener service\n");
    return 0;
  }
  
  // wait forever for new connections and receive data from existing connections
  memserver->WaitForClientRequests();

  return 0;
}
