#include <sys/socket.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <ctype.h>
#include <vector>
#include "memcache.h"

using namespace std;

/* This method deletes the last entry in the linked list and
 * deletes the corresponding entry in the map. i.e., it evicts
 * the least recently used entry
 */
void Cache::DeleteLastNode() {
  CacheNode* temp = tail_->prev;
  if (temp == nullptr) {
    return;
  }

  if (temp == head_) {
    return;
  }

  tail_->prev = temp->prev;
  temp->prev->next = tail_;

  if (cache_map_.find(temp->key) == cache_map_.end()) {
    printf("ERROR: cannot delete last node\n");
    return;
  }

  auto itr = cache_map_.find(temp->key);
  cache_map_.erase(itr);
  delete temp;
}

/* This method add a new entry if the key is not already 
 * present in the map. If the key is present it updates the 
 * data with the new data 
 * The eviction algorithm being used is LRU. Hence every new
 * addition to the map causes the associated linkedlist entry to
 * be brought to the front of the list. If the total entries
 * exceed the capacity, then the last entry in the list is deleted
 * to make room for the new entry
 * @param: The CacheNode to be added to the map
 * @return: the status of the operation
 */
CacheStatus Cache::addNewEntry(CacheNode *entry) {
  if (entry == nullptr) {
    return Error; 
  }
  unique_lock<mutex> lock(cache_mutex_);
  // if the entry is already present
  if (cache_map_.find(entry->key) != cache_map_.end()) {
    // update and bring the entry to the head of the list
    CacheNode *node = cache_map_[entry->key];
    node->flags = entry->flags;
    if (node->data) {
      delete node->data;
      node->data = nullptr;
    }
    node->bytes = entry->bytes;
    node->data = new char[entry->bytes];
    memcpy(node->data, entry->data, entry->bytes);
    if (node != head_->next) {
      node->prev->next = node->next;
      node->next->prev = node->prev;
      node->next = head_->next;
      head_->next->prev = node;
      head_->next = node; 
    }
    // delete the entry that was created by the caller
    delete entry;
    return Stored;    
  }

  if (cache_map_.size() + 1 > size_) {
    DeleteLastNode();
  }

  entry->next = head_->next;
  entry->prev = head_;
  head_->next->prev = entry;
  head_->next = entry;
  cache_map_[entry->key] = entry;
  return Stored;
}


/* Returns the number of entries in the map
 */
size_t Cache::NumEntries() {
  unique_lock<mutex> lock(cache_mutex_);
  return cache_map_.size();
}

/*
 * This method returns the CacheNode for the corresponding key
 * If there is no entry for the node, it returns a nullptr
 * If there is an entry, it brings the node to the head of the 
 * list
 * @param key: key for which the data is requested
 * @return: Creates a copy of the CacheNode and return 
 * the pointer if the data is present, nullptr otherwise
 */
CacheNode* Cache::getEntry(string key) {
  CacheNode* node = nullptr;

  unique_lock<mutex> lock(cache_mutex_);
  if (cache_map_.find(key) == cache_map_.end()) {
    return node;
  }

  CacheNode* dbNode = cache_map_[key];
  node = new CacheNode(dbNode);
  if (dbNode != head_->next) {
    dbNode->prev->next = dbNode->next;
    dbNode->next->prev = dbNode->prev;
    
    dbNode->next = head_->next;
    head_->next->prev = dbNode;
    dbNode->prev = head_;
    head_->next = dbNode;
  }
  
  return node;
}

/* returns a uint16_t value for the the string
 * @param: the string that needs to be converted
 * @param: the result of the conversion
 * @return: true if the conversion is successful, false otherwise
 */
static bool
str_to_uint16(const char *str, uint16_t *res)
{
  char *end;
  errno = 0;
  intmax_t val = strtoimax(str, &end, 10);
  if (errno == ERANGE || val < 0 || val > UINT16_MAX || end == str || *end != '\0')
  {
    return false;
  }
  *res = (uint16_t) val;
  return true;
}

/* This function parses the 'get command from the string
 * It retrieves the key(s), validates if the key(s) is(are) valid, searches
 * for the data in the map, and returns the data if the data is present
 * If the data is not present, it returns an empty string for that key
 * If the command does not follow memcache protocol specifications, it
 * returns string "wrong command format"
 * @param s: the string to be parsed
 * @param memcahe: the Cache pointer
 * @param: result as specified by the protocol specifications on success
 *  else an empty string if data is not found
 */
string ParseGetCmd(string s, Cache* memcache) {
  string result;
  string error_string = "CLIENT_ERROR ";
  // printf("cmd string is %s\n", s.c_str());
  int len = s.length();
  int i = 4;
  vector<string> keys;
  string key_str; 
  while (i < len) {
    if (i == len - 1 && s[i] != '\n') {
      error_string.append("wrong command format\r\n");
      return error_string;
    } else if (i == len - 1 && s[i] == '\n') {
      break;
    }
    if (s[i] == '\r') {
      if (i + 2 != len) {
        error_string.append("wrong command format\r\n");
        return error_string;
      }
      keys.push_back(key_str);
      i++;
      continue;
    }
    if (s[i] == '\n' || iscntrl(s[i])) {
      error_string.append("wrong command format\r\n");
      return error_string;
    }

    if (s[i] == ' ') {
      if (key_str.length() == 0) {
        error_string.append("wrong command format\r\n");
        return error_string;
      }
      keys.push_back(key_str);
      key_str.clear();
      i++;
      continue;
    }
    
    key_str += s[i];
    if (key_str.length() > 250) {
      error_string.append("key length exceeds 250 character limit\r\n");
      return error_string;
    }
    i++;
  }

  if (keys.size() == 0) {
    error_string.append("wrong command format\r\n");
    return error_string;
  }
 
  for (auto& key : keys) {
    CacheNode* returnedNode = memcache->getEntry(key);
    if (returnedNode == nullptr) {
      continue;
    }
    if (returnedNode->bytes == 0 || returnedNode->data == nullptr) {
      printf("Error in returning key %s\n", key.c_str());
      continue;
    }
    result.append("VALUE ").append(returnedNode->key);
    result.append(" ").append(to_string(returnedNode->flags));
    result.append(" ").append(to_string(returnedNode->bytes));
    result.append("\r\n");
    result.append(returnedNode->data, returnedNode->bytes);
    result.append("\r\n");
    delete returnedNode;
  } 
  return result;
}

/* This function parses the 'set' command, retrieves the key, flags, exp time, 
 * the number of bytes for the data, optional "noreply" and the data itself
 * If the format is correct, then it stores the data in the map. If the key is
 * already present, then it updates the data for the already present entry.
 * If there is no space in the map, and an eviction is required, then the least
 * recently used entry in the map is evicted to make space
 * @param s: the string that needs to be parsed
 * @param memcahe: the pointer to memcache
 * @return s: STORED if successful, CLIENT_ERROR on failure
 */
string ParseSetCmd(string s, Cache* memcache) {
  string result;
  string error_str = "CLIENT_ERROR ";
  // printf("cmd string is %s\n", s.c_str());
  int len = s.length();
  int i = 4;
  string key;
  uint16_t flags;
  unsigned long exp_time;
  uint64_t bytes = 0;
  while (i < len && s[i] != ' ') {
    key += s[i];
    i++;
  }
  if (key.length() == 0) {
    return error_str;
  }
  // printf("key = %s\n", key.c_str());
  if (key.length() > 250) {
    error_str.append("key length exceeds 250 characters\r\n");
    return error_str;
  }
  for (auto c : key) {
    if (iscntrl(c)) {
      error_str.append("key contains control character\r\n");
      return error_str;
    }
  }
  i++;
  if (i >= len) {
    error_str.append("wrong command format\r\n");
    return error_str;
  }
  char flag_str[50];
  int flag_str_index = 0;
  while (i < len && flag_str_index < 50 && s[i] != ' ') {
    flag_str[flag_str_index] = s[i];
    flag_str_index++;
    i++;
  }
  if (i == len || s[i] != ' ') {
    error_str.append("expected flag\r\n");
    return error_str;
  }
  flag_str[flag_str_index] = '\0';
  if (str_to_uint16(flag_str, &flags) != true) {
    error_str.append("expected flag\r\n");
    return error_str;
  }
  // printf("flags = %u\n", (unsigned int)flags);
  string exp_str;
  i++;
  if (i >= len) {
    error_str.append("wrong command format\r\n");
    return error_str;
  }
  while (i < len && s[i] != ' ') {
    exp_str += s[i];
    i++;
  }
  if (exp_str.length() == 0 || i == len) {
    error_str.append("expected expiry time\r\n");
    return error_str;
  }
  exp_time = strtoul(exp_str.c_str(), nullptr, 10);
  // printf("exp time = %lu seconds\n", exp_time);
  i++;
  if (i == len) {
    error_str.append("wrong command format\r\n");
    return error_str;
  }
  string bytes_str;
  while (i < len && s[i] != ' ' && s[i] != '\r') {
    bytes_str += s[i];
    i++;
  }
  if (bytes_str.length() == 0 || i == len) {
    printf("error parsing bytes...\n");
    error_str.append("wrong command format\r\n");
    return error_str;
  }
  bytes = strtoul(bytes_str.c_str(), nullptr, 10);
  if (bytes == 0 || bytes > MAX_DATA_LEN) {
    printf("ERROR: could not parse bytes\n");
    error_str.append("wrong bytes format\r\n");
    return error_str;
  }
  // printf("%" PRIu64 "\n", bytes);
  // skip the space, optional noreply and trailing \r\n part of the command
  while (i < len && s[i] == ' ') {
    i++;
  }

  if (i == len) {
    error_str.append("wrong bytes format\r\n");
    return error_str;
  }

  // skip noreply if present
  if (i + 7 < len && s.substr(i, 7) == "noreply") {
    // printf("noreply found..skipping..\n");
    i += 7;
  }
  if (i+2 >= len) {
    error_str.append("wrong command format\r\n");
    return error_str;
  }

  // skip the trailing \r\n as part of the command
  if (s[i] == '\r' && s[i+1] == '\n') {
    i += 2;
  } else {
    // \r\n was not found in the command
    error_str.append("wrong command format\r\n");
    return error_str;
  }
  if (i + (bytes - 1) > len) {
    error_str.append("wrong command format\r\n");
    return error_str;
  }
  char *data = new char[bytes];
  void *data_ptr = reinterpret_cast<void *>(&s[i]);
  memcpy(reinterpret_cast<void *>(data), data_ptr, bytes);
  if (i + bytes + 2 != len) {
    printf("math doesnt add up\n");
    error_str.append("wrong command format\r\n");
    delete[] data;
    return error_str;
  }
  CacheNode *node = new CacheNode();
  if (node == nullptr) {
    error_str.append("memory error\r\n");
    return error_str;
  }
  node->key = key;
  node->flags = flags;
  node->exptime = exp_time;
  node->bytes = bytes;
  node->data = data;
  if (memcache->addNewEntry(node) == Stored) {
    result = return_str[Stored];
  } else {
    error_str.append("\r\n");
    return error_str;
  }
  return result.append("\r\n");
}

/* This function parses the data from client and determines if the command
 * is a set or a get. For any other command type it returns an ERROR
 * This function is run by the threadpool module
 * @param s: command recevied from the client
 * @param socket: the bidirectional socket to which the reply will be sent
 * @param: pointer to memcache
 */
void ParseDataFromClient(string s, int socket, Cache* memcache) {
  printf("In ParseDataFromClient for string %s, socket %d\n", s.c_str(), socket);
  string send_to_client_str;
  if (s.length() < 3) {
    send_to_client_str = "ERROR wrong command format\r\n";
  } else {
    string cmd_str = s.substr(0, 3);
    if (cmd_str == "set") {
      send_to_client_str = ParseSetCmd(s, memcache);
    } else if (cmd_str == "get") {
      send_to_client_str = ParseGetCmd(s, memcache);
    } else {
      send_to_client_str = "ERROR\r\n";
    } 
  }
  if (send_to_client_str.length() == 0) {
    printf("No reply from server for command '%s'", s.c_str());
    return;
  }
  // send the result to client
  if (send(socket, send_to_client_str.c_str(), send_to_client_str.length(), 0) == -1) {
    printf("Failed to send result %s to client\n", send_to_client_str.c_str());
  }
}

