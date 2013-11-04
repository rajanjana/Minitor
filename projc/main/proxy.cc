/*
 * Author: Rajan Jana
 * email: rjana@usc.edu
 *
 * Course: CSCI 551
 * Project B
 *
 *
 * This file contains logic to manage the proxy part of the project.
 * here, routers are initialized, circuits are created, and messages are interfaced with the tunnel.
 *
 *
 * */

#include <time.h>
#include "start.h"
#include "../data/circuitData.h"

enum boolVal{FALSE,TRUE};

MySocket *socket1;
int sockFD;
bool recovery = 0;
int stage;
int killSent = 0;

int proxyPort;
char ethIP[INET6_ADDRSTRLEN];
char tun_name[IFNAMSIZ];
int tunFD;
int s = 1;
int itr = 0;
CircuitData circuitData;
FlowCache flowCache;
RouterData *routerData;
int globalCircuitID;
int totalRouterFailed = 0;
int routerFailed[7] = {0};
int currentSecond = 0;

// api to print the contents in hex into the log file
void logHex(unsigned char* message, int length, FileUtil * logger){
	for(int i=0; i< length ;i++){
		logger->writeLine("%02x",message[sizeof(struct iphdr) + i]);
	}
}

void logTCP(unsigned char* buffer, char* srcIP, char* destIP){
	struct iphdr *ipheader = (struct iphdr*)buffer;
	struct tcphdr *tcpHeader = (struct tcphdr*)(buffer + (ipheader->ihl * 4));

	proxyLogger->writeLine("src IP/port: %s:%hu, ",srcIP,ntohs(tcpHeader->source));
	proxyLogger->writeLine("dst IP/port: %s:%hu, ",destIP,ntohs(tcpHeader->dest));
	proxyLogger->writeLine("seqno: %u, ackno: %u\n",ntohl(tcpHeader->seq), ntohl(tcpHeader->ack_seq));

}

// get circuit id
int getCircuitID(){
	return ((itr*256) + s++);
}

//create a circuit from scratch. and store it in a map
void initializeCircuitInfo(int circuitID, CircuitInfo *info){
	info->circuitID = circuitID;
	info->minHops = minHops;
	info->pendingHops = 0;
	info->routerIDs = new int[minHops];
	info->keyHandler = new KeyHandler[minHops];

	int count = 0;
	int random = (rand() % numOfRouters) + 1;
	while(count < minHops){
		//info->keyHandler[count] = new KeyHandler;
		if(stage == 9 && !routerFailed[random])
		{	proxyLogger->writeLine("hop: %d, router: %d\n",count+1,random);
			info->routerIDs[count] = random;
			count++;
		}else if(stage != 9){
			proxyLogger->writeLine("hop: %d, router: %d\n",count+1,random);
			info->routerIDs[count] = random;
			count++;
		}
		random = (random % numOfRouters) + 1;
	}
	circuitData.insertCircuitInfo(circuitID,*info);
	circuitData.print(circuitData.getCircuitInfo(circuitID));
}

/*
 * This is initialization function for proxy.
 * Here, dynamic UDP port is assigned to proxy.
 *
 * */
void getProxyUp(){

	int bindStatus,status = 1;

	DEBUG('S',"proxy.cc -\n");

	sockaddr_in client;
	socklen_t c_len = sizeof(client);

	socket1 = new MySocket(SOCK_DGRAM,IPPROTO_UDP);
	//socket1->setDestAddr(INADDR_ANY);
	socket1->setPort("0");

	sockFD = socket1->getSocket(1); // create socket

	ASSERT(sockFD != -1);

	if(setsockopt( sockFD, SOL_SOCKET, SO_REUSEADDR, &status, sizeof(status)) < 0)
		perror("Cannot set reuseaddr\n");

	bindStatus = bind(sockFD,socket1->res->ai_addr,socket1->res->ai_addrlen); //bind socket

	ASSERT(bindStatus != -1);

	getsockname(sockFD, (struct sockaddr *)&client, &c_len);

	proxyPort = ntohs(client.sin_port);

	DEBUG('S',"proxy.cc - IP is %s\n",inet_ntoa(client.sin_addr));

	proxyLogger->writeLine("proxy port: %d\n",proxyPort);

}

/*
 * this is for stage 6. will be called when packet received from tunnel
 *
 * */

void handleEncryptCircuitMessage(char* message, int nread, int circuitID, bool killFlag){
	char payload[BUF_SIZE];
	char relayData[BUF_SIZE];
	char killMessage[HEADER_LENGTH_WITHOUT_PAYLOAD];
	RouterInfo *firstHop;
//	RouterInfo *routerInfo;
	CircuitInfo circuitInfo;
	unsigned char* eText;
	int eLen;
	int tempLen;
	int numOfBytes = 0;


	struct iphdr *ipHeader = (struct iphdr*)message;

	//set the ip address of the source to 0
	DEBUG('P',"handleEncryptCircuitMessage - setting ip to 0\n");
	ipHeader->saddr = 0;
	ipHeader->check = 0;
	ipHeader->check = csum((unsigned short *)ipHeader, sizeof(struct iphdr)/2);

	circuitInfo = circuitData.getCircuitInfo(circuitID);

	memcpy(payload,message,nread);
	tempLen = nread;
	for(int i=circuitInfo.pendingHops-1;i>=0;i--){ //encrypt the message with the keys of intermediate nodes
		DEBUG('P',"handleEncryptCircuitMessage - encrypting payload with %d\n",circuitInfo.routerIDs[i]);
		//routerInfo = routerData->getRouterInformation(circuitInfo.routerIDs[i]);
		circuitInfo.keyHandler[i].encryptMessage((unsigned char*)payload,tempLen,&eText,&eLen);
		memcpy(payload,eText,eLen);
		tempLen = eLen;
	}

	//get relay message with encrypted text
	ControlMessage::getEncryptRelayMessage(circuitID,payload,eLen,relayData);
	firstHop = routerData->getRouterInformation(circuitInfo.routerIDs[0]);
	//send to the first hop router
	numOfBytes = sendto(sockFD,relayData,HEADER_LENGTH + eLen,0,(struct sockaddr*)&firstHop->addr,firstHop->sockLen);
	ASSERT(numOfBytes > 0);

	if(killFlag && ((numOfRouters - minHops) > totalRouterFailed)){
		killSent = 1;
		ControlMessage::getRouterKillMessage(killMessage);
		firstHop = routerData->getRouterInformation(circuitInfo.routerIDs[1]);
		sleep(2);
		DEBUG('S',"Killing router %d\n",circuitInfo.routerIDs[1]);
		numOfBytes = sendto(sockFD,killMessage,HEADER_LENGTH_WITHOUT_PAYLOAD,0,(struct sockaddr*)&firstHop->addr,firstHop->sockLen);
		ASSERT(numOfBytes > 0);
	}

}

// handle circuit message for stage 5
void handleCircuitMessage(char* message, int nread){
	char* payload;
	RouterInfo *firstHop;
	CircuitInfo circuitInfo;
	int numOfBytes = 0;
	payload = new char[nread + HEADER_LENGTH_WITH_PORT];

	//create relay message
	ControlMessage::getRelayMessage(globalCircuitID,payload,message,nread);
	circuitInfo = circuitData.getCircuitInfo(globalCircuitID);
	//NetworkUtil::printPacket((unsigned char*)payload,nread+HEADER_LENGTH);
	firstHop = routerData->getRouterInformation(circuitInfo.routerIDs[0]); //fetch next hop router
	numOfBytes = sendto(sockFD,payload,nread+HEADER_LENGTH,0,(struct sockaddr*)&firstHop->addr,firstHop->sockLen);
	ASSERT(numOfBytes > 0);
}

/**
 * this function creates an encrypted circuit
 */
int createEncryptCircuit(){
	DEBUG('P',"proxy.cc - createEncryptCircuit - \n");
	int circuitID = getCircuitID();

	int numOfBytes = 0,nread = 0;
	char message[BUF_SIZE];
	char keyData[BUF_SIZE];
	char portData[BUF_SIZE];
	struct sockaddr_in client;
	RouterInfo *firstHop;

	socklen_t clientlen = sizeof(client);

	globalCircuitID = circuitID;
	DEBUG('P',"proxy.cc - createEncryptCircuit - Circuit ID = %d\n",circuitID);
	CircuitInfo circuitInfo;
	CircuitInfo info;
	initializeCircuitInfo(circuitID,&circuitInfo);
	circuitData.print(circuitData.getCircuitInfo(circuitID));

	firstHop = routerData->getRouterInformation(circuitInfo.routerIDs[circuitInfo.pendingHops]); //get first hop router
	info = circuitData.getCircuitInfo(circuitID);

	RouterInfo *routerInfo;
	RouterInfo *nextHopInfo;
	//RouterInfo *prevHopInfo;

	unsigned char* eText;
	int eLen = KEY_LENGTH;
	int tempLen;
	// loop through all hops for FDH message and circuit extend with encrypt
	while(info.pendingHops < info.minHops){
		DEBUG('P',"Create circuit: yet need to extend circuit..\n");
		routerInfo = routerData->getRouterInformation(info.routerIDs[info.pendingHops]);
		info.keyHandler[info.pendingHops].setRouterKeys((unsigned char)routerInfo->routerID);
		proxyLogger->writeLine("new-fake-diffie-hellman, router index: %d, circuit outgoing: 0x%02x, key: 0x",routerInfo->routerID, info.circuitID);
		logHexData((unsigned char*)info.keyHandler[info.pendingHops].keyText,KEY_LENGTH,proxyLogger);
		proxyLogger->writeLine("\n");
		if(info.pendingHops == 0){ //first router...do no encrypt..directly send the key text
			ControlMessage::getFDHMessage(info.circuitID,info.keyHandler[info.pendingHops].keyText,message,eLen); //first
		}else{
			memcpy(keyData,info.keyHandler[info.pendingHops].keyText,eLen);
			tempLen = KEY_LENGTH;
			for(int i=info.pendingHops - 1;i>=0;i--){ //need to encrypt with intermediate nodes with earlist node last
				DEBUG('P',"Create Encrypt circuit:encrypting with %d router key len %d for %d\n",i,eLen,info.pendingHops);
				//prevHopInfo = routerData->getRouterInformation(info.routerIDs[i]);
				info.keyHandler[i].encryptMessage((unsigned char*)keyData,tempLen,&eText,&eLen);
				memcpy(keyData,eText,eLen);
				tempLen = eLen;
			}
			ControlMessage::getFDHMessage(info.circuitID,(unsigned char*)keyData,message,eLen); //send FDH message
		}
		numOfBytes = sendto(sockFD,message,eLen + HEADER_LENGTH,0,(struct sockaddr*)&firstHop->addr,firstHop->sockLen);
		ASSERT(numOfBytes > 0);

		int portNo;
		if(info.pendingHops == (info.minHops - 1)) // last hop...send Oxffff as last port no
			portNo = 0xffff;
		else{
			nextHopInfo = routerData->getRouterInformation(info.routerIDs[info.pendingHops+1]);
			if(routerInfo == NULL)
				printf("NULL\n");
			portNo = nextHopInfo->portNo;
		}
		*(unsigned short*)(portData) = (unsigned short)htons(portNo); //get the port to encrypt
		eLen = 2;
		tempLen = eLen;
		for(int i=info.pendingHops;i >=0;i--){
			DEBUG('P',"Create encrypt circuit:encrypting port no %d with router %d\n",portNo,i);
			//prevHopInfo = routerData->getRouterInformation(info.routerIDs[i]);
			info.keyHandler[i].encryptMessage((unsigned char*)portData,tempLen,&eText,&eLen);
			memcpy(portData,eText,eLen);
			free(eText);
			tempLen = eLen;
		}

		sleep(1);
		ControlMessage::getEncryptCircuitExtendMessage(circuitID,(unsigned char*)portData,eLen,message); //send circuit extend message
		numOfBytes = sendto(sockFD,message,eLen + HEADER_LENGTH,0,(struct sockaddr*)&firstHop->addr,firstHop->sockLen);
		ASSERT(numOfBytes > 0);
		info.pendingHops++;
		circuitData.insertCircuitInfo(circuitID,info);

		nread = recvfrom(sockFD, message,HEADER_LENGTH ,0,(struct sockaddr*)&client, &clientlen);
		ASSERT(nread > 0);

		ControlMessage::extractEncryptCircuitExtendDoneMessage(message,&circuitID); // circuit extend message
		DEBUG('P',"proxy Received circuit extend done message for circuit %d\n",circuitID);
		info = circuitData.getCircuitInfo(circuitID);
		proxyLogger->writeLine("pkt from port: %d, length: %d, contents: 0x",ntohs(client.sin_port),nread - sizeof(struct iphdr));
		logHex((unsigned char*)message,nread - sizeof(struct iphdr),proxyLogger);
		proxyLogger->writeLine("\n");
		proxyLogger->writeLine("incoming extend-done circuit, incoming: 0x%02x from port: %d\n",circuitID,ntohs(client.sin_port));
	}
	return circuitID;

}

/**
 * create a non encrypted circuit for stage 5
 * first sends extend message to first hop..waits for extend done message...
 * and then extends to one more hop until all hops
 */
void createCircuit(){
	DEBUG('P',"proxy.cc - createCircuit - \n");
	int circuitID = getCircuitID();
	int numOfBytes = 0,nread = 0;
	char message[HEADER_LENGTH_WITH_PORT];
	struct sockaddr_in client;
	RouterInfo *firstHop;

	socklen_t clientlen = sizeof(client);

	globalCircuitID = circuitID;

	DEBUG('P',"proxy.cc - createCircuit - Circuit ID = %d\n",circuitID);
	CircuitInfo circuitInfo;
	CircuitInfo info;
	initializeCircuitInfo(circuitID,&circuitInfo);
	circuitData.print(circuitData.getCircuitInfo(circuitID));
	firstHop = routerData->getRouterInformation(circuitInfo.routerIDs[circuitInfo.pendingHops]);
	info = circuitData.getCircuitInfo(circuitID);
	RouterInfo *routerInfo;
	while(info.pendingHops < info.minHops){
		DEBUG('P',"Create circuit: yet need to extend circuit..\n");
		int portNo;
		if(info.pendingHops == (info.minHops - 1)) //last hop..send port no 0xffff
			portNo = 0xffff;
		else{
			routerInfo = routerData->getRouterInformation(info.routerIDs[info.pendingHops+1]);
			if(routerInfo == NULL)
				printf("NULL\n");
			portNo = routerInfo->portNo; //set next hop port no
		}
		DEBUG('P',"Sending PORT NO %d\n",portNo);
		ControlMessage::getCircuitExtendMessage(info.circuitID,portNo,message); //send circuit extend message
		numOfBytes = sendto(sockFD,message,HEADER_LENGTH_WITH_PORT,0,(struct sockaddr*)&firstHop->addr,firstHop->sockLen);
		ASSERT(numOfBytes > 0);

		info.pendingHops++; //increament hop count for next hop
		circuitData.insertCircuitInfo(info.circuitID,info); //store info for current circuit
		nread = recvfrom(sockFD, message,HEADER_LENGTH_WITH_PORT ,0,(struct sockaddr*)&client, &clientlen);
		ASSERT(nread > 0); //receive circuit extend done message

		NetworkUtil::printPacket((unsigned char*)message,HEADER_LENGTH_WITH_PORT);
		DEBUG('P',"create circuit: message type: %d\n",ControlMessage::getMessageType(message));
		ControlMessage::extractCircuitExtendDoneMessage(message,&circuitID);

		//log all the messages
		proxyLogger->writeLine("pkt from port: %d, length: %d,",ntohs(client.sin_port),nread - sizeof(struct iphdr));
		proxyLogger->writeLine(" contents: 0x");
		logHex((unsigned char*)message,nread - sizeof(struct iphdr),proxyLogger);
		proxyLogger->writeLine("\n");
		proxyLogger->writeLine("incoming extend-done circuit, incoming: 0x%02x from port: %d\n",circuitID,ntohs(client.sin_port));

		DEBUG('P',"Create circuit : Circuit ID %d\n",circuitID);
		info = circuitData.getCircuitInfo(circuitID);
	}
}

/**
 * Waits for all routers to come up and send relevant information to proxy
 */
void waitForAllRouters(){
	char* pipePos;
	char buffer[BUF_SIZE];
	char routerIP[INET6_ADDRSTRLEN];
	sockaddr_in routerAddr;
	socklen_t router_len = sizeof(routerAddr);
	int nread,routerID,routerPid;
	int msgCount = 0;
	RouterInfo *routerInfo;

	while(msgCount < numOfRouters){
		nread = recvfrom(sockFD, buffer,BUF_SIZE ,0,(struct sockaddr*)&routerAddr, &router_len);
		ASSERT(nread != 0);
		pipePos = strchr(buffer,'|');
		if(pipePos != NULL){
			sscanf(buffer,"%d|%d|%s",&routerID, &routerPid,routerIP);
			if(stage > 4)
				proxyLogger->writeLine("router: %d, pid: %d, port: %d, IP: %s\n",routerID, routerPid,ntohs(routerAddr.sin_port),routerIP);
			else if(stage <= 4)
				proxyLogger->writeLine("router: %d, pid: %d, port: %d\n",routerID, routerPid,ntohs(routerAddr.sin_port));
			DEBUG('S',"proxy.cc - proxySubroutine: Received message from router %s\n",buffer);
			routerInfo = routerData->getRouterInformation(routerID);
			routerInfo->routerID = routerID;
			routerInfo->pid = routerPid;
			routerInfo->portNo = ntohs(routerAddr.sin_port);
			routerInfo->isFailed = 0;
			routerFailed[routerID] = 0;
			memcpy(&(routerInfo->addr),&routerAddr,sizeof(struct sockaddr_in));
			routerInfo->sockLen = sizeof(struct sockaddr_in);
			routerData->insertRouterData(routerID, routerInfo);
			msgCount++;
		}
	}
}


/*
 * decrypt the message with all the intermediate node keys
 * send the decrypted message to tunnel
 *
 * */
void decapsulateMessage(char* buffer, int nread, char* nBuffer, int *length){

	int circuitID;
	char payload[BUF_SIZE];
	char srcIP[INET6_ADDRSTRLEN];
	char dstIP[INET6_ADDRSTRLEN];
	int type;

	unsigned char* clearText;
	int len;
	CircuitInfo info;
	ControlMessage::extractEncryptRelayReturnMessage(buffer,&circuitID, payload, nread);
//	RouterInfo *routerInfo;
	DEBUG('R',"proxy.cc: decapsulating....%d bytes received\n",nread);

	info = circuitData.getCircuitInfo(circuitID);

	int eLen = nread - HEADER_LENGTH;

	for(int i = 0;i < info.pendingHops;i++){
		//routerInfo = routerData->getRouterInformation(info.routerIDs[i]);
		info.keyHandler[i].decryptMessage((unsigned char*)payload,eLen,(unsigned char**)&clearText,&len);
		memcpy(payload,clearText,len);
		eLen = len;
	}

	struct iphdr *ipHeader = (struct iphdr*)payload;
	ipHeader->daddr = inet_addr(ethIP); //set the outgoing tunnel packet dest ip to this eth's IP
	ipHeader->check = 0;
	ipHeader->check = csum((unsigned short *)ipHeader, sizeof(struct iphdr)/2);

	memcpy(nBuffer,payload,len);
	NetworkUtil::getIP(srcIP,dstIP,&type,nBuffer,len);
	if(stage >=7 && NetworkUtil::isTCPPacket((unsigned char*)nBuffer)){
		proxyLogger->writeLine("incoming TCP packet, circuit: 0x%02x, ",circuitID);
	}else{
		proxyLogger->writeLine("incoming packet, circuit incoming: 0x%02x, src:%s, dst: %s\n",circuitID,srcIP,dstIP);
	}
	*length = len;
	DEBUG('P',"proxy.cc: decapsulating....%d bytes after decryption\n",len);
}


void getTCPMessage(char * buffer, int nread){
	struct iphdr *ipHeader = (struct iphdr*)buffer;
	struct tcphdr *tcpHeader = (struct tcphdr*)(buffer + (ipHeader->ihl * 4));

	int packetSize = nread - (ipHeader->ihl * 4);
	DEBUG('R',"TCP Packet size is %d\n",packetSize);

	pseudo_hdr pseudoHdr;
	pseudoHdr.dst = inet_addr(ethIP);
	pseudoHdr.src = ipHeader->saddr;
	pseudoHdr.mbz = 0;
	pseudoHdr.proto = IPPROTO_TCP;
	pseudoHdr.len = htons(packetSize);

	ASSERT(nread == ntohs(ipHeader->tot_len));

	char tcpWithChecksum[sizeof(pseudo_hdr) + packetSize];

	tcpHeader->check = 0;
	memcpy(tcpWithChecksum, (char *)&pseudoHdr, sizeof(pseudo_hdr) );
	memcpy(tcpWithChecksum + sizeof (pseudo_hdr) ,(char *) tcpHeader, packetSize);

	tcpHeader->check = (unsigned short)in_cksum((unsigned short*)tcpWithChecksum,sizeof(pseudo_hdr) + packetSize);

}


void handleWorryMessage(char* buffer, int nread){
	uint16_t circID;
	uint16_t sPort, dPort;

	circID = ControlMessage::getCircuitID(buffer);
	CircuitInfo info = circuitData.getCircuitInfo(circID);

	unsigned char* clearText;
	int len;
	DEBUG('R',"proxy.cc: decapsulating 0x92....%d bytes received 777777 %d\n",nread,circID);
	int eLen = nread - HEADER_LENGTH;

	//routerInfo = routerData->getRouterInformation(info.routerIDs[i]);
	info.keyHandler[0].decryptMessage((unsigned char*)buffer+HEADER_LENGTH,eLen,(unsigned char**)&clearText,&len);

	printf("lENGTH AFTER Decryot %d\n",len);

	ControlMessage::extractRouterWorryMessage((char*)clearText,&sPort,&dPort);

	printf("%hu ----- %hu\n",sPort,dPort);

	RouterInfo* routerInfo = routerData->getRouterInformation(info.routerIDs[1]);
	printf("Killing %d\n",info.routerIDs[1]);
	routerFailed[info.routerIDs[1]] = 1;
	routerInfo->isFailed = 1;
	routerData->insertRouterData(info.routerIDs[1], routerInfo);
	recovery = 1;

	totalRouterFailed ++;
	killSent = 0;

}


/*
 * This is the proxy subroutine.
 * This is called after the routers are forked.
 * It waits for messages from both tunnel and router.
 *
 * */
void proxySubroutine(){
	int nread;
	char buffer[BUF_SIZE];
	char nBuffer[BUF_SIZE];
	int numOfBytes;

	sockaddr_in routerAddr;
	socklen_t router_len = sizeof(routerAddr);
	int maxFD;

	char srcIP[INET6_ADDRSTRLEN];
	char destIP[INET6_ADDRSTRLEN];
	int type;

	fd_set readSet;

	srand(time(NULL));

	waitForAllRouters();
	if(stage == 5)
		createCircuit();
	if((stage == 6 || stage == 7))
		createEncryptCircuit();

	if(stage < 2)
		return;

	strcpy(tun_name,"tun1");

	tunFD = tun_alloc(tun_name,IFF_TUN | IFF_NO_PI);

	ASSERT(tunFD > 0);

	DEBUG('S',"proxy.cc - proxySubroutine: TUN FD %d\n",tunFD);

	maxFD = tunFD > sockFD ? tunFD:sockFD;
	maxFD++;
	RouterInfo *sendRouterInfo;
	while(TRUE){
		FD_ZERO(&readSet);
		FD_SET(sockFD, &readSet);
		FD_SET(tunFD, &readSet);

		if (select(maxFD , &readSet, NULL, NULL,NULL ) == -1){
			perror("select error");
			ASSERT(0);
		}
		if( FD_ISSET(sockFD,&readSet)){
			DEBUG('S',"proxy.cc - proxySubroutine: For descriptor %d\n",sockFD);
			nread = recvfrom(sockFD, buffer,BUF_SIZE ,0,(struct sockaddr*)&routerAddr, &router_len);
			ASSERT(nread != 0);
			if(ControlMessage::isTORMessage(buffer)){
				uint8_t mtype = (uint8_t)ControlMessage::getMessageType(buffer);
				if(mtype == WORRY_ROUTER && stage == 9){
					proxyLogger->writeLine("pkt from port: %d, length: %d, contents: 0x",ntohs(routerAddr.sin_port),nread - sizeof(struct iphdr));
					logHex((unsigned char*)buffer,nread - sizeof(struct iphdr),proxyLogger);
					DEBUG('S',"HHHHHHHHHH Received worry message\n");
					if(killSent)
						handleWorryMessage(buffer,nread);
				}else if(stage == 5){
					NetworkUtil::getIP(srcIP,destIP,&type,buffer+23,nread);
					proxyLogger->writeLine("pkt from port: %d, length: %d, contents: 0x",ntohs(routerAddr.sin_port),nread - sizeof(struct iphdr));
					logHex((unsigned char*)buffer,nread - sizeof(struct iphdr),proxyLogger);
					proxyLogger->writeLine("\nincoming packet, circuit incoming: 0x%02x, src:%s, ",ControlMessage::getCircuitID(buffer),srcIP);
					proxyLogger->writeLine("dst: %s\n",destIP);
					ASSERT(write(tunFD,buffer+HEADER_LENGTH,nread-HEADER_LENGTH) > 0);
					DEBUG('S',"proxy.cc - proxySubroutine: Relaying back message to tunnel %d\n",nread);
				}else if(stage == 6 || stage == 7 || stage == 8 || stage == 9){
					proxyLogger->writeLine("pkt from port: %d, length: %d, contents: 0x",ntohs(routerAddr.sin_port),nread - sizeof(struct iphdr));
					logHex((unsigned char*)buffer,nread - sizeof(struct iphdr),proxyLogger);
					proxyLogger->writeLine("\n");
					int length;
					decapsulateMessage(buffer,nread,nBuffer,&length);
					if(stage >= 7 && NetworkUtil::isTCPPacket((unsigned char*)nBuffer)){
						logTCP((unsigned char*)nBuffer,srcIP,destIP);
						getTCPMessage(nBuffer,length);
					}//else
						//decapsulateMessage(buffer,nread,nBuffer,&length);
					ASSERT(write(tunFD,nBuffer,length) > 0);
				}
			}else{
				NetworkUtil::getIP(srcIP,destIP,&type,buffer,nread);
				DEBUG('S',"proxy.cc - proxySubroutine: Received message from router %d bytes\n",nread);
				proxyLogger->writeLine("ICMP from port %d, src: %s, dst: %s, type: %c\n",ntohs(routerAddr.sin_port),srcIP,destIP,type);
				ASSERT(write(tunFD,buffer,nread) > 0);
				DEBUG('S',"proxy.cc - proxySubroutine: Sending message to tunnel %d\n",nread);
			}

		}else if(FD_ISSET(tunFD,&readSet)){
			DEBUG('S',"proxy.cc - proxySubroutine: For descriptor %d\n",tunFD);
			nread = read(tunFD,buffer,sizeof(buffer));
			ASSERT(nread > 0);
			NetworkUtil::getIP(srcIP,destIP,&type,buffer,nread);
			DEBUG('S',"proxy.cc - proxySubroutine: Received message from tunnel %d\n",nread);
			memcpy(ethIP,srcIP,INET6_ADDRSTRLEN);
			if(stage == 5){
				proxyLogger->writeLine("ICMP from tunnel, src: %s, dst: %s, type: %c\n",srcIP,destIP,type);
				handleCircuitMessage(buffer,nread);
			}else if(stage == 6){
				proxyLogger->writeLine("ICMP from tunnel, src: %s, dst: %s, type: %c\n",srcIP,destIP,type);
				handleEncryptCircuitMessage(buffer,nread,globalCircuitID,0);
			}else if(stage == 7 || stage == 8 || stage == 9){
				uint8_t protocol;
				uint16_t sPort, dPort;
				uint64_t packetSent = 0;
				int cID = globalCircuitID;
				protocol = NetworkUtil::getProtocol((unsigned char*)buffer);
				if(NetworkUtil::isTCPPacket((unsigned char*)buffer)){
					DEBUG('S',"TCP From tunnel\n");
					proxyLogger->writeLine("TCP from tunnel, ");
					logTCP((unsigned char*)buffer,srcIP,destIP);
					NetworkUtil::parseTCPPacket((unsigned char*)buffer, (unsigned short*)&sPort,(unsigned short*)&dPort);
				}else{
					proxyLogger->writeLine("ICMP from tunnel, src: %s, dst: %s, type: %c\n",srcIP,destIP,type);
					DEBUG('S',"Non From tunnel\n");
					sPort = 0;
					dPort = 0;
				}
				if((stage == 8 || stage == 9) && ((cID = flowCache.lookUp(inet_addr(srcIP),inet_addr(destIP),sPort,dPort,protocol)) == -1)){
					cID = createEncryptCircuit();
					recovery = 0;
					DEBUG('S',"First time flow...creating circuit and adding entry");
					flowCache.insertFlowEntry(cID,inet_addr(srcIP),inet_addr(destIP),sPort,dPort,protocol);
				}else if(stage >= 8){
					if(stage == 9 && recovery){
						flowCache.deleteFlowEntry(inet_addr(srcIP),inet_addr(destIP),sPort,dPort,protocol);
					}else{
						DEBUG('S',"Flow already exists...no need to create circuit\n");
						packetSent = flowCache.updateFlowEntry(inet_addr(srcIP),inet_addr(destIP),sPort,dPort,protocol);
						DEBUG('S',"Packet sent on this flow %d\n",packetSent);
					}
				}
				if(stage == 9 && (packetSent == (dieAfterPackets - 1))){
					DEBUG('S',"Need to send router kill message\n");
					killSent = 1;
					handleEncryptCircuitMessage(buffer,nread,cID,1);
				}else{
					DEBUG('S',"Circuit Id %d\n",cID);
					handleEncryptCircuitMessage(buffer,nread,cID,0);
				}
			}else{
				proxyLogger->writeLine("ICMP from tunnel, src: %s, dst: %s, type: %c\n",srcIP,destIP,type);
				int index = (ntohl(inet_addr(destIP)) % numOfRouters) + 1;
				sendRouterInfo = routerData->getRouterInformation(index);
				DEBUG('S',"proxy.cc - proxySubroutine: Sending to router %d\n",index);
				numOfBytes = sendto(sockFD,buffer,nread,0,(struct sockaddr*)&sendRouterInfo->addr,router_len);
				ASSERT(numOfBytes != 0);
			}
		}
	}
}
