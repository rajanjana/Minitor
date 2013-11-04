/*
 * Author: Rajan Jana
 * email: rjana@usc.edu
 *
 * Course: CSCI 551
 * Project B
 *
 *
 * Test function to validate circuit data access object functionality.
 *
 * */

#include "../data/circuitData.h"
#include <stdio.h>

void print(CircuitInfo info){
	printf("Circuit ID = %d\n",info.circuitID);
	printf("Min hops = %d\n",info.minHops);
	printf("Pending hops = %d\n",info.pendingHops);
	printf("Router ID = %d\n",info.routerIDs[0]);
}

int main(void){
	CircuitData *data = new CircuitData();

	CircuitInfo info1;
	info1.circuitID = 1;
	info1.minHops = 3;
	info1.pendingHops = 3;
	info1.routerIDs = new int[info1.minHops];
	info1.routerIDs[0] = 3;

	data->insertCircuitInfo(info1.circuitID,info1);

	print(data->getCircuitInfo(1));

	CircuitInfo info2;
	info2.circuitID = 2;
	info2.minHops = 5;
	info2.pendingHops = 5;
	info2.routerIDs = new int[info2.minHops];
	info2.routerIDs[0] = 5;

	data->insertCircuitInfo(info2.circuitID,info2);

	print(data->getCircuitInfo(2));

	CircuitInfo info3 = data->getCircuitInfo(2);
	info3.routerIDs[0] = 10;

	data->insertCircuitInfo(info3.circuitID,info3);

	print(data->getCircuitInfo(2));


	printf("Total circuits = %d\n",data->getTotalCircuits());

}
