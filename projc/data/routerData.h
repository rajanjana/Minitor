/*
 * Author: Rajan Jana
 * email: rjana@usc.edu
 *
 * Course: CSCI 551
 * Project B
 *
 *
 *
 * Router data access object..stores all relevant information like router id, port no and also aes_key.
 * Information used by proxy
 *
 * */

#ifndef ROUTER_INFO_H_
#define ROUTER_INFO_H_

#include "../utility/socket.h"



class RouterInfo{

public:

	int pid;
	int routerID;
	int portNo;
	char iface[IFNAMSIZ];
	char ifaceAddr[INET6_ADDRSTRLEN];
	struct sockaddr_in addr;
	socklen_t sockLen;
	bool isFailed;
};


class RouterData{

public:
		RouterData(int len);
		RouterInfo* getRouterInformation(int index);
		void insertRouterData(int index,RouterInfo* routerInfo);
		void print();
		~RouterData();
private:
	 RouterInfo* routerInfo;
	 int length;
};

#endif
