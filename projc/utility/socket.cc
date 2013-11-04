/*
 * Author: Rajan Jana
 * email: rjana@usc.edu
 *
 * Course: CSCI 551
 * Project A
 *
 * */

#include "socket.h"

/*
	Function to set the socket information.
*/
MySocket::MySocket(int sockType, int protocol){
	memset(&hints,0,sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = sockType;
	hints.ai_protocol = protocol;
	hints.ai_flags = INADDR_ANY;
}

void MySocket::setDestAddr(const char *remote){
destAddr = new char[strlen(remote) + 1];
memcpy(destAddr,remote,strlen(remote));
}

char * MySocket::getDestAddr(){
	return destAddr;
}

void MySocket::setPort(const char *portNo){
	port = new char[strlen(portNo) + 1];
	memcpy(port,portNo,strlen(portNo));
	port[strlen(portNo)] = '\0';
}

char* MySocket::getPort(){
	return port;
}

int MySocket::getSocket(int flag){
	int status,sockFD;
	char iport[20];
	char ipp[40];
	sprintf(iport,"%s",port);
	if(!flag){
		//printf("Port is %s\n",iport);
		sprintf(ipp,"%s",destAddr);
		//printf("IP is %s\n",ipp);
		if((status = getaddrinfo(destAddr,iport,&hints,&res)) != 0){
			fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
			return -1;
		}
	}else{
		if((status = getaddrinfo(NULL,iport,&hints,&res)) != 0){
			fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
			return -1;
		}
	}

	sockFD = socket(res->ai_family,res->ai_socktype,res->ai_protocol);
	return sockFD;
}

void MySocket::clearSockAddr(){
	freeaddrinfo(res);
}

void MySocket::tearDown(int sockFD){
	freeaddrinfo(res);
	close(sockFD);
}
