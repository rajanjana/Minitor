/*
 * Author: Rajan Jana
 * email: rjana@usc.edu
 *
 * Course: CSCI 551
 * Project B
 *
 * */

#ifndef NETWORK_UTILITY_H
#define NETWORK_UTILITY_H

#include "socket.h"
#include <linux/ip.h>
#include <linux/icmp.h>
#include <netinet/tcp.h>

#define BUF_SIZE 4096


struct pseudo_hdr {
	u_int32_t src;          /* 32bit source ip address*/
	u_int32_t dst;          /* 32bit destination ip address */
	u_char mbz;             /* 8 reserved bits (all 0) 	*/
	u_char proto;           /* protocol field of ip header */
	u_int16_t len;          /* tcp length (both header and data */
};

class NetworkUtil{
	public:
		static char* getInterfaceAddress(const char*);
		static void printPacket(unsigned char* packet, int length);
		static void getIP(char* srcIP, char *destIP,int *type,char *buffer,int len);
		static int isTCPPacket(unsigned char* packet);
		static void parseTCPPacket(unsigned char* packet, uint16_t* sPort, uint16_t* dPort);
		static uint8_t getProtocol(unsigned char* packet);

};

#endif
