#ifndef memcache_h
#define memcache_h
#include <string>
#include <cstring>
#include <mutex>
#include <unordered_map>
#include "Threadpool.h"

using namespace std;

#define MAX_DATA_LEN 128 * 1024
#define MAX_PAYLOAD_LENGTH ((128 * 1024) + 512)
#define CAPACITY 5000

struct CacheNode {
  string key; // key for the data
  uint16_t flags; // flags associated with the data
  time_t exptime; //  the exp time, not used
  uint64_t bytes; // number of bytes of data
  char *data; // a pointer to the data buffer
  CacheNode *next; // points to the next node in the linkedlist
  CacheNode *prev; // points to the previous node in the linkedlist
  CacheNode() {
    key = "";
    flags = 0;
    exptime = 0;
    data = nullptr;
    prev = nullptr;
    next = nullptr;
    bytes = 0;
  }

  CacheNode(const CacheNode *node) {
    key = node->key;
    flags = node->flags;
    exptime = node->exptime;
    bytes = node->bytes;
    next = nullptr;
    prev = nullptr;
    if (bytes > 0 && node->data != nullptr) {
      data = new char[bytes];
      memcpy(data, node->data, bytes);
    }
  }

  ~CacheNode() {
    if (data) {
      delete[] data;
      data = nullptr;
    }
  }
};

enum CacheStatus {
  Stored,
  NotStored,
  Error,
  ClientError
};

static unordered_map<uint32_t, string> return_str = {
  {Stored, "STORED"},
  {NotStored, "NOT_STORED"},
  {Error, "ERROR"},
  {ClientError, "CLIENT_ERROR"}
};

class Cache {
 public:
  Cache(int size = CAPACITY) {
    size_ = size;
    head_ = new CacheNode();
    tail_ = new CacheNode();
    head_->next = tail_;
    tail_->prev = head_;
  }

  ~Cache() {
    CacheNode *itr = head_;
    while (itr != nullptr) {
      CacheNode *temp = itr;
      itr = itr->next;
      delete temp;
    }
    cache_map_.clear();
  }

  CacheStatus addNewEntry(CacheNode *entry);

  CacheNode* getEntry(string key);

  size_t NumEntries();

  inline uint32_t Capacity() { return size_; }

 private:
  void DeleteLastNode();

  uint32_t size_; // size of the cache
  unordered_map<string, CacheNode*> cache_map_; // map to store the key and associated CacheNode pointer
  CacheNode *head_; // the head of the linkedlist
  CacheNode *tail_; // tail of the linkedlist
  mutex cache_mutex_; // mutex to provide synchronization
};

int GetDataFromClient(int socket, ThreadPool *pool, Cache* memcache);
void ParseDataFromClient(string s, int socket, Cache* memcache, int total_bytes);
string ParseSetCmd(string s, Cache* memcache, int total_bytes);
string ParseGetCmd(string s, Cache* memcache);
#endif //memcache_h
