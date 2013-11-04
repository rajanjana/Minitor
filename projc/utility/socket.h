/*
 * Author: Rajan Jana
 * email: rjana@usc.edu
 *
 * Course: CSCI 551
 * Project A
 *
 * */

#ifndef SOCKET_H
#define SOCKET_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <malloc.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <net/if.h>




/*
This is a re usable class. The purpose of this class is to return a socket based on the type specified in the class constructor and the parameters set that is the remote host address and port number. 
*/
class MySocket{

	public:
		struct addrinfo hints,*res;
		int sockType;
		int protocol;
		char* destAddr;
		char* port;
		struct sockaddr_storage their_addr;
		socklen_t addr_size;

		MySocket(int sockType, int protocol);
		void setDestAddr(const char *remote);
		char * getDestAddr();
		void setPort(const char *portNo);
		char* getPort();
		int getSocket(int flag);
		void tearDown(int sockFD);
		void clearSockAddr();
};

#endif


