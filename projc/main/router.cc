/*
 * Author: Rajan Jana
 * email: rjana@usc.edu
 *
 * Course: CSCI 551
 * Project B
 *
 * This files manages the router part of the project.
 * Responsible to handle message creation, relay control and data messages and provide multiple layers of encryption as
 * with onion routing.
 *
 * */

#include "start.h"
#include "../data/circuitMap.h"
#include "../timer/timers-c.h"

#define FALSE 0
#define TRUE 1
#define MAXVALUE  0x7ffffff


const char* PROXY_INTERFACE = "eth0";

FileUtil *routerLogger;
CircuitMap circuitMap;

char icmpResponse[BUF_SIZE];
int rawsockFD;
int timerFlag = 0;
int timerHandle;
int tcpRawSockFD;
int udpsockFD;
int routerID;
char* ifaceName;
char* ifaceAddr;
sockaddr_in proxyAddr;
sockaddr_in rawEthAddr;
sockaddr_in rawTCPEthAddr;
socklen_t proxy_len;
FlowCache flowMap;

sockaddr_in client;
socklen_t c_len;
int outgoingCircuitID;
int ss = 1;
int portNo;


/*

 * Referred from http://www.tenouk.com/download/pdf/Module43.pdf
 * Used to calculate checksum for the ip headers and icmp headers
 *
 * */

unsigned short csum(unsigned short *buf, int nwords)
{
        unsigned long sum;
        for(sum=0; nwords>0; nwords--)
                sum += *buf++;
        sum = (sum >> 16) + (sum &0xffff);
        sum += (sum >> 16);
        return (unsigned short)(~sum);
}

unsigned short mysum(unsigned short* buf, int words){
	unsigned long sum = 0;
	for(int i=0;i<words;i++){
		sum += (buf[i]);
	}
	return (unsigned short)(~sum);

}

/*
 * in_cksum --
 *      Checksum routine for Internet Protocol family headers (C Version)
 */
unsigned short in_cksum(unsigned short *addr,int len)
{
        register int sum = 0;
        u_short answer = 0;
        register u_short *w = addr;
        register int nleft = len;

        /*
         * Our algorithm is simple, using a 32 bit accumulator (sum), we add
         * sequential 16 bit words to it, and at the end, fold back all the
         * carry bits from the top 16 bits into the lower 16 bits.
         */
        while (nleft > 1)  {
                sum += *w++;
                nleft -= 2;
        }

        /* mop up an odd byte, if necessary */
        if (nleft == 1) {
                *(u_char *)(&answer) = *(u_char *)w ;
                sum += answer;
        }

        /* add back carry outs from top 16 bits to low 16 bits */
        sum = (sum >> 16) + (sum & 0xffff);     /* add hi 16 to low 16 */
        sum += (sum >> 16);                     /* add carry */
        answer = ~sum;                          /* truncate to 16 bits */
        return(answer);
}


/**
 * generate outgoing id
 */
int getOutgoingID(int incomingID){
	return ((routerID*256) + ss++);
}

void logTCP(unsigned char* packet, char* prevIP, char* newIP, char* dstIP){
	struct iphdr *iphdr = (struct iphdr*)packet;
	struct tcphdr *tcpHeader = (struct tcphdr*)(packet + (iphdr->ihl * 4));

	routerLogger->writeLine("incoming src IP/port: %s:%hu, ",prevIP,ntohs(tcpHeader->source));
	routerLogger->writeLine("outgoing src IP/port: %s:%hu, ",newIP,ntohs(tcpHeader->source));
	routerLogger->writeLine("dst IP/port: %s:%hu, ",dstIP,ntohs(tcpHeader->dest));
	routerLogger->writeLine("seqno: %u, ackno: %u\n",ntohl(tcpHeader->seq), ntohl(tcpHeader->ack_seq));
}

/**
 * api to log the hex value of the message into log file
 */
void logHexData(unsigned char* message, int length, FileUtil *util){
	for(int i=0;i<length;i++){
		util->writeLine("%02x",message[i]);
	}
}

/**
 * Function to forward the message to the outside world
 */
void forwardMessage(char* buffer, int nread, int cirID){
	struct msghdr messageHdr;
	struct iovec iovector;
	struct sockaddr_in destAddr;
	char icmpMessage[nread - sizeof(struct iphdr)];
	int numOfBytes;

	DEBUG('R',"router.cc - Router ID %d - Forwarding %d bytes via raw socket\n",routerID,nread);

	struct iphdr *ipHeader = (struct iphdr*)buffer;

	bzero(&destAddr,sizeof(sockaddr_in));
	destAddr.sin_family = AF_INET;
	memcpy(&destAddr.sin_addr.s_addr,&(ipHeader->daddr),sizeof(destAddr.sin_addr.s_addr));

	bzero(icmpMessage,nread-sizeof(struct iphdr));
	memcpy(icmpMessage,buffer+sizeof(struct iphdr),nread - sizeof(struct iphdr));

	bzero(&iovector,sizeof(iovec));
	iovector.iov_base = (char*)icmpMessage; //set the icmp message in the scatter gatter vector
	iovector.iov_len = nread - sizeof(struct iphdr);

	bzero(&messageHdr,sizeof(struct msghdr));
	messageHdr.msg_name = (char*)&destAddr; //set the destination address
	messageHdr.msg_namelen = sizeof(struct sockaddr_in);
	messageHdr.msg_iov = &iovector;
	messageHdr.msg_iovlen = 1;

	int cID;
	if((cID = flowMap.lookUp(inet_addr(ifaceAddr),ipHeader->daddr,0,0,IPPROTO_ICMP)) == -1){
		DEBUG('R',"First time flow...creating circuit and adding entry");
		flowMap.insertFlowEntry(cirID,inet_addr(ifaceAddr),ipHeader->daddr,0,0,IPPROTO_ICMP);
	}else{
		DEBUG('R',"Flow already exists");
	}


	numOfBytes = sendmsg(rawsockFD,&messageHdr,0);

	ASSERT(numOfBytes > 0);

	DEBUG('R',"router.cc - Router ID %d - Sent %d bytes via raw socket\n",routerID,numOfBytes);
}

void forwardTCPMessage(char* buffer, int nread, int cirID){
	struct iphdr *ipHeader = (struct iphdr*)buffer;
	struct tcphdr *tcpHeader = (struct tcphdr*)(buffer + (ipHeader->ihl * 4));

	int packetSize = nread - (ipHeader->ihl * 4);
	DEBUG('R',"TCP Packet size is %d\n",packetSize);

	pseudo_hdr pseudoHdr;
	pseudoHdr.dst = ipHeader->daddr;
	pseudoHdr.src = inet_addr(ifaceAddr);
	pseudoHdr.mbz = 0;
	pseudoHdr.proto = IPPROTO_TCP;
	pseudoHdr.len = htons(packetSize);

	ASSERT(nread == ntohs(ipHeader->tot_len));

	char tcpWithChecksum[sizeof(pseudo_hdr) + packetSize];

	tcpHeader->check = 0;
	memcpy(tcpWithChecksum, (char *)&pseudoHdr, sizeof(pseudo_hdr) );
	memcpy(tcpWithChecksum + sizeof (pseudo_hdr) ,(char *) tcpHeader, packetSize);

	tcpHeader->check = (unsigned short)in_cksum((unsigned short*)tcpWithChecksum,sizeof(pseudo_hdr) + packetSize);

	struct msghdr messageHdr;
	struct iovec iovector;
	struct sockaddr_in destAddr;
	char tcpMessage[packetSize];
	int numOfBytes;

	DEBUG('R',"router.cc - Router ID %d - Forwarding TCP %d bytes via raw socket\n",routerID,nread);

	bzero(&destAddr,sizeof(sockaddr_in));
	destAddr.sin_family = AF_INET;
	memcpy(&destAddr.sin_addr.s_addr,&(ipHeader->daddr),sizeof(destAddr.sin_addr.s_addr));

	bzero(tcpMessage,packetSize);
	memcpy(tcpMessage,buffer+(ipHeader->ihl * 4),packetSize);

	bzero(&iovector,sizeof(iovec));
	iovector.iov_base = (char*)tcpMessage; //set the icmp message in the scatter gatter vector
	iovector.iov_len = packetSize;

	bzero(&messageHdr,sizeof(struct msghdr));
	messageHdr.msg_name = (char*)&destAddr; //set the destination address
	messageHdr.msg_namelen = sizeof(struct sockaddr_in);
	messageHdr.msg_iov = &iovector;
	messageHdr.msg_iovlen = 1;
	int cID;
	if((cID = flowMap.lookUp(inet_addr(ifaceAddr),ipHeader->daddr,ntohs(tcpHeader->source),ntohs(tcpHeader->dest),IPPROTO_TCP)) == -1){
		DEBUG('R',"First time flow...creating circuit and adding entry");
		flowMap.insertFlowEntry(cirID,inet_addr(ifaceAddr),ipHeader->daddr,ntohs(tcpHeader->source),ntohs(tcpHeader->dest),IPPROTO_TCP);
	}else{
		DEBUG('R',"Flow already exists");
	}

	numOfBytes = sendmsg(tcpRawSockFD,&messageHdr,0);

	ASSERT(numOfBytes > 0);

	DEBUG('R',"router.cc - Router ID %d - Sent TCP %d bytes via raw socket\n",routerID,numOfBytes);


}



/**
 * relay back encrypted message...
 */
void relayBackEncryptedData(char* payload,int nread,int circuitID, int flag){
	char relayData[BUF_SIZE];
	struct iphdr* ipHeader;
	char srcIP[INET6_ADDRSTRLEN];
	char dstIP[INET6_ADDRSTRLEN];
	int type;
	int numOfBytes = 0;
	CircuitDao circuitDao;
	unsigned char *eText;
	int eLen;
	int outgoingID = outgoingCircuitID;

	uint16_t sPort, dPort;
	uint8_t protocol = NetworkUtil::getProtocol((unsigned char*)payload);

	DEBUG('R',"encryption relayBackData - Relaying back data.....%d\n",routerID);
	NetworkUtil::getIP(srcIP,dstIP,&type,payload,nread);
	ipHeader = (struct iphdr*)payload;

	if(!flag){
		if(stage == 8 || stage == 9){
			if(protocol == IPPROTO_TCP){
				NetworkUtil::parseTCPPacket((unsigned char*)payload, &sPort, &dPort);
				outgoingID = flowMap.lookUp(ipHeader->daddr,ipHeader->saddr,dPort,sPort,protocol);
			}else{
				outgoingID = flowMap.lookUp(ipHeader->daddr,ipHeader->saddr,0,0,protocol);
			}
		}
		printf("\nOutgoing id %d\n",outgoingID);
		circuitDao = circuitMap.getOutgoingCircuitDao(outgoingID);
		ipHeader->daddr = 0; //set destination address to 0.
		ipHeader->check = 0;
		ipHeader->check = csum((unsigned short *)ipHeader, sizeof(struct iphdr)/2);
		if(stage < 7)
			routerLogger->writeLine("incoming packet, src:%s, dst: %s, outgoing circuit: 0x%02x\n",srcIP,dstIP,circuitDao.incomingID);
	}else{
		circuitDao = circuitMap.getOutgoingCircuitDao(circuitID);
		routerLogger->writeLine("relay reply encrypted packet, circuit incoming: 0x%02x, outgoing: 0x%02x\n",circuitDao.outgoingID,circuitDao.incomingID);
	}

	//encrypt message and send the data backwards..first layer of onion routing
	circuitDao.keyHandler.encryptMessage((unsigned char*)payload,nread,&eText,&eLen);

	ControlMessage::getEncryptRelayReturnMessage(circuitDao.incomingID,(char*)relayData,(char*)eText,eLen);
	socklen_t socklen = sizeof(circuitDao.prev);
	numOfBytes = sendto(udpsockFD,relayData,eLen + HEADER_LENGTH,0,(struct sockaddr*)&circuitDao.prev,socklen);
	ASSERT(numOfBytes > 0);
}




/**
 * relay back data for non-encrypted message..
 */
void relayBackData(char* payload, int nread, int circuitID, int flag){
	char* returnData;
	struct iphdr* ipHeader;
	returnData = new char[nread + HEADER_LENGTH];
	char srcIP[INET6_ADDRSTRLEN];
	char dstIP[INET6_ADDRSTRLEN];
	int type;
	int numOfBytes = 0;
	CircuitDao circuitDao;
	DEBUG('R',"relayBackData - Relaying back data.....\n");
	NetworkUtil::getIP(srcIP,dstIP,&type,payload,nread);
	if(!flag){
		circuitDao = circuitMap.getOutgoingCircuitDao(outgoingCircuitID);
		routerLogger->writeLine("incoming packet, src:%s, dst: %s, outgoing circuit: 0x%02x\n",srcIP,dstIP,circuitDao.incomingID);
	}else{
		circuitDao = circuitMap.getOutgoingCircuitDao(circuitID);
		routerLogger->writeLine("relay reply packet, circuit incoming: 0x%02x, outgoing: 0x%02x",circuitDao.outgoingID,circuitDao.incomingID);
		routerLogger->writeLine(", src: %s, incoming dst: %s, outgoing dest: %s\n",srcIP,dstIP,circuitDao.prevHopIP);
	}
	ipHeader = (struct iphdr*)payload;
	ipHeader->daddr = inet_addr(circuitDao.prevHopIP); //set the destination IP to previous hop
	ipHeader->check = 0;
	ipHeader->check = csum((unsigned short *)ipHeader, sizeof(struct iphdr)/2);
	ControlMessage::getRelayReturnMessage(circuitDao.incomingID,returnData, payload,nread);

	socklen_t socklen = sizeof(circuitDao.prev);
	numOfBytes = sendto(udpsockFD,returnData,nread + HEADER_LENGTH,0,(struct sockaddr*)&circuitDao.prev,socklen);
	ASSERT(numOfBytes > 0);

}


int sendWorryMessage(void* circuitID){
	int circuitID1 = *(int*)circuitID;
	char message[HEADER_LENGTH + 4];
	unsigned char *eText;
	int eLen;

	if(timerFlag){
		CircuitDao circuitDao = circuitMap.getIncomingCircuitDao(circuitID1);

		ControlMessage::getRouterWorryMessage(message,circuitID1,portNo,circuitDao.next_hop_portNo);

		circuitDao.keyHandler.encryptMessage((unsigned char*)message+23,4,&eText,&eLen);

		char message1[HEADER_LENGTH + eLen];

		memcpy(message1,message,23);
		memcpy(message1+23,eText,eLen);

		socklen_t socklen = sizeof(circuitDao.prev);


		DEBUG('R',"Sending 0x92 to proxy...\n");
		routerLogger->writeLine("router %hu worried about %hu on circuit %d\n",portNo,circuitDao.next_hop_portNo,circuitID1);
		int numOfBytes = sendto(udpsockFD,message1,eLen + HEADER_LENGTH,0,(struct sockaddr*)&circuitDao.prev,socklen);
		ASSERT(numOfBytes >= 0);
	}
	timerFlag = 0;
	Timers_RemoveTimer(timerHandle);
	return 0;

}





/**
 * handle TOR Message - a simple switch case based on message type...for encryption
 */
void handleTOREncryptMessage(char* message, struct sockaddr_in client,int nread){
	uint8_t type = ControlMessage::getMessageType(message);
	//DEBUG('R',"Router.cc - handleTORMessage Type: %d\n",type);
	int circuitID;
	int numOfBytes = 0;
	CircuitDao circuitDao;
	char IP[INET6_ADDRSTRLEN];
	int type1;
	socklen_t socklen;
	struct sockaddr_in nextHop;
	unsigned char keyData[nread - HEADER_LENGTH];
	unsigned char portData[nread - HEADER_LENGTH];
	unsigned char relayData[nread - HEADER_LENGTH];
	char payload[nread - HEADER_LENGTH];

	unsigned char* cleartext;
	int len = 6;
	DEBUG('R',"Router %d Received message type 0x%02x\n",routerID,type);
	if(type == ENCRYPT_RELAY_DATA)
		routerLogger->writeLine("pkt from port: %d, length %d, contents: 0x",ntohs(client.sin_port),nread-sizeof(struct iphdr));
	else
		routerLogger->writeLine("pkt from port: %d, length %d, contents: 0x",ntohs(client.sin_port),nread-sizeof(struct iphdr));
	logHex((unsigned char*)message,nread - sizeof(struct iphdr),routerLogger);
	routerLogger->writeLine("\n");
	switch(type){
		case FDH_MESSAGE: // F-D-H Message
			ControlMessage::extractFDHMessage(message,&circuitID,keyData,nread);
			if(circuitMap.isNewIncomingCircuit(circuitID)){ //new incoming message
				routerLogger->writeLine("fake-diffie-hellman, new circuit incoming: 0x%02x, key: 0x",circuitID);
				logHexData((unsigned char*)keyData,nread - HEADER_LENGTH,routerLogger);
				routerLogger->writeLine("\n");
				DEBUG('R',"Router.cc - handleEncryptTORMessage: First incoming ID for router %d - circuitID %d\n",routerID,circuitID);
				circuitDao.incomingID = circuitID;
				circuitDao.keyHandler.extractKey(keyData);
				circuitDao.outgoingID = -1;
				printf("Router %d..setting outgoing circuit %d\n",routerID,circuitDao.outgoingID);
				circuitMap.insertIncomingCircuitDao(circuitID,circuitDao);
			}else{
				circuitDao = circuitMap.getIncomingCircuitDao(circuitID);
				DEBUG('R',"Router %d Forwarding FDH Message to port %d\n",routerID,circuitDao.next_hop_portNo);
				circuitDao.keyHandler.decryptMessage(keyData,nread-HEADER_LENGTH,(unsigned char**)&cleartext,&len);
				ControlMessage::getFDHMessage(circuitDao.outgoingID,cleartext,message,len);
				socklen = sizeof(nextHop);
				bzero((char *)&nextHop,sizeof(nextHop));
				nextHop.sin_family = AF_INET;
				nextHop.sin_addr.s_addr = inet_addr("127.0.0.1");
				nextHop.sin_port = htons(circuitDao.next_hop_portNo);
				numOfBytes = sendto(udpsockFD,message,HEADER_LENGTH+len,0,(struct sockaddr*)&nextHop,socklen);
				ASSERT(numOfBytes > 0);
				routerLogger->writeLine("fake-diffie-hellman, forwarding, circuit incoming: 0x%02x, key: 0x",circuitID);
				logHex((unsigned char*)message + 3,nread-HEADER_LENGTH,routerLogger);
				routerLogger->writeLine("\n");
				//forward fdh message
			}
			break;
		case ENCRYPT_CIRCUIT_EXTEND:
			ControlMessage::extractEncryptCircuitExtendMessage(message,&circuitID,portData,nread);
			DEBUG('R',"Router %d : Got encrypt circuit extend message\n",routerID);
			circuitDao.keyHandler.decryptMessage(portData,nread-HEADER_LENGTH,(unsigned char**)&cleartext,&len);
			circuitDao = circuitMap.getIncomingCircuitDao(circuitID);
			printf("router id %d ------------------ %d for %d\n",routerID,circuitDao.outgoingID,circuitDao.incomingID);
			if(circuitDao.outgoingID == -1){//first time
				DEBUG('R',"Router %d got extend circuit for first time\n",routerID);
				circuitDao.outgoingID = getOutgoingID(0);
				outgoingCircuitID = circuitDao.outgoingID;
				circuitDao.next_hop_portNo = ntohs(*(unsigned short*)cleartext);
				memcpy(&circuitDao.prev,&client,sizeof(struct sockaddr_in));
				circuitDao.prev_hop_portNo = ntohs(client.sin_port);
				if(circuitDao.next_hop_portNo == 0xffff){
					circuitDao.isLastHop = 1;
					DEBUG('R',"Router.cc - handleEncryptTORMessage: LAST HOP\n");
				}else{
					circuitDao.isLastHop = 0;
				}
				circuitMap.insertIncomingCircuitDao(circuitID,circuitDao);
				circuitMap.insertOutgoingCircuitDao(circuitDao.outgoingID,circuitDao);
				ControlMessage::getEncryptCircuitExtendDoneMessage(circuitID,message);
				socklen = sizeof(circuitDao.prev);
				numOfBytes = sendto(udpsockFD,message,HEADER_LENGTH,0,(struct sockaddr*)&circuitDao.prev,socklen);
				ASSERT(numOfBytes > 0);
				routerLogger->writeLine("new extend circuit: incoming: 0x%02x, outgoing: 0x%02x at %d\n",circuitID,circuitDao.outgoingID,circuitDao.next_hop_portNo);
			}else{
				DEBUG('R',"Router.cc - Forwarding circuit extend to %d\n",circuitDao.next_hop_portNo);
				ControlMessage::getEncryptCircuitExtendMessage(circuitDao.outgoingID,cleartext,len,message);
				socklen = sizeof(nextHop);
				bzero((char *)&nextHop,sizeof(nextHop));
				nextHop.sin_family = AF_INET;
				nextHop.sin_addr.s_addr = inet_addr("127.0.0.1");
				nextHop.sin_port = htons(circuitDao.next_hop_portNo);
				numOfBytes = sendto(udpsockFD,message,HEADER_LENGTH+len,0,(struct sockaddr*)&nextHop,socklen);
				ASSERT(numOfBytes > 0);
				routerLogger->writeLine("forwarding extend circuit: incoming: 0x%02x, outgoing: 0x%02x at %d\n",circuitID,circuitDao.outgoingID,circuitDao.next_hop_portNo);
			}
			break;
		case ENCRYPT_CIRCUIT_EXTEND_DONE:
			DEBUG('R',"Router.cc - Router %d got circuit extend done\n",routerID);
			ControlMessage::extractEncryptCircuitExtendDoneMessage(message,&circuitID);
			circuitDao = circuitMap.getOutgoingCircuitDao(circuitID);
			ControlMessage::getEncryptCircuitExtendDoneMessage(circuitDao.incomingID,message);
			socklen = sizeof(nextHop);
			bzero((char *)&nextHop,sizeof(nextHop));
			nextHop.sin_family = AF_INET;
			nextHop.sin_addr.s_addr = inet_addr("127.0.0.1");
			nextHop.sin_port = htons(circuitDao.prev_hop_portNo);
			numOfBytes = sendto(udpsockFD,message,HEADER_LENGTH,0,(struct sockaddr*)&nextHop,socklen);
			ASSERT(numOfBytes > 0);
			DEBUG('R',"Router %d - Forwarding to previous hop %d\n",routerID,circuitDao.prev_hop_portNo);
			routerLogger->writeLine("forwarding extend-done circuit, incoming: 0x%02x, outgoing: 0x%02x at %d\n",circuitID,circuitDao.incomingID, circuitDao.prev_hop_portNo);
			break;
		case ENCRYPT_RELAY_DATA:
			DEBUG('R',"Router.cc - Router %d got RELAY message\n",routerID);
			ControlMessage::extractEncryptRelayMessage(message, &circuitID,relayData,nread);
			circuitDao = circuitMap.getIncomingCircuitDao(circuitID);
			routerLogger->writeLine("relay encrypted packet, circuit incoming: 0x%02x, outgoing: 0x%02x\n",circuitID,circuitDao.outgoingID);
			circuitDao.keyHandler.decryptMessage(relayData,nread-HEADER_LENGTH,(unsigned char**)&cleartext,&len);
			if(!circuitDao.isLastHop){
				ControlMessage::getEncryptRelayMessage(circuitDao.outgoingID,(char*)cleartext,len,message);
				socklen = sizeof(nextHop);
				bzero((char *)&nextHop,sizeof(nextHop));
				nextHop.sin_family = AF_INET;
				nextHop.sin_addr.s_addr = inet_addr("127.0.0.1");
				nextHop.sin_port = htons(circuitDao.next_hop_portNo);
				numOfBytes = sendto(udpsockFD,message,len+HEADER_LENGTH,0,(struct sockaddr*)&nextHop,socklen);
				ASSERT(numOfBytes > 0);

				if(stage == 9 && !timerFlag){
					timerHandle = Timers_AddTimer(5000, sendWorryMessage, (void*)&circuitDao.incomingID);
					timerFlag = 1;
				}
			}else{
				NetworkUtil::getIP(circuitDao.prevHopIP,IP,&type1,(char*)cleartext,len);

				if(stage >= 7 && NetworkUtil::isTCPPacket((unsigned char*)cleartext)){
					routerLogger->writeLine("outgoing TCP packet, circuit incoming: 0x%02x, ",circuitDao.incomingID);
					logTCP((unsigned char*)cleartext,circuitDao.prevHopIP,ifaceAddr,IP);
					forwardTCPMessage((char*)cleartext,len,circuitDao.outgoingID);
				}else{
					routerLogger->writeLine("outgoing packet, circuit incoming: 0x%02x, ",circuitDao.incomingID);
					routerLogger->writeLine("incoming src: %s, outgoing src: %s, dst: %s\n",circuitDao.prevHopIP,ifaceAddr,IP);
					forwardMessage((char*)cleartext,len, circuitDao.outgoingID);
				}
			}
			break;
		case ENCRYPT_RETURN_DATA:
			ControlMessage::extractEncryptRelayReturnMessage(message,&circuitID,payload,nread);
			relayBackEncryptedData(payload, nread-HEADER_LENGTH,circuitID,1);

			if(stage == 9 && timerFlag){
				timerFlag = 0;
				Timers_RemoveTimer(timerHandle);
			}


			break;
		case KILL_ROUTER:
			printf("-----------------------------------------Router %d killing itself\n",routerID);
			routerLogger->writeLine("Router %d killed\n",routerID);
			exit(0);
			break;
	}
}



void handleTORMessage(char* message, struct sockaddr_in client, int nread){
	int type = ControlMessage::getMessageType(message);
	//DEBUG('R',"Router.cc - handleTORMessage Type: %d\n",type);
	int circuitID,nextPortNo;
	int numOfBytes = 0;
	CircuitDao circuitDao;
	socklen_t socklen1;
	struct iphdr *ipHeader;
	char relayData[nread];
	char payload[nread - HEADER_LENGTH];
	char IP[INET6_ADDRSTRLEN];
	int type1;
	socklen_t socklen;
	struct sockaddr_in nextHop;
	switch(type){
		case CIRCUIT_EXTEND:
			ControlMessage::extractCircuitExtendMessage(message,&circuitID,&nextPortNo);
			routerLogger->writeLine("pkt from port: %d, length: %d, contents: 0x",ntohs(client.sin_port),nread-sizeof(struct iphdr));
			logHex((unsigned char*)message,nread - sizeof(struct iphdr),routerLogger);
			routerLogger->writeLine("\n");
			if(circuitMap.isNewIncomingCircuit(circuitID)){
				DEBUG('R',"Router.cc - handleTORMessage: First incoming ID\n");
				circuitDao.incomingID = circuitID;
				circuitDao.next_hop_portNo = nextPortNo;
				if(nextPortNo == 0xffff){
					circuitDao.isLastHop = 1;
				}else{
					circuitDao.isLastHop = 0;
				}
				DEBUG('R',"Router.cc - handleTORMessage: last hop value %d\n",circuitDao.isLastHop);
				circuitDao.outgoingID = getOutgoingID(circuitID);
				outgoingCircuitID = circuitDao.outgoingID;
				circuitDao.prev_hop_portNo = ntohs(client.sin_port);
				memcpy(&circuitDao.prev,&client,sizeof(struct sockaddr_in));
				circuitMap.insertIncomingCircuitDao(circuitID,circuitDao);
				circuitMap.insertOutgoingCircuitDao(circuitDao.outgoingID,circuitDao);
				circuitMap.iPrint(circuitID);
				routerLogger->writeLine("new extend circuit: incoming: 0x%02x,",circuitDao.incomingID);
				routerLogger->writeLine(" outgoing: 0x%02x at %d\n",circuitDao.outgoingID,circuitDao.next_hop_portNo);
				ControlMessage::getCircuitExtendDoneMessage(circuitDao.incomingID,message);
				socklen = sizeof(circuitDao.prev);
				numOfBytes = sendto(udpsockFD,message,23,0,(struct sockaddr*)&circuitDao.prev,socklen);
				ASSERT(numOfBytes > 0);
			}else{
				DEBUG('R',"Router.cc - handleTORMessage: existing incoming ID\n");
				circuitDao = circuitMap.getIncomingCircuitDao(circuitID);
				DEBUG('R',"Router.cc - handleTORMessage: last hop value %d\n",circuitDao.isLastHop);
				if(circuitDao.isLastHop){
					DEBUG('R',"Router.cc - handleTORMessage: LAST HOP\n");
					ControlMessage::getCircuitExtendDoneMessage(circuitDao.incomingID,message);
					socklen = sizeof(circuitDao.prev);
					numOfBytes = sendto(udpsockFD,message,23,0,(struct sockaddr*)&circuitDao.prev,socklen);
					ASSERT(numOfBytes > 0);
				}else{
					DEBUG('R',"Router.cc - handleTORMessage: Forwarding to %d\n",circuitDao.next_hop_portNo);
					ControlMessage::getCircuitExtendMessage(circuitDao.outgoingID,nextPortNo,message);
					socklen = sizeof(nextHop);
					bzero((char *)&nextHop,sizeof(nextHop));
					nextHop.sin_family = AF_INET;
					nextHop.sin_addr.s_addr = inet_addr("127.0.0.1");
					nextHop.sin_port = htons(circuitDao.next_hop_portNo);
					numOfBytes = sendto(udpsockFD,message,25,0,(struct sockaddr*)&nextHop,socklen);
					ASSERT(numOfBytes > 0);
					routerLogger->writeLine("forwarding extend circuit: incoming: 0x%02x, outgoing: 0x%02x at %d\n",circuitDao.incomingID,circuitDao.outgoingID,circuitDao.next_hop_portNo);
				}
			}
			break;
		case CIRCUIT_EXTEND_DONE:
			ControlMessage::extractCircuitExtendDoneMessage(message,&circuitID);
			circuitDao = circuitMap.getOutgoingCircuitDao(circuitID);
			ControlMessage::getCircuitExtendDoneMessage(circuitDao.incomingID,message);

			routerLogger->writeLine("pkt from port: %d, length: %d, contents: 0x",circuitDao.next_hop_portNo, nread - sizeof(struct iphdr));
			logHex((unsigned char*)message, nread - sizeof(struct iphdr),routerLogger);
			routerLogger->writeLine("\n");
			socklen1 = sizeof(circuitDao.prev);
			numOfBytes = sendto(udpsockFD,message,23,0,(struct sockaddr*)&circuitDao.prev,socklen1);
			ASSERT(numOfBytes > 0);
			routerLogger->writeLine("forwarding extend-done circuit, incoming: 0x%02x, outgoing: 0x%02x at %d\n",circuitDao.outgoingID,circuitDao.incomingID,circuitDao.prev_hop_portNo);
			break;
		case RELAY_DATA:
			routerLogger->writeLine("pkt from port: %d, length: %d, contents: 0x", ntohs(client.sin_port), nread-sizeof(struct iphdr));
			logHex((unsigned char*)message,nread - sizeof(struct iphdr),routerLogger);
			routerLogger->writeLine("\n");
			DEBUG('R',"handleTORMessage - Router id %d\n",routerID);
			DEBUG('R',"handleTORMessage - Nread id %d\n",nread);
			ControlMessage::extractRelayMessage(message,&circuitID,payload,nread);
			circuitDao = circuitMap.getIncomingCircuitDao(circuitID);
			ipHeader = (struct iphdr*)payload;
			NetworkUtil::getIP(circuitDao.prevHopIP,IP,&type1,payload,nread-HEADER_LENGTH);
			circuitMap.insertIncomingCircuitDao(circuitDao.incomingID,circuitDao);
			circuitMap.insertOutgoingCircuitDao(circuitDao.outgoingID,circuitDao);
			ipHeader->saddr = inet_addr(ifaceAddr);
			ipHeader->check = 0;
			ipHeader->check = csum((unsigned short *)ipHeader, sizeof(struct iphdr)/2);
			if(!circuitDao.isLastHop){
				ControlMessage::getRelayMessage(circuitDao.outgoingID,relayData,payload,nread);
				socklen = sizeof(nextHop);
				bzero((char *)&nextHop,sizeof(nextHop));
				nextHop.sin_family = AF_INET;
				nextHop.sin_addr.s_addr = inet_addr("127.0.0.1");
				nextHop.sin_port = htons(circuitDao.next_hop_portNo);
				numOfBytes = sendto(udpsockFD,relayData,nread,0,(struct sockaddr*)&nextHop,socklen);
				ASSERT(numOfBytes > 0);
				routerLogger->writeLine("relay packet, circuit incoming: 0x%02x,",circuitDao.incomingID);
				routerLogger->writeLine(" outgoing: 0x%02x,",circuitDao.outgoingID);
				routerLogger->writeLine(" incoming src: %s, outgoing src: %s",circuitDao.prevHopIP, ifaceAddr);
				routerLogger->writeLine(", dst: %s\n",IP);
			}else{
				routerLogger->writeLine("outgoing packet, circuit incoming: 0x%02x,",circuitDao.incomingID);
				routerLogger->writeLine(" incoming src:%s, outgoing src: %s,",circuitDao.prevHopIP,ifaceAddr);
				routerLogger->writeLine(" dst: %s\n",IP);
				forwardMessage(payload,nread-HEADER_LENGTH,circuitDao.outgoingID);
			}
			break;
		case RELAY_RETURN_DATA:
			routerLogger->writeLine("pkt from port: %d, length: %d, contents: 0x",ntohs(client.sin_port),nread-sizeof(struct iphdr));
			logHex((unsigned char*)message,nread - sizeof(struct iphdr),routerLogger);
			routerLogger->writeLine("\n");
			ControlMessage::extractRelayReturnMessage(message,&circuitID,payload,nread);
			relayBackData(payload, nread-HEADER_LENGTH,circuitID,1);
			break;
		default:
			handleTOREncryptMessage(message,client,nread);
			break;
	}
	//free(payload);
	//free(relayData);
}



void sendToProxy(char* buffer, int nread){

	DEBUG('R',"Router.cc - sendToProxy: Router %d - Bytes to handle %d",routerID,nread);
	char IP[INET6_ADDRSTRLEN];

	struct iphdr *ipHeader = (struct iphdr*)buffer;
	ipHeader->daddr = inet_addr(NetworkUtil::getInterfaceAddress(PROXY_INTERFACE));

	inet_ntop(AF_INET, &ipHeader->daddr, IP, INET6_ADDRSTRLEN);

	ipHeader->check = 0;
	ipHeader->check = csum((unsigned short *)ipHeader, sizeof(struct iphdr)/2);

}


/*
 * This is for ICMP packet response message generation
 * It parses and extract ip headers and imcmp headers.
 * Set icmp packet type to 0 to indicate icmp response.
 * Also, generates checksum and creates appropriate response message
 *
 * */
void handleMessage(char* buffer, int nread){

	char icmpPayload[nread - sizeof(struct iphdr) - sizeof(struct icmphdr)];
	char icmpMessage[nread - sizeof(struct iphdr)];

	DEBUG('S',"router.cc - handleMessage: Received message length %d\n",nread);
	struct iphdr *ipHeader = (struct iphdr*)buffer;

	DEBUG('S',"router.cc - handleMessage: Setting ip header checksum and swapping address\n");
	unsigned int addr = ipHeader->daddr;
	ipHeader->daddr = ipHeader->saddr;
	ipHeader->saddr = addr;
	ipHeader->check = 0;
	ipHeader->check = csum((unsigned short *)ipHeader, sizeof(struct iphdr)/2);

	struct icmphdr icmpHeader;

	DEBUG('S',"router.cc - handleMessage: Setting icmp header and payload to computer checksum\n");
	memset(&icmpHeader,0, sizeof(struct icmphdr));
	memcpy(&icmpHeader,(char*)buffer + sizeof(struct iphdr), sizeof(struct icmphdr));


	memcpy(icmpPayload,(char*)buffer + sizeof(struct iphdr) + sizeof(struct icmphdr),nread - sizeof(struct iphdr) - sizeof(struct icmphdr));

	icmpHeader.type = 0;
	icmpHeader.checksum = 0;

	memcpy(icmpMessage, &icmpHeader, sizeof(struct icmphdr));
	memcpy(icmpMessage + sizeof(struct icmphdr), icmpPayload, nread - sizeof(struct iphdr) - sizeof(struct icmphdr));

	DEBUG('S',"router.cc - handleMessage: computing icmp checksum\n");
	icmpHeader.checksum = csum((unsigned short *)icmpMessage, (nread - sizeof(struct iphdr))/2);

	DEBUG('S',"router.cc - handleMessage: merging icmp header, payload and ip header\n");
	memcpy(icmpMessage, &icmpHeader,sizeof(struct icmphdr));
	memcpy(icmpMessage +sizeof(struct icmphdr), icmpPayload, nread - sizeof(struct iphdr) - sizeof(struct icmphdr));

	DEBUG('S',"router.cc - handleMessage: creating icmp echo response packet. about to finish\n");
	memcpy((char *)&icmpResponse, ipHeader, sizeof (struct iphdr));
	memcpy((char *)&icmpResponse + sizeof(struct iphdr), icmpMessage,nread - sizeof(struct iphdr));

}


void connectToRawSocket(){

	int bindStatus;

	rawsockFD = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

	ASSERT(rawsockFD > 0);

	bzero((char *)&rawEthAddr,sizeof(sockaddr_in));
	rawEthAddr.sin_family = AF_INET;
	rawEthAddr.sin_addr.s_addr = inet_addr(ifaceAddr);

	bindStatus = bind(rawsockFD, (struct sockaddr *) &rawEthAddr, sizeof(sockaddr_in));

	ASSERT(bindStatus != -1);

	DEBUG('R',"router.cc - Router %d : connectToRawSocket IP is %s\n",routerID,ifaceAddr);

}

void connectToTCPRawSocket(){

	int bindStatus;

	tcpRawSockFD = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);

	ASSERT(tcpRawSockFD > 0);

	bzero((char *)&rawTCPEthAddr,sizeof(sockaddr_in));
	rawTCPEthAddr.sin_family = AF_INET;
	rawTCPEthAddr.sin_addr.s_addr = inet_addr(ifaceAddr);

	bindStatus = bind(tcpRawSockFD, (struct sockaddr *) &rawTCPEthAddr, sizeof(sockaddr_in));

	ASSERT(bindStatus != -1);

	DEBUG('R',"router.cc - Router %d : connectToTCPRawSocket IP is %s\n",routerID,ifaceAddr);

}


void connectToProxy(){
	MySocket *socket;
	int bindStatus,status=1,numOfBytes;
	char sendBuffer[BUF_SIZE];

	int pid;

	DEBUG('R',"router.cc - connectToProxy\n");

	pid = getpid();

	proxy_len = sizeof(proxyAddr);

	c_len = sizeof(client);

	socket = new MySocket(SOCK_DGRAM,IPPROTO_UDP);
	socket->setPort("0");
	udpsockFD = socket->getSocket(1);
	ASSERT(udpsockFD != -1);

	if(setsockopt( udpsockFD, SOL_SOCKET, SO_REUSEADDR, &status, sizeof(status)) < 0)
		perror("Cannot set reuseaddr\n");
	bindStatus = bind(udpsockFD,socket->res->ai_addr,socket->res->ai_addrlen);

	ASSERT(bindStatus != -1);

	getsockname(udpsockFD, (struct sockaddr *)&client, &c_len);

	if(stage > 2)
		routerLogger->writeLine("router: %d, pid: %d, port: %d, IP: %s\n",routerID, pid, ntohs(client.sin_port),ifaceAddr);
	else if(stage <= 2)
		routerLogger->writeLine("router: %d, pid: %d, port: %d\n",routerID, pid, ntohs(client.sin_port));

	portNo = ntohs(client.sin_port);

	bzero((char *)&proxyAddr,sizeof(proxyAddr));
	proxyAddr.sin_family = AF_INET;
	proxyAddr.sin_addr.s_addr = client.sin_addr.s_addr;
	proxyAddr.sin_port = htons(proxyPort);

	sprintf(sendBuffer,"%d|%d|%s",routerID,pid,ifaceAddr);

	sleep(2);
	DEBUG('S',"router.cc - routerSubroutine: proxy ip %s port %d\n",inet_ntoa(client.sin_addr),htons(proxyPort));
	numOfBytes = sendto(udpsockFD,sendBuffer,strlen(sendBuffer),0,(struct sockaddr*)&proxyAddr,proxy_len);

	ASSERT(numOfBytes != 0);

}

int isAddressedToMe(char* destIP){
	int octVal[4];
	int octVal1[4];
	sscanf(destIP,"%d.%d.%d.%d",&octVal[0],&octVal[1],&octVal[2],&octVal[3]);
	sscanf(ifaceAddr,"%d.%d.%d.%d",&octVal1[0],&octVal1[1],&octVal1[2],&octVal1[3]);
	if(octVal[0] == 10 && octVal[1] ==  5 && octVal[2] == 51)return TRUE;
	if(octVal[0] == octVal1[0] &&
			octVal[1] == octVal1[1] &&
			octVal[2] == octVal1[2] &&
			octVal[3] == octVal1[3])return TRUE;

	return FALSE;
}

/*
 * This subroutine is for router after forking.
 * it will create a dynamic udp port and will send up message to proxy and will then receive icmp echo messages from
 * proxy.
 *
 * */
void routerSubroutine(int routerid, char* interfaceName, char* interfaceAddr){

	char fileName[40];
	int numOfBytes,nread;

	char buffer[BUF_SIZE];
	char rawBuffer[BUF_SIZE];
	char tcpRawBuffer[BUF_SIZE];
	char srcIP[INET6_ADDRSTRLEN];
	char destIP[INET6_ADDRSTRLEN];
	int type,maxFD;

	routerID = routerid;

	ifaceName = (char*)malloc(sizeof(char)*strlen(interfaceName) + 1);
	ifaceAddr = (char*)malloc(sizeof(char)*strlen(interfaceAddr) + 1);
	memcpy(ifaceName,interfaceName,strlen(interfaceName));
	memcpy(ifaceAddr,interfaceAddr,strlen(interfaceAddr));


	sprintf(fileName,"stage%d.router%d.out",stage,routerID);
	routerLogger = new FileUtil(fileName,WRITE_FILE);

	DEBUG('R',"router.cc - Router %d with interface %s and address %s\n", routerID,ifaceName,ifaceAddr);

	connectToProxy();

	connectToRawSocket();

	connectToTCPRawSocket();

	if(stage < 2)
		return;

	fd_set readSet;
	maxFD = rawsockFD > udpsockFD ? rawsockFD:udpsockFD;
	maxFD = maxFD > tcpRawSockFD ? maxFD:tcpRawSockFD;
	maxFD++;

	struct timeval tmv;
	int status;

	while(TRUE){
		FD_ZERO(&readSet);
		FD_SET(rawsockFD, &readSet);
		FD_SET(udpsockFD, &readSet);
		FD_SET(tcpRawSockFD,&readSet);

		if(stage == 9 && timerFlag == 1){

			Timers_NextTimerTime(&tmv);
			if (tmv.tv_sec == 0 && tmv.tv_usec == 0) {
				// No Timer have been defined
					Timers_ExecuteNextTimer();
				continue;
			}
			if (tmv.tv_sec == MAXVALUE && tmv.tv_usec == 0){
			  /* There are no timers in the event queue */
					break;
			}

			/* The select call here will wait for tv seconds
			 * before expiring You need to modifiy it to listen to
			 * multiple sockets and add code for packet
			 * processing. Refer to the select man pages or "Unix
			 * Network Programming" by R. Stevens Pg 156.
			 */
			status = select(maxFD, &readSet, NULL, NULL, &tmv);

			if (status < 0){
				// This should not happen
				fprintf(stderr, "Select returned %d\n", status);
			} else {
				if (status == 0) {
					/* Timer expired, Hence process it  */
						Timers_ExecuteNextTimer();
					/* Execute all timers that have expired.*/
					Timers_NextTimerTime(&tmv);
					while(tmv.tv_sec == 0 && tmv.tv_usec == 0) {
						/* Timer at the head of the queue has expired  */
							Timers_ExecuteNextTimer();
						Timers_NextTimerTime(&tmv);

					}
					timerFlag = 0;
					Timers_RemoveTimer(timerHandle);
				}
			}
		}else if (select(maxFD , &readSet, NULL, NULL,NULL ) == -1){
			perror("select error");
			ASSERT(0);
		}

		if( FD_ISSET(udpsockFD,&readSet)){
			nread = recvfrom(udpsockFD, buffer,BUF_SIZE ,0,(struct sockaddr *)&client, &c_len);
			ASSERT(nread > 0);
			//DEBUG('R',"router.cc - routerSubroutine: Received message length %d\n",nread);
			//DEBUG('R',"router.cc - routerSubroutine: Before handle message\n");
			if(ControlMessage::isTORMessage(buffer)){
				if(stage == 5 || stage == 6 || stage == 7 || stage == 8 || stage == 9){
					//DEBUG('R',"router.cc - routerSubroutine: Received TOR Message\n");
					handleTORMessage(buffer,client,nread);
				}
			}else{
				NetworkUtil::getIP(srcIP,destIP,&type,buffer,nread);
				routerLogger->writeLine("ICMP from port %d, src: %s, dst: %s, type: %c\n",ntohs(client.sin_port),srcIP,destIP,type);
				if(isAddressedToMe(destIP)){
					handleMessage(buffer,nread);
					DEBUG('R',"router.cc - routerSubroutine: sending message %s\n",icmpResponse);
					numOfBytes = sendto(udpsockFD,icmpResponse,nread,0,(struct sockaddr*)&proxyAddr,proxy_len);
					ASSERT(numOfBytes != 0);
				}else{
					DEBUG('R',"router.cc - routerSubroutine: Router ID %d - Need to forward via raw socket\n",routerID);
					forwardMessage(buffer,nread,0);
				}
			}
		}else if(FD_ISSET(rawsockFD,&readSet)){
			nread = recvfrom(rawsockFD, rawBuffer,BUF_SIZE ,0,(struct sockaddr *)&client, &c_len);
			ASSERT(nread > 0);
			DEBUG('R',"router.cc - RawSocket: Router %d - Received message length %d\n",routerID,nread);
			NetworkUtil::getIP(srcIP,destIP,&type,rawBuffer,nread);
			if(stage == 3 || stage == 4)
				routerLogger->writeLine("ICMP from raw sock, src: %s, dst: %s, type: %c\n",srcIP,destIP,type);
			if(isAddressedToMe(destIP)){
				DEBUG('R',"router.cc - RawSocket: Router %d Destined to me\n",routerID);
				if(stage == 5){
					relayBackData(rawBuffer,nread,1,0);
				}else if(stage == 6 || stage == 7 || stage == 8 || stage == 9){
					relayBackEncryptedData(rawBuffer,nread,1,0);
				}else{
					sendToProxy(rawBuffer,nread);
					numOfBytes = sendto(udpsockFD,rawBuffer,nread,0,(struct sockaddr*)&proxyAddr,proxy_len);
					ASSERT(numOfBytes != 0);
				}
			}
		}else if(FD_ISSET(tcpRawSockFD,&readSet)){
			nread = recvfrom(tcpRawSockFD, tcpRawBuffer,BUF_SIZE ,0,(struct sockaddr *)&client, &c_len);
			ASSERT(nread > 0);
			NetworkUtil::getIP(srcIP,destIP,&type,tcpRawBuffer,nread);
			DEBUG('R',"router.cc - TCP RawSocket: Router %d - Received message length %d\n",routerID,nread);
			printf("router.cc - TCP RawSocket: Router %d - Received message length %d\n",routerID,nread);
			struct iphdr *ipHeader = (struct iphdr*)(tcpRawBuffer);
			struct tcphdr *tcpHeader = (struct tcphdr*)(tcpRawBuffer + (ipHeader->ihl * 4));
			routerLogger->writeLine("incoming TCP packet, src IP/port: %s:%hu, ",srcIP,ntohs(tcpHeader->source));
			routerLogger->writeLine("dst IP/port: %s:%hu, ",destIP,ntohs(tcpHeader->dest));
			routerLogger->writeLine("seqno: %u, ackno: %u\n",ntohl(tcpHeader->seq),ntohl(tcpHeader->ack_seq));

			relayBackEncryptedData(tcpRawBuffer,nread,1,0);

		}
	}

}

