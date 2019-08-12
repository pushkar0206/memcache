# memcached server
The project provides implementation of in memory memcached server. It provides support for the `set` and `get` commands. The data is stored in memory and evicted using LRU eviction scheme

# To build this project
```
cmake -H. -Bbuild -DCMAKE_INSTALL_PREFIX:PATH=/tmp/foo
cd build
cmake --build .
```

This project requires `gfortran` compiler. Please install the gfortran compiler using `apt install gfortran` if the following error is encountered:
```
-- Building for Linux
-- The Fortran compiler identification is unknown
CMake Error at projects/openblas.cmake:3 (enable_language):
No CMAKE_Fortran_COMPILER could be found.
```

# Description
The project is divided into three main components: 
1. `The networking layer` responsible for listening for incoming connections, receiving commands from client, and responding with the data
2. `The protocol parsing layer` that parses the data received from clients and performs a validity check before submitting or querying `the storage layer`
3. `The storage layer` that is responsible for storing data from the client, and retrieving the requested data. The storage layer is implemented as an in memory map that indexes on the key. To maintain an eviction policy, a LRU based eviction algorithm is used.


Once the server is started it creates a socket and listens on port 11211 for incoming client connections. For every connection that is accepted, it waits for the client to send either a `set` command to store the data or a `get` to return the data. Once the data is received from the client, it is submitted to a threadpool. The threadpool implementation is **not** mine. I have used the implementation found [here](https://github.com/mtrebi/thread-pool). The networking layer uses a [select](http://man7.org/linux/man-pages/man2/select.2.html) function to monitor for incoming connections and receive data from multiple clients. The threads within the threadpool can process multiple requests in parallel. Every request is parsed, and verified if it conforms to the protocol specification. Valid requests are then submitted to the storage layer. Since multiple threads can try to access the storage layer concurrently, access to the storage layer is syncronized using a mutex. The source code for the server can be found in the `src` folder. The server only accepts data length of upto 128KB. For requests containing data larger than 128KB the server sends an error string back to the client. The length of the key also needs to be less than or equal to 250 bytes.

# Running the unit tests
The unit tests can be found in the `test` folder. The unit tests are divided into two major types. The file `memcache_lru.cpp` contains tests that verify that request are stored and retrieved correctly, and an LRU eviction policy is followed when an entry needs to be deleted. The file `memcache_cmds.cpp` contains tests that verify that the commands are parsed as expected and the data is stored and retrieved correctly. 

The unit tests can be run as:
```
$ ./build/bin/unit_tests
Running main() from gtest_main.cc
[==========] Running 13 tests from 1 test case.
[----------] Global test environment set-up.
[----------] 13 tests from memcache
[ RUN      ] memcache.createCache
[       OK ] memcache.createCache (0 ms)
[ RUN      ] memcache.addOneNewEntry
[       OK ] memcache.addOneNewEntry (1 ms)
[ RUN      ] memcache.addTwoNewEntries
[       OK ] memcache.addTwoNewEntries (2 ms)
[ RUN      ] memcache.addDuplicateEntries
[       OK ] memcache.addDuplicateEntries (1 ms)
[ RUN      ] memcache.getOneEntry
[       OK ] memcache.getOneEntry (1 ms)
[ RUN      ] memcache.addThreeGetTwoEntries
[       OK ] memcache.addThreeGetTwoEntries (2 ms)
[ RUN      ] memcache.setCmdStrwithoutnoreply
[       OK ] memcache.setCmdStrwithoutnoreply (0 ms)
[ RUN      ] memcache.setCmdStrwithnoreply
[       OK ] memcache.setCmdStrwithnoreply (0 ms)
[ RUN      ] memcache.setCmdStrwithnoreplyandcontrolchar
[       OK ] memcache.setCmdStrwithnoreplyandcontrolchar (0 ms)
[ RUN      ] memcache.setCmdStrwithnoreplyandwrongbytes
[       OK ] memcache.setCmdStrwithnoreplyandwrongbytes (0 ms)
[ RUN      ] memcache.setCmdStrwithoutnoreplyAndget
[       OK ] memcache.setCmdStrwithoutnoreplyAndget (0 ms)
[ RUN      ] memcache.setCmdStrwithoutnoreplyAndget2
[       OK ] memcache.setCmdStrwithoutnoreplyAndget2 (0 ms)
[ RUN      ] memcache.set1CmdStrwithoutnoreplyAndget2
[       OK ] memcache.set1CmdStrwithoutnoreplyAndget2 (0 ms)
[----------] 13 tests from memcache (7 ms total)

[----------] Global test environment tear-down
[==========] 13 tests from 1 test case ran. (7 ms total)
[  PASSED  ] 13 tests.
```

# Running the server
Included with this project are a few sample client programs that can be used to send and retrieve data from the server. They are in `clienttest` folder. They can be compiled as `$ gcc client.c -g -o client`

The server can be started as follows:
```
$ ./build/bin/main 
Creating threadpool of size 12
selectserver: new connection from ::ffff:127.0.0.1 on socket 5
In ParseDataFromClient for string set tutorialspoint2 0 900 10
memcached2
, socket 5
Received no data from socket 5
connection from socket 5 disconnected
```

From client:
```
$ ./client2
Client-socket() OK
Connection established...
Sending some string to the server 127.0.0.1...
Client-write() is OK
String successfully sent
Waiting for the result...
Client-read() received 8 bytes
received data is STORED
```

# Improvements
I have verified the basic functionality and correctness. I have tested the server against multiple connections with multiple clients trying to set and get data at the same time. I have also tested that the get command can retrieve data for multiple keys, as long as the server holds the data for those keys. 
Though I have done some negative testing, I am sure, I have missed a few negative test cases. I have also not analzyed the performance of the server. I have not analzyed the server under heavy or constant client connections/disconnections to determine the optimal number of threads that need to be created within the threadgroup. Also, when a new connection is accepted, performing a malloc or new can slow down server response. It is generally better to avoid dynamically allocating data for every client connection. One way to avoid that would be to allocate a big chunk of memory when the server comes up, and to for the server to manage that memory instead of relying on the OS for every small memory request.
