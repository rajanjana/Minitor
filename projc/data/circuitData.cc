/*
 * Author: Rajan Jana
 * email: rjana@usc.edu
 *
 * Course: CSCI 551
 * Project B
 *
 *
 * This file contains API to hold information about each and every circuit that the
 * proxy generates. It keeps information like routers associated with particular circuits.
 * It provides easy access to insert, update and fetch circuit information.
 *
 *
 * */

#include "circuitData.h"

using namespace std;

/*CircuitInfo::CircuitInfo(CircuitInfo &info){
	circuitID = info.circuitID;
	minHops = info.minHops;
	routerIDs = info.routerIDs;
	pendingHops = info.pendingHops;
}*/

CircuitData::CircuitData(){

}

CircuitInfo CircuitData::getCircuitInfo(int index){
	return circuitMap[index];
}

void CircuitData::insertCircuitInfo(int circuitID,CircuitInfo info){
	circuitMap[circuitID] = info;
}

int CircuitData::getTotalCircuits(){
	return circuitMap.size();
}

void CircuitData::print(CircuitInfo info){
	printf("Circuit ID = %d\n",info.circuitID);
	printf("Min hops = %d\n",info.minHops);
	printf("Pending hops = %d\n",info.pendingHops);
	for(int i=0;i<info.minHops;i++)
		printf("Router ID = %d\n",info.routerIDs[i]);
}




