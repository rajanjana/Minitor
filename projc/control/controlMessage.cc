/*
 * Author: Rajan Jana
 * email: rjana@usc.edu
 *
 * Course: CSCI 551
 * Project B
 *
 * This class provides implementation for the TOR control message APIs as declared in controlmessage.h
 *
 *
 * */

#include "controlMessage.h"
#include "../utility/networkUtil.h"

void ControlMessage::getCircuitExtendMessage(int circuitID, int portNo, char* message){

	struct iphdr ipHeader;
	bzero(&ipHeader,sizeof(struct iphdr));
	ipHeader.saddr = inet_addr(NetworkUtil::getInterfaceAddress("lo"));
	ipHeader.daddr = inet_addr(NetworkUtil::getInterfaceAddress("lo"));
	ipHeader.protocol = PROTOCOL_ID;

	memcpy(message,(char*)&ipHeader,sizeof(struct iphdr));
	*(char*)(message + sizeof(struct iphdr)) = CIRCUIT_EXTEND;
	*(unsigned short*)(message + sizeof(struct iphdr) + 1) = (unsigned short)htons(circuitID);
	*(unsigned short*)(message + sizeof(struct iphdr) + 3) = (unsigned short)htons(portNo);
}

void ControlMessage::getCircuitExtendDoneMessage(int circuitID, char* message){
	struct iphdr ipHeader;
	bzero(&ipHeader,sizeof(struct iphdr));
	ipHeader.saddr = inet_addr(NetworkUtil::getInterfaceAddress("lo"));
	ipHeader.daddr = inet_addr(NetworkUtil::getInterfaceAddress("lo"));
	ipHeader.protocol = PROTOCOL_ID;

	memcpy(message,(char*)&ipHeader,sizeof(struct iphdr));
	*(char*)(message + sizeof(struct iphdr)) = CIRCUIT_EXTEND_DONE;
	*(unsigned short*)(message + sizeof(struct iphdr) + 1) = (unsigned short)htons(circuitID);
}


int ControlMessage::getMessageType(char* message){
	return *((char*)(message+sizeof(struct iphdr)));
}

int ControlMessage::isTORMessage(char* message){
	struct iphdr *ipHeader = (struct iphdr*)message;
	if(ipHeader->protocol == PROTOCOL_ID)
		return 1;
	else
		return 0;
}


void ControlMessage::extractCircuitExtendMessage(char* message, int* circuitID, int *portNo){
	*circuitID = ntohs((*(unsigned short*)(message + sizeof(struct iphdr) + 1)));
	*portNo = ntohs((*(unsigned short*)(message + sizeof(struct iphdr) + 3)));
}


void ControlMessage::extractCircuitExtendDoneMessage(char* message, int *circuitID){
	*circuitID = ntohs((*(unsigned short*)(message + sizeof(struct iphdr) + 1)));
}

void ControlMessage::getRelayMessage(int circuitID, char* message, char* icmpPayload, int length){
	struct iphdr ipHeader;
	bzero(&ipHeader,sizeof(struct iphdr));
	ipHeader.saddr = inet_addr(NetworkUtil::getInterfaceAddress("lo"));
	ipHeader.daddr = inet_addr(NetworkUtil::getInterfaceAddress("lo"));
	ipHeader.protocol = PROTOCOL_ID;

	memcpy(message,(char*)&ipHeader,sizeof(struct iphdr));
	*(char*)(message + sizeof(struct iphdr)) = RELAY_DATA;
	*(unsigned short*)(message + sizeof(struct iphdr) + 1) = (unsigned short)htons(circuitID);

	memcpy((char*)message+HEADER_LENGTH,(char*)icmpPayload,length);
}

void ControlMessage::extractRelayMessage(char* message, int* circuitID, char* payload, int nread){
	*circuitID = ntohs((*(unsigned short*)(message + sizeof(struct iphdr) + 1)));
	memcpy(payload, message+HEADER_LENGTH, nread - sizeof(struct iphdr) - 1 - 2);
}


void ControlMessage::getRelayReturnMessage(int circuitID, char* message, char* icmpPayload, int length){
	struct iphdr ipHeader;
	bzero(&ipHeader,sizeof(struct iphdr));
	ipHeader.saddr = inet_addr(NetworkUtil::getInterfaceAddress("lo"));
	ipHeader.daddr = inet_addr(NetworkUtil::getInterfaceAddress("lo"));
	ipHeader.protocol = PROTOCOL_ID;

	memcpy(message,(char*)&ipHeader,sizeof(struct iphdr));
	*(char*)(message + sizeof(struct iphdr)) = RELAY_RETURN_DATA;
	*(unsigned short*)(message + sizeof(struct iphdr) + 1) = (unsigned short)htons(circuitID);
	memcpy((char*)message+HEADER_LENGTH,(char*)icmpPayload,length);
}

void ControlMessage::extractRelayReturnMessage(char* message, int* circuitID, char* payload, int nread){
	*circuitID = ntohs((*(unsigned short*)(message + sizeof(struct iphdr) + 1)));
	memcpy(payload, message+HEADER_LENGTH, nread - sizeof(struct iphdr) - 1 - 2);
}


void ControlMessage::getFDHMessage(int circuitID, unsigned char* keyText, char* message, int msgLen){
	struct iphdr ipHeader;
	bzero(&ipHeader,sizeof(struct iphdr));
	ipHeader.saddr = inet_addr(NetworkUtil::getInterfaceAddress("lo"));
	ipHeader.daddr = inet_addr(NetworkUtil::getInterfaceAddress("lo"));
	ipHeader.protocol = PROTOCOL_ID;

	memcpy(message,(char*)&ipHeader,sizeof(struct iphdr));
	*(char*)(message + sizeof(struct iphdr)) = FDH_MESSAGE;
	*(unsigned short*)(message + sizeof(struct iphdr) + 1) = (unsigned short)htons(circuitID);
	memcpy(message + sizeof(struct iphdr) + 3, keyText,msgLen);
}

void ControlMessage::extractFDHMessage(char* message, int* circuitID, unsigned char* keyText, int nread){
	*circuitID = ntohs((*(unsigned short*)(message + sizeof(struct iphdr) + 1)));
	memcpy(keyText,message+HEADER_LENGTH,nread-HEADER_LENGTH);
}

void ControlMessage::getEncryptCircuitExtendMessage(int circuitID, unsigned char* portNo, int portLength, char* message){
	struct iphdr ipHeader;
	bzero(&ipHeader,sizeof(struct iphdr));
	ipHeader.saddr = inet_addr(NetworkUtil::getInterfaceAddress("lo"));
	ipHeader.daddr = inet_addr(NetworkUtil::getInterfaceAddress("lo"));
	ipHeader.protocol = PROTOCOL_ID;

	memcpy(message,(char*)&ipHeader,sizeof(struct iphdr));
	*(char*)(message + sizeof(struct iphdr)) = ENCRYPT_CIRCUIT_EXTEND;
	*(unsigned short*)(message + sizeof(struct iphdr) + 1) = (unsigned short)htons(circuitID);
	memcpy(message + sizeof(struct iphdr) + 3, portNo,portLength);
}

void ControlMessage::extractEncryptCircuitExtendMessage(char* message, int *circuitID, unsigned char* portData,int nread){
	*circuitID = ntohs((*(unsigned short*)(message + sizeof(struct iphdr) + 1)));
	memcpy(portData,message+HEADER_LENGTH,nread-HEADER_LENGTH);
}

void ControlMessage::getEncryptCircuitExtendDoneMessage(int circuitID, char* message){
	struct iphdr ipHeader;
	bzero(&ipHeader,sizeof(struct iphdr));
	ipHeader.saddr = inet_addr(NetworkUtil::getInterfaceAddress("lo"));
	ipHeader.daddr = inet_addr(NetworkUtil::getInterfaceAddress("lo"));
	ipHeader.protocol = PROTOCOL_ID;

	memcpy(message,(char*)&ipHeader,sizeof(struct iphdr));
	*(char*)(message + sizeof(struct iphdr)) = ENCRYPT_CIRCUIT_EXTEND_DONE;
	*(unsigned short*)(message + sizeof(struct iphdr) + 1) = (unsigned short)htons(circuitID);
}

void ControlMessage::getEncryptRelayMessage(int circuitID, char* relayData, int length, char* message){
	struct iphdr ipHeader;
	bzero(&ipHeader,sizeof(struct iphdr));
	ipHeader.saddr = inet_addr(NetworkUtil::getInterfaceAddress("lo"));
	ipHeader.daddr = inet_addr(NetworkUtil::getInterfaceAddress("lo"));
	ipHeader.protocol = PROTOCOL_ID;

	memcpy(message,(char*)&ipHeader,sizeof(struct iphdr));
	*(char*)(message + sizeof(struct iphdr)) = ENCRYPT_RELAY_DATA;
	*(unsigned short*)(message + sizeof(struct iphdr) + 1) = (unsigned short)htons(circuitID);

	memcpy((char*)message+HEADER_LENGTH,(char*)relayData,length);
}

void ControlMessage::extractEncryptRelayMessage(char* message, int* circuitID, unsigned char* payload, int nread){
	*circuitID = ntohs((*(unsigned short*)(message + sizeof(struct iphdr) + 1)));
	memcpy(payload, message+HEADER_LENGTH, nread - sizeof(struct iphdr) - 1 - 2);
}


void ControlMessage::getEncryptRelayReturnMessage(int circuitID, char* message, char* payload, int length){
	struct iphdr ipHeader;
	bzero(&ipHeader,sizeof(struct iphdr));
	ipHeader.saddr = inet_addr(NetworkUtil::getInterfaceAddress("lo"));
	ipHeader.daddr = inet_addr(NetworkUtil::getInterfaceAddress("lo"));
	ipHeader.protocol = PROTOCOL_ID;

	memcpy(message,(char*)&ipHeader,sizeof(struct iphdr));
	*(char*)(message + sizeof(struct iphdr)) = ENCRYPT_RETURN_DATA;
	*(unsigned short*)(message + sizeof(struct iphdr) + 1) = (unsigned short)htons(circuitID);
	memcpy((char*)message+HEADER_LENGTH,(char*)payload,length);
}


void ControlMessage::extractEncryptRelayReturnMessage(char* message, int *circuitID, char* payload, int nread){
	*circuitID = ntohs((*(unsigned short*)(message + sizeof(struct iphdr) + 1)));
	memcpy(payload, message+HEADER_LENGTH, nread - sizeof(struct iphdr) - 1 - 2);
}

void ControlMessage::extractEncryptCircuitExtendDoneMessage(char* message, int *circuitID){
	*circuitID = ntohs((*(unsigned short*)(message + sizeof(struct iphdr) + 1)));
}

int ControlMessage::getCircuitID(char* message){
	return ntohs((*(unsigned short*)(message + sizeof(struct iphdr) + 1)));
}

void ControlMessage::getRouterKillMessage(char * message){
	struct iphdr ipHeader;
	bzero(&ipHeader,sizeof(struct iphdr));
	ipHeader.saddr = inet_addr(NetworkUtil::getInterfaceAddress("lo"));
	ipHeader.daddr = inet_addr(NetworkUtil::getInterfaceAddress("lo"));
	ipHeader.protocol = PROTOCOL_ID;

	memcpy(message,(char*)&ipHeader,sizeof(struct iphdr));
	*(char*)(message + sizeof(struct iphdr)) = KILL_ROUTER;
}


void ControlMessage::getRouterWorryMessage(char* message, int circuitID, int myPort, int nextHop){
	struct iphdr ipHeader;
	bzero(&ipHeader,sizeof(struct iphdr));
	ipHeader.saddr = inet_addr(NetworkUtil::getInterfaceAddress("lo"));
	ipHeader.daddr = inet_addr(NetworkUtil::getInterfaceAddress("lo"));
	ipHeader.protocol = PROTOCOL_ID;

	memcpy(message,(char*)&ipHeader,sizeof(struct iphdr));
	*(char*)(message + sizeof(struct iphdr)) = WORRY_ROUTER;
	*(unsigned short*)(message + sizeof(struct iphdr) + 1) = (unsigned short)htons(circuitID);
	*(unsigned short*)(message + sizeof(struct iphdr) + 3) = (unsigned short)htons(myPort);
	*(unsigned short*)(message + sizeof(struct iphdr) + 5) = (unsigned short)htons(nextHop);
}

void ControlMessage::extractRouterWorryMessage(char* message, uint16_t* portNo, uint16_t* hopPortNo){
	*portNo = ntohs((*(unsigned short*)(message)));
	*hopPortNo = ntohs((*(unsigned short*)(message + 2)));
}



