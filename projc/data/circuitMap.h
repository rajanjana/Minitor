/*
 * Author: Rajan Jana
 * email: rjana@usc.edu
 *
 * Course: CSCI 551
 * Project B
 *
 *
 * APIs to store and access circuit information for routers
 *
 *
 * */

#ifndef CIRCUIT_MAP_H_
#define CIRCUIT_MAP_H_

#include <map>
#include <stdio.h>
#include "../utility/socket.h"
#include "../encrypt/keyHandler.h"

#define TRUE 1
#define FALSE 0

using namespace std;


class CircuitDao{

	public:
		int incomingID;
		int next_hop_portNo;
		int prev_hop_portNo;
		int isLastHop;
		int outgoingID;
		struct sockaddr_in prev;
		char prevHopIP[INET6_ADDRSTRLEN];
		KeyHandler keyHandler;
		int timerHandle;
};


class CircuitMap{

public:

	CircuitMap();
	CircuitDao getIncomingCircuitDao(int iCircuitID);
	CircuitDao getOutgoingCircuitDao(int oCircuitID);
	void insertIncomingCircuitDao(int iCircuitID,CircuitDao info);
	void insertOutgoingCircuitDao(int oCircuitID,CircuitDao info);
	int isNewIncomingCircuit(int iCircuitID);
	int isNewOutgoingCircuit(int oCircuitID);
	void iPrint(int iCircuitID);
	void oPrint(int oCircuitID);

	private:
		map<int,CircuitDao> iCircuitMap; //map based on incoming circuit id key
		map<int,CircuitDao> oCircuitMap; // map based on outgoing circuit id key

};

#endif
