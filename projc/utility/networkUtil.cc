/*
 * Author: Rajan Jana
 * email: rjana@usc.edu
 *
 * Course: CSCI 551
 * Project B
 *
 * */

#include "networkUtil.h"


char* NetworkUtil:: getInterfaceAddress(const char *interfaceName){
	struct ifreq ifr;

	int sockFD = socket(AF_INET, SOCK_DGRAM, 0);
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, interfaceName, IFNAMSIZ-1);
	ioctl(sockFD, SIOCGIFADDR, &ifr);

	return inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
}

void NetworkUtil::printPacket(unsigned char* packet, int length){
	for (int var = 0; var < length; ++var) {
		if(!(var % 8))
			printf("\n");
		printf("%02x\t",packet[var]);
	}
	printf("\n");
}


void NetworkUtil::getIP(char* srcIP, char *destIP,int *type,char *buffer,int len){
	struct iphdr *ipHeader = (struct iphdr*)buffer;
	struct icmphdr *icmpHeader;
	char icmpPacket[BUF_SIZE];

	if(srcIP != NULL){
		memset(srcIP,0,INET6_ADDRSTRLEN);
		inet_ntop(AF_INET, &ipHeader->saddr, srcIP, INET6_ADDRSTRLEN);
	}

	if(destIP != NULL){
		memset(destIP,0,INET6_ADDRSTRLEN);
		inet_ntop(AF_INET, &ipHeader->daddr, destIP, INET6_ADDRSTRLEN);
	}

	memcpy(icmpPacket, buffer+sizeof(iphdr),len-sizeof(iphdr));
	icmpHeader=(struct icmphdr*)icmpPacket;
	*type = icmpHeader->type+48;
}

int NetworkUtil::isTCPPacket(unsigned char* packet){
	struct iphdr *ipHeader = (struct iphdr*)packet;

	return ipHeader->protocol == IPPROTO_TCP;
}

uint8_t NetworkUtil::getProtocol(unsigned char* packet){
	struct iphdr *ipHeader = (struct iphdr*)packet;

	return ipHeader->protocol;
}

void NetworkUtil::parseTCPPacket(unsigned char* packet, uint16_t* sPort,uint16_t* dPort){
	struct iphdr *ipheader = (struct iphdr*)packet;
	struct tcphdr *tcpHeader = (struct tcphdr*)(packet + (ipheader->ihl * 4));

	*sPort = ntohs(tcpHeader->source);
	*dPort = ntohs(tcpHeader->dest);
}



