#include <vector>
#include <thread>
#include "gtest/gtest.h"
#include "memcache.h"

/*
 * The unit tests in this file verify the 'set' and 'get' commands
 * Specifically, the validity of the text commands
 * Also, for valid set command, the data should be stored, and
 * a subsequent get command should be able to retrieve the data
 * Also, for a get command with multiple keys, the data should be returned
 * for those keys which are present in the cache
 */

// set without noreply
TEST(memcache, setCmdStrwithoutnoreply) {
  std::unique_ptr<Cache> cache;
  cache = std::make_unique<Cache>(3);
  ASSERT_NE(cache, nullptr);
  std::string cmd_str;
  cmd_str += "set ";
  cmd_str += "tutorialspoint ";
  cmd_str += "0 900 9\r\n";
  cmd_str += "memcached\r\n";
  std::string result = ParseSetCmd(cmd_str, cache.get(), cmd_str.length());
  ASSERT_EQ(result, "STORED\r\n");
  ASSERT_EQ(cache->NumEntries(), 1);
  std::string key = "tutorialspoint";
  time_t exptime = 900;
  std::string data = "memcached";
  CacheNode *returnedNode = cache->getEntry(key);
  ASSERT_NE(returnedNode, nullptr);
  ASSERT_EQ(memcmp(data.c_str(), returnedNode->data, returnedNode->bytes), 0);
  ASSERT_EQ(exptime, returnedNode->exptime);
}

// set with noreply
TEST(memcache, setCmdStrwithnoreply) {
  std::unique_ptr<Cache> cache;
  cache = std::make_unique<Cache>(3);
  ASSERT_NE(cache, nullptr);
  std::string cmd_str;
  cmd_str += "set ";
  cmd_str += "tutorialspoint ";
  cmd_str += "0 900 9 noreply\r\n";
  cmd_str += "memcached\r\n";
  std::string result = ParseSetCmd(cmd_str, cache.get(), cmd_str.length());
  ASSERT_EQ(result, "STORED\r\n");
  ASSERT_EQ(cache->NumEntries(), 1);
  std::string key = "tutorialspoint";
  time_t exptime = 900;
  std::string data = "memcached";
  CacheNode *returnedNode = cache->getEntry(key);
  ASSERT_NE(returnedNode, nullptr);
  ASSERT_EQ(memcmp(data.c_str(), returnedNode->data, returnedNode->bytes), 0);
  ASSERT_EQ(exptime, returnedNode->exptime);
}

// set with noreply and control character in key
TEST(memcache, setCmdStrwithnoreplyandcontrolchar) {
  std::unique_ptr<Cache> cache;
  cache = std::make_unique<Cache>(3);
  ASSERT_NE(cache, nullptr);
  std::string cmd_str;
  cmd_str += "set ";
  cmd_str += "tutorials\apoint ";
  cmd_str += "0 900 9 noreply\r\n";
  cmd_str += "memcached\r\n";
  std::string result = ParseSetCmd(cmd_str, cache.get(), cmd_str.length());
  ASSERT_NE(result, "STORED\r\n");
}

// set with noreply and wrong bytes
TEST(memcache, setCmdStrwithnoreplyandwrongbytes) {
  std::unique_ptr<Cache> cache;
  cache = std::make_unique<Cache>(3);
  ASSERT_NE(cache, nullptr);
  std::string cmd_str;
  cmd_str += "set ";
  cmd_str += "tutorialspoint ";
  cmd_str += "0 900 15 noreply\r\n";
  cmd_str += "memcached\r\n";
  std::string result = ParseSetCmd(cmd_str, cache.get(), cmd_str.length());
  ASSERT_NE(result, "STORED\r\n");
}
// set without noreply and get
TEST(memcache, setCmdStrwithoutnoreplyAndget) {
  std::unique_ptr<Cache> cache;
  cache = std::make_unique<Cache>(3);
  ASSERT_NE(cache, nullptr);
  std::string cmd_set_str;
  cmd_set_str = "set tutorialspoint 0 900 9\r\nmemcached\r\n";
  std::string result = ParseSetCmd(cmd_set_str, cache.get(), cmd_set_str.length());
  ASSERT_EQ(result, "STORED\r\n");
  ASSERT_EQ(cache->NumEntries(), 1);
  std::string cmd_get_str;
  cmd_get_str = "get tutorialspoint\r\n";
  std::string get_result = ParseGetCmd(cmd_get_str, cache.get());
  std::string expected_str = "VALUE tutorialspoint 0 9\r\nmemcached\r\n";
  ASSERT_EQ(get_result, expected_str);
}

// set two without noreply and get two
TEST(memcache, setCmdStrwithoutnoreplyAndget2) {
  std::unique_ptr<Cache> cache;
  cache = std::make_unique<Cache>(3);
  ASSERT_NE(cache, nullptr);
  std::string cmd_set_str;
  cmd_set_str = "set tutorialspoint1 415 900 10\r\nmemcached1\r\n";
  std::string result = ParseSetCmd(cmd_set_str, cache.get(), cmd_set_str.length());
  ASSERT_EQ(result, "STORED\r\n");
  ASSERT_EQ(cache->NumEntries(), 1);
  cmd_set_str = "set tutorialspoint2 416 900 11\r\nmemcached12\r\n";
  result = ParseSetCmd(cmd_set_str, cache.get(), cmd_set_str.length());
  ASSERT_EQ(result, "STORED\r\n");
  ASSERT_EQ(cache->NumEntries(), 2);
  std::string cmd_get_str;
  cmd_get_str = "get tutorialspoint1 tutorialspoint2\r\n";
  std::string get_result = ParseGetCmd(cmd_get_str, cache.get());
  std::string expected_str = "VALUE tutorialspoint1 415 10\r\nmemcached1\r\n";
  expected_str.append("VALUE tutorialspoint2 416 11\r\nmemcached12\r\n");
  ASSERT_EQ(get_result, expected_str);
}
// set one without noreply and get two
TEST(memcache, set1CmdStrwithoutnoreplyAndget2) {
  std::unique_ptr<Cache> cache;
  cache = std::make_unique<Cache>(3);
  ASSERT_NE(cache, nullptr);
  std::string cmd_set_str;
  cmd_set_str = "set tutorialspoint 0 900 9\r\nmemcached\r\n";
  std::string result = ParseSetCmd(cmd_set_str, cache.get(), cmd_set_str.length());
  ASSERT_EQ(result, "STORED\r\n");
  ASSERT_EQ(cache->NumEntries(), 1);
  std::string cmd_get_str;
  cmd_get_str = "get xyz tutorialspoint\r\n";
  std::string get_result = ParseGetCmd(cmd_get_str, cache.get());
  std::string expected_str = "VALUE tutorialspoint 0 9\r\nmemcached\r\n";
  ASSERT_EQ(get_result, expected_str);
}

void setThreadCmd(int thread_no, Cache* cache, std::vector<char*>& data_list, std::mutex& data_list_mutex) {
  std::string cmd_set_str = "set ";
  char *data = new char[9];
  std::generate_n(data, 9, std::rand);
  cmd_set_str.append(std::to_string(thread_no)).append(" 0 900 9\r\n");
  cmd_set_str.append(data, 9);
  cmd_set_str.append("\r\n");
  std::string result = ParseSetCmd(cmd_set_str, cache, cmd_set_str.length());
  ASSERT_EQ(result, "STORED\r\n");
  std::lock_guard<std::mutex> lock(data_list_mutex);
  data_list[thread_no] = data;
}

void getThreadCmd(int thread_no, Cache* cache, std::vector<char*>& data_list, std::mutex& data_list_mutex) {
  std::string cmd_get_str = "get ";
  cmd_get_str.append(std::to_string(thread_no)).append("\r\n");
  char *data = new char[9];
  {
    std::lock_guard<std::mutex> lock(data_list_mutex);
    // printf("Copy data from data_list\n");
    memcpy(data, data_list[thread_no], 9);
  }
  // printf("issue '%s'\n", cmd_get_str.c_str());
  std::string result = ParseGetCmd(cmd_get_str, cache);
  std::string expected_result = "VALUE ";
  expected_result.append(std::to_string(thread_no)).append(" 0 9\r\n");
  expected_result.append(data, 9).append("\r\n");
  ASSERT_EQ(result, expected_result);
  delete[] data;
}

// This function will create multiple set threads where each thread will set a key
// and a fixed length random data. These threads will run in parallel to each other.
// All the random data is saved for future comparison
// After all the set threads have joined, equal number of get threads are created which
// will try to get the data for each of the keys, and the data is compared with the saved
// data
TEST(memcache, stressTest) {
  std::vector<std::thread> set_threads_list(1000);
  std::unique_ptr<Cache> cache;
  std::mutex data_list_mutex;
  std::vector<char*> data_list(set_threads_list.size(), nullptr);
  cache = std::make_unique<Cache>(set_threads_list.size());
  ASSERT_NE(cache, nullptr);
  for (int i = 0; i < set_threads_list.size(); i++) {
    set_threads_list[i] = std::thread(setThreadCmd, i, cache.get(), std::ref(data_list), std::ref(data_list_mutex));
  }

  for (int i = 0; i < set_threads_list.size(); i++) {
    if (set_threads_list[i].joinable()) {
      set_threads_list[i].join();
    }
  }
  ASSERT_EQ(cache->NumEntries(), set_threads_list.size());

  std::vector<std::thread> get_threads_list(set_threads_list.size());
  for (int i = 0; i < get_threads_list.size(); i++) {
    get_threads_list[i] = std::thread(getThreadCmd, i, cache.get(), std::ref(data_list), std::ref(data_list_mutex));
  }

  for (int i = 0; i < get_threads_list.size(); i++) {
    if (get_threads_list[i].joinable()) {
      get_threads_list[i].join();
    }
  }
  for (auto data : data_list) {
    if (data != nullptr) {
      delete[] data;
    }
  }
}

