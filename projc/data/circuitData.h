/*
 * Author: Rajan Jana
 * email: rjana@usc.edu
 *
 * Course: CSCI 551
 * Project B
 *
 * provides APIs for circuit access information for proxy
 *
 * */

#ifndef CIRCUIT_DATA_H_
#define CIRCUIT_DATA_H_

#include <map>
#include <stdio.h>
#include <limits.h>
#include "../encrypt/keyHandler.h"

using namespace std;

class CircuitInfo{

public:
	int circuitID;
	int minHops;
	int* routerIDs;
	int pendingHops;
	KeyHandler *keyHandler;
	//CircuitInfo(CircuitInfo &info);
};


class CircuitData{
public:
		CircuitData();
		CircuitInfo getCircuitInfo(int index);
		void insertCircuitInfo(int circuitID,CircuitInfo info);
		int getTotalCircuits();
		void print(CircuitInfo info);
private:
		map<int,CircuitInfo> circuitMap;
};


#endif
