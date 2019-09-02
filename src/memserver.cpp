#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <errno.h>
#include <ctype.h>
#include <vector>
#include "memserver.h"

using namespace std;

int CacheServer::init() {
  struct addrinfo hints, *ai, *p;
  int rv, yes = 1;

  FD_ZERO(&master_);
  FD_ZERO(&read_fds_);

  // get us a socket and bind it
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ((rv = getaddrinfo(nullptr, port_.c_str(), &hints, &ai)) != 0) {
		fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
		exit(1);
	}
	
	for(p = ai; p != nullptr; p = p->ai_next) {
    	listener_ = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (listener_ < 0) { 
			continue;
		}
		
		// lose the pesky "address already in use" error message
		setsockopt(listener_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

		if (bind(listener_, (const struct sockaddr *)p->ai_addr, p->ai_addrlen) < 0) {
			close(listener_);
			continue;
		}

		break;
	}

	// if we got here, it means we didn't get bound
	if (p == nullptr) {
		fprintf(stderr, "selectserver: failed to bind\n");
		return -1;
	}

	freeaddrinfo(ai); // all done with this

  // listen
  if (listen(listener_, 10) == -1) {
      perror("listen");
      exit(3);
  }

  // add the listener to the master set
  FD_SET(listener_, &master_);

  // keep track of the biggest file descriptor
  fdmax_ = listener_; // so far, it's this one

  return 0;
}

void CacheServer::WaitForClientRequests() {
  int newfd, i;
  struct sockaddr_storage remoteaddr;
  socklen_t addrlen;
  char remoteIP[INET6_ADDRSTRLEN];

  for(;;) {
    read_fds_ = master_; // copy it
    if (select(fdmax_+1, &read_fds_, NULL, NULL, NULL) == -1) {
      perror("select");
      exit(4);
    }

    // run through the existing connections looking for data to read
    for(i = 0; i <= fdmax_; i++) {
      if (FD_ISSET(i, &read_fds_)) { // we got one!!
        if (i == listener_) {
          // handle new connections
          addrlen = sizeof remoteaddr;
				  newfd = accept(listener_, (struct sockaddr *)&remoteaddr, &addrlen);

				  if (newfd == -1) {
            perror("accept");
          } else {
            FD_SET(newfd, &master_); // add to master set
            if (newfd > fdmax_) {    // keep track of the max
              fdmax_ = newfd;
            }
            printf("selectserver: new connection from %s on "
                   "socket %d\n",
					  inet_ntop(remoteaddr.ss_family,
					  get_in_server_addr((struct sockaddr*)&remoteaddr), remoteIP, INET6_ADDRSTRLEN), 
                         newfd);
          }
        } else {
          // printf("Got data to read from client\n");
          if (GetData(i) < 0) {
            printf("connection from socket %d disconnected\n", i);
            close(i);
            FD_CLR(i, &master_);
          }
        } // END handle data from client
      } // END got new incoming connection
    } // END looping through file descriptors
  }
}

// get sockaddr, IPv4 or IPv6:
void *CacheServer::get_in_server_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
				
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/* This function received data from client and submits the
 * recieved data to threadpool from processing
 * @param: socket to receive the data from
 * @param: threadpool to submit the data for processing
 * @param: memcache object
 * @return: 0 on success, -1 on failure
 */
int CacheServer::GetData(int socket) {
  char buffer[MAX_PAYLOAD_LENGTH];
  int nbytes = 0;
  string s;
  
  /*if ((nbytes = recv(socket, buffer, sizeof buffer, 0)) <= 0) {
    printf("Received no data from socket %d\n", socket);
    return -1;
  }*/
  int n;
  while((n = recv(socket, buffer, sizeof(buffer), 0)) > 0) {
    if(n > 0 && (nbytes + n <= MAX_PAYLOAD_LENGTH)) {
      s.append(buffer, n);
      nbytes += n;
    }
    if (n == 0 || nbytes == MAX_PAYLOAD_LENGTH) {
      break;
    }
    if (nbytes > 2 && s[nbytes - 2] == '\r' && s[nbytes - 1] == '\n') {
      break;
    }
  }

  if(n < 0) {
    return -1;
  }

  if (nbytes == 0) {
    return 0;
  }

  //buffer[nbytes] = '\0'; 
  //string s(buffer, MAX_PAYLOAD_LENGTH);
  //s = buffer;
  // printf("Received data '%s' sending to threadpool\n", s.c_str());
  printf("Received total = %d bytes\n", nbytes);
  pool_->submit(ParseDataFromClient, s, socket, memcache_, nbytes);
  return 0;
}



