/*
 * Author: Rajan Jana
 * email: rjana@usc.edu
 *
 * Course: CSCI 551
 * Project B
 *
 * API implementation to store and access information about circuits for routers
 *
 *
 * */

#include "circuitMap.h"

CircuitMap::CircuitMap(){

}

void CircuitMap::insertIncomingCircuitDao(int iCircuitID,CircuitDao info){
	iCircuitMap[iCircuitID] = info;
}

void CircuitMap::insertOutgoingCircuitDao(int oCircuitID, CircuitDao info){
	oCircuitMap[oCircuitID] = info;
}

CircuitDao CircuitMap::getIncomingCircuitDao(int iCircuitID){
	return iCircuitMap[iCircuitID];
}

CircuitDao CircuitMap::getOutgoingCircuitDao(int oCircuitID){
	return oCircuitMap[oCircuitID];
}

int CircuitMap::isNewIncomingCircuit(int iCircuitID){
	if(iCircuitMap.find(iCircuitID) == iCircuitMap.end())
		return TRUE;
	return FALSE;
}

int CircuitMap::isNewOutgoingCircuit(int oCircuitID){
	if(oCircuitMap.find(oCircuitID) == oCircuitMap.end())
		return TRUE;
	return FALSE;
}

void CircuitMap::iPrint(int iCircuitID){
	CircuitDao dao = iCircuitMap[iCircuitID];
	printf("Incoming id = %x\n",dao.incomingID);
	printf("Outgoing id = %x\n",dao.outgoingID);
	printf("Next port no = %d\n",dao.next_hop_portNo);
	printf("Previous port no = %d\n",dao.prev_hop_portNo);
}

void CircuitMap::oPrint(int oCircuitID){
	CircuitDao dao = oCircuitMap[oCircuitID];
	printf("Incoming id = %x\n",dao.incomingID);
	printf("Outgoing id = %x\n",dao.outgoingID);
	printf("Next port no = %d\n",dao.next_hop_portNo);
	printf("Previous port no = %d\n",dao.prev_hop_portNo);
}


