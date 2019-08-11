#include <algorithm>
#include "gtest/gtest.h"
#include "memcache.h"

/*
 * The unit tests in this file are designed to
 * test the storage module and the LRU caching scheme. Specifically the
 * addNewEntry() and getEntry() api's should return the 
 * expected data. These tests verify the correctioness of these apis.
 * Also, the eviction algorithm is verified. When there is no space
 * the least recently used entry should get evicted.
 */

// Verify that the cache gets created with the expected capacity
TEST(memcache, createCache) {
  std::unique_ptr<Cache> cache;
  cache = std::make_unique<Cache>(3);
  ASSERT_NE(cache, nullptr);
  ASSERT_EQ(cache->NumEntries(), 0);
  ASSERT_EQ(cache->Capacity(), 3U);
}

// Verify that a new entry gets added with addNewEntry()
TEST(memcache, addOneNewEntry) {
  std::unique_ptr<Cache> cache;
  cache = std::make_unique<Cache>();
  CacheNode *node = new CacheNode();
  node->key = "abcd1234abcd";
  node->bytes = MAX_DATA_LEN/2;
  node->data = new char[MAX_DATA_LEN/2];
  std::generate_n(node->data, MAX_DATA_LEN/2, std::rand);
  ASSERT_EQ(cache->addNewEntry(node), Stored);
  ASSERT_EQ(cache->NumEntries(), 1);
}

// Verify that two new entries get added when the keys are unique
TEST(memcache, addTwoNewEntries) {
  std::unique_ptr<Cache> cache;
  cache = std::make_unique<Cache>();
  CacheNode *node = new CacheNode();
  node->key = "abcd1234abcd";
  node->bytes = MAX_DATA_LEN/2;
  node->data = new char[MAX_DATA_LEN/2];
  std::generate_n(node->data, MAX_DATA_LEN/2, std::rand);
  ASSERT_EQ(cache->addNewEntry(node), Stored);
  ASSERT_EQ(cache->NumEntries(), 1);

  node = new CacheNode();
  node->key = "abcd123456753abcd";
  node->bytes = MAX_DATA_LEN/2;
  node->data = new char[MAX_DATA_LEN/2];
  std::generate_n(node->data, MAX_DATA_LEN/2, std::rand);
  ASSERT_EQ(cache->addNewEntry(node), Stored);
  ASSERT_EQ(cache->NumEntries(), 2);
}

// Verify that when a duplicate entry is added, the num of entries are still 1
TEST(memcache, addDuplicateEntries) {
  std::unique_ptr<Cache> cache;
  cache = std::make_unique<Cache>();
  CacheNode *node = new CacheNode();
  node->key = "abcd1234abcd";
  node->bytes = MAX_DATA_LEN/2;
  node->data = new char[MAX_DATA_LEN/2];
  std::generate_n(node->data, MAX_DATA_LEN/2, std::rand);
  ASSERT_EQ(cache->addNewEntry(node), Stored);
  ASSERT_EQ(cache->NumEntries(), 1);

  node = new CacheNode();
  node->key = "abcd1234abcd";
  node->data = new char[MAX_DATA_LEN/2];
  node->bytes = MAX_DATA_LEN/2;
  std::generate_n(node->data, MAX_DATA_LEN/2, std::rand);
  ASSERT_EQ(cache->addNewEntry(node), Stored);
  ASSERT_EQ(cache->NumEntries(), 1);
}

// Verify that the getEntry returns the correct entry
TEST(memcache, getOneEntry) {
  std::unique_ptr<Cache> cache;
  cache = std::make_unique<Cache>(3);
  CacheNode *node = new CacheNode();
  node->key = "abcd1234abcd";
  node->flags = 421;
  node->bytes = MAX_DATA_LEN/2;
  node->data = new char[MAX_DATA_LEN/2];
  std::generate_n(node->data, MAX_DATA_LEN/2, std::rand);
  ASSERT_EQ(cache->addNewEntry(node), Stored);
  ASSERT_EQ(cache->NumEntries(), 1);

  CacheNode* returnedNode = cache->getEntry(node->key);
  ASSERT_NE(returnedNode, nullptr);
  ASSERT_EQ(node->flags, returnedNode->flags);
  ASSERT_EQ(node->bytes, returnedNode->bytes);
  ASSERT_EQ(memcmp(node->data, returnedNode->data, node->bytes), 0);
  delete returnedNode;
}

// Verify the LRU eviction scheme
TEST(memcache, addThreeGetTwoEntries) {
  // add first entry
  std::unique_ptr<Cache> cache;
  cache = std::make_unique<Cache>(2);
  CacheNode *node = new CacheNode();
  node->key = "1";
  node->flags = 1;
  node->bytes = MAX_DATA_LEN/2;
  node->data = new char[MAX_DATA_LEN/2];
  std::generate_n(node->data, MAX_DATA_LEN/2, std::rand);
  ASSERT_EQ(cache->addNewEntry(node), Stored);
  ASSERT_EQ(cache->NumEntries(), 1);
  
  // add second entry
  node = new CacheNode();
  node->key = "2";
  node->flags = 435;
  node->bytes = MAX_DATA_LEN/2;
  node->data = new char[MAX_DATA_LEN/2];
  std::generate_n(node->data, MAX_DATA_LEN/2, std::rand);
  ASSERT_EQ(cache->addNewEntry(node), Stored);
  ASSERT_EQ(cache->NumEntries(), 2);

  // add third entry
  node = new CacheNode();
  node->key = "3";
  node->flags = 1;
  node->bytes = MAX_DATA_LEN/2;
  node->data = new char[MAX_DATA_LEN/2];
  std::generate_n(node->data, MAX_DATA_LEN/2, std::rand);
  ASSERT_EQ(cache->addNewEntry(node), Stored);
  ASSERT_EQ(cache->NumEntries(), 2);

  // object with key 1 should get evicted
  CacheNode* returnedNode = cache->getEntry("1");
  ASSERT_EQ(returnedNode, nullptr);

  // get object with 2
  returnedNode = cache->getEntry("2");
  ASSERT_NE(returnedNode, nullptr);
  ASSERT_EQ(returnedNode->flags, 435);
  delete returnedNode;
}


