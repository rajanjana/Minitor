/*
 * Author: Rajan Jana
 * email: rjana@usc.edu
 *
 * Course: CSCI 551
 * Project B
 *
 * Defines all message formats for Tor.
 * Provides APIs to generate and extract TOR control messages
 *
 *
 * */

#ifndef CONTROL_MESSAGE_H_
#define CONTROL_MESSAGE_H

#include <linux/ip.h>
#include <stdint.h>

#define PROTOCOL_ID 253

#define TOR_PAYLOAD 0x51
#define CIRCUIT_EXTEND 0x52
#define CIRCUIT_EXTEND_DONE 0x53
#define RELAY_DATA 0x51
#define RELAY_RETURN_DATA 0x54

#define ENCRYPT_RELAY_DATA 0x61
#define ENCRYPT_CIRCUIT_EXTEND 0x62
#define ENCRYPT_CIRCUIT_EXTEND_DONE 0x63
#define ENCRYPT_RETURN_DATA 0x64
#define FDH_MESSAGE 0x65

#define KILL_ROUTER 0x91
#define WORRY_ROUTER 0X92

#define HEADER_LENGTH 23

#define HEADER_LENGTH_WITH_PORT 25
#define HEADER_LENGTH_WITH_ENCRYPT_MESSAGE 39
#define HEADER_LENGTH_WITHOUT_PAYLOAD 21

class ControlMessage{

public:

	static void getCircuitExtendMessage(int circuitID, int portNo,char* message);
	static void getCircuitExtendDoneMessage(int circuitID, char* message);
	static void getRelayMessage(int circuitID, char* relayData, char*message, int length);
	static void getRelayReturnMessage(int circuitID, char* relayReturn, char* message, int length);
	static int getMessageType(char* message);
	static void extractCircuitExtendMessage(char* message, int* circuitID, int* portNo);
	static void extractCircuitExtendDoneMessage(char* message, int* circuitID);
	static void extractRelayMessage(char*message, int* circuitID,char*payload, int nread);
	static void extractRelayReturnMessage(char*message, int* circuitID, char* payload, int nread);

	static void getFDHMessage(int circuitID, unsigned char* keyText, char* message, int msgLen);
	static void extractFDHMessage(char* message, int *circuitID, unsigned char* keyText, int nread);

	static void getEncryptCircuitExtendMessage(int circuitID,unsigned char* portNo,int portLength,char* message);
	static void extractEncryptCircuitExtendMessage(char* message, int *circuitID, unsigned char* portData,int nread);

	static void getEncryptCircuitExtendDoneMessage(int circuitID, char* message);
	static void extractEncryptCircuitExtendDoneMessage(char* message, int* circuitID);

	static void getEncryptRelayMessage(int circuitID, char* relayData,int length,char* message);
	static void extractEncryptRelayMessage(char*message, int* circuitID,unsigned char*payload, int nread);

	static void getEncryptRelayReturnMessage(int circuitID, char* relayReturn, char* payload, int length);
	static void extractEncryptRelayReturnMessage(char* message, int* circuitID, char* payload, int nread);

	static int isTORMessage(char* buffer);
	static int getCircuitID(char* buffer);

	static void getRouterKillMessage(char* message);
	static void getRouterWorryMessage(char* message, int circuitID,int,int);
	static void extractRouterWorryMessage(char* message,uint16_t *fPort, uint16_t *nPort);

};


#endif
