#include <cstdio>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <inttypes.h>

#include "mylib.h"
#include "config.h"
#include "memcache.h"

#define PORT "11211"   // port we're listening on
#define MAX_PAYLOAD_LENGTH ((128 * 1024) + 512) 

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
				
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
/* This function received data from client and submits the
 * recieved data to threadpool from processing
 * This function assumes that the size of data from client
 * is limited to 4096 bytes
 * @param: socket to receive the data from
 * @param: threadpool to submit the data for processing
 * @param: memcache object
 * @return: 0 on success, -1 on failure
 */
int GetDataFromClient(int socket, ThreadPool *pool, Cache* memcache) {
  char buffer[MAX_PAYLOAD_LENGTH];
  string s;
  int nbytes;
  
  if ((nbytes = recv(socket, buffer, sizeof buffer, 0)) <= 0) {
    printf("Received no data from socket %d\n", socket);
    return -1;
  }

  buffer[nbytes] = '\0'; 
  s = buffer;
  // printf("Received data '%s' sending to threadpool\n", s.c_str());
  pool->submit(ParseDataFromClient, s, socket, memcache);
  return 0;
}

int main(void)
{
  fd_set master;    // master file descriptor list
  fd_set read_fds;  // temp file descriptor list for select()
  int fdmax;        // maximum file descriptor number

  int listener;     // listening socket descriptor
  int newfd;        // newly accept()ed socket descriptor
  struct sockaddr_storage remoteaddr; // client address
  socklen_t addrlen;

  char buf[256];    // buffer for client data
  int nbytes;
  char remoteIP[INET6_ADDRSTRLEN];

  int yes=1;        // for setsockopt() SO_REUSEADDR, below  
  int i, j, rv;

	struct addrinfo hints, *ai, *p;

    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

	// get us a socket and bind it
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ((rv = getaddrinfo(nullptr, PORT, &hints, &ai)) != 0) {
		fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
		exit(1);
	}
	
	for(p = ai; p != nullptr; p = p->ai_next) {
    	listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (listener < 0) { 
			continue;
		}
		
		// lose the pesky "address already in use" error message
		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

		if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
			close(listener);
			continue;
		}

		break;
	}

	// if we got here, it means we didn't get bound
	if (p == nullptr) {
		fprintf(stderr, "selectserver: failed to bind\n");
		exit(2);
	}

	freeaddrinfo(ai); // all done with this

  // listen
  if (listen(listener, 10) == -1) {
      perror("listen");
      exit(3);
  }

  // add the listener to the master set
  FD_SET(listener, &master);

  // keep track of the biggest file descriptor
  fdmax = listener; // so far, it's this one

  ThreadPool *pool = new ThreadPool(12);
  printf("Creating threadpool of size 12\n");
  pool->init();

  // create the memcache object
  Cache *memcache = new Cache();

  // main loop
  for(;;) {
    read_fds = master; // copy it
    if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
      perror("select");
      exit(4);
    }

    // run through the existing connections looking for data to read
    for(i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &read_fds)) { // we got one!!
        if (i == listener) {
          // handle new connections
          addrlen = sizeof remoteaddr;
				  newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);

				  if (newfd == -1) {
            perror("accept");
          } else {
            FD_SET(newfd, &master); // add to master set
            if (newfd > fdmax) {    // keep track of the max
              fdmax = newfd;
            }
            printf("selectserver: new connection from %s on "
                   "socket %d\n",
					  inet_ntop(remoteaddr.ss_family,
					  get_in_addr((struct sockaddr*)&remoteaddr), remoteIP, INET6_ADDRSTRLEN), 
                         newfd);
          }
        } else {
          // printf("Got data to read from client\n");
          if (GetDataFromClient(i, pool, memcache) < 0) {
            printf("connection from socket %d disconnected\n", i);
            close(i);
            FD_CLR(i, &master);
          }
        } // END handle data from client
      } // END got new incoming connection
    } // END looping through file descriptors
  } // END for(;;)
  return 0;
}
