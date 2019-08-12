/************tcpclient.c************************/
/* Header files needed to use the sockets API. */
/* File contains Macro, Data Type and */
/* Structure definitions along with Function */
/* prototypes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

/* BufferLength is 1000 bytes */
#define BufferLength 1000

/* Default host name of server system. Change it to your default */
/* server hostname or IP.  If the user do not supply the hostname */
/* as an argument, the_server_name_or_IP will be used as default*/
#define SERVER "127.0.0.1"

/* Server's port number */
#define SERVPORT 11211

 

/* Pass in 1 parameter which is either the */
/* address or host name of the server, or */
/* set the server name in the #define SERVER ... */
int main(int argc, char *argv[])
{
  /* Variable and structure definitions. */
  int sd, rc, length = sizeof(int);
  struct sockaddr_in serveraddr;
  char buffer[BufferLength];
  char server[255];
  char temp;
  int totalcnt = 0;
  struct hostent *hostp;
  char data[BufferLength] = "set tutorialspoint1 0 900 10 noreply\r\nmemcached1\r\n";

  /* The socket() function returns a socket */
  /* descriptor representing an endpoint. */
  /* The statement also identifies that the */
  /* INET (Internet Protocol) address family */
  /* with the TCP transport (SOCK_STREAM) */
  /* will be used for this socket. */
  /******************************************/
  /* get a socket descriptor */
  if((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Client-socket() error");
    exit(-1);
  } else {
    printf("Client-socket() OK\n");
  }
  /*If the server hostname is supplied*/
  if(argc > 1) {
    /*Use the supplied argument*/
    strcpy(server, argv[1]);
    printf("Connecting to the server %s, port %d ...\n", server, SERVPORT);
  } else {
    /*Use the default server name or IP*/
    strcpy(server, SERVER);
  }
   
  memset(&serveraddr, 0x00, sizeof(struct sockaddr_in));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_port = htons(SERVPORT);

  if((serveraddr.sin_addr.s_addr = inet_addr(server)) == (unsigned long)INADDR_NONE) {
    /* When passing the host name of the server as a */
    /* parameter to this program, use the gethostbyname() */
    /* function to retrieve the address of the host server. */
    /***************************************************/
    /* get host address */
    hostp = gethostbyname(server);
    if(hostp == (struct hostent *)NULL) {
      printf("HOST NOT FOUND --> ");
      /* h_errno is usually defined */
      /* in netdb.h */
      printf("h_errno = %d\n",h_errno);
      printf("---This is a client program---\n");
      printf("Command usage: %s <server name or IP>\n", argv[0]);
      close(sd);
      exit(-1);
    }
    memcpy(&serveraddr.sin_addr, hostp->h_addr, sizeof(serveraddr.sin_addr));
  }

  /* After the socket descriptor is received, the */
  /* connect() function is used to establish a */
  /* connection to the server. */
  /***********************************************/

  /* connect() to server. */
  if((rc = connect(sd, (struct sockaddr *)&serveraddr, sizeof(serveraddr))) < 0) {
    perror("Client-connect() error");
    close(sd);
    exit(-1);
  } else {
    printf("Connection established...\n");
  }
   
  /* Send string to the server using */
  /* the write() function. */
  /*********************************************/
  /* Write() some string to the server. */

  printf("Sending some string to the server %s...\n", server);
  rc = write(sd, data, sizeof(data));

  if(rc < 0) {
    perror("Client-write() error");
    rc = getsockopt(sd, SOL_SOCKET, SO_ERROR, &temp, (socklen_t *)&length);
    if(rc == 0) {
      /* Print out the asynchronously received error. */
      errno = temp;
      perror("SO_ERROR was");
    }
    close(sd);
    exit(-1);
  } else {
    printf("Client-write() is OK\n");
    printf("String successfully sent\n");
    printf("Waiting for the result...\n");
  }

  totalcnt = 0;
  memset(buffer, 0, sizeof(buffer));
  totalcnt = read(sd, buffer, sizeof(buffer));
  printf("Client-read() received %d bytes\n", totalcnt);
  buffer[totalcnt] = '\0';
  printf("received data is %s\n", buffer);

  /* When the data has been read, close() */
  /* the socket descriptor. */
  /****************************************/
  /* Close socket descriptor from client side. */
  close(sd);
  return 0;
}
