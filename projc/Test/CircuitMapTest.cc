/*
 * Author: Rajan Jana
 * email: rjana@usc.edu
 *
 * Course: CSCI 551
 * Project B
 *
 * Test file to validate the functionality of circuit map access object.
 *
 * */

#include <stdio.h>
#include "../data/circuitMap.h"


void print(CircuitDao dao){
	printf("Incoming id = %d\n",dao.incomingID);
	printf("Outgoing id = %d\n",dao.outgoingID);
	printf("Next hop port = %d\n",dao.next_hop_portNo);
	printf("Previous port = %d\n",dao.prev_hop_portNo);

	printf("\n");
}

int main(void){
	CircuitMap cirMap;

	CircuitDao info1;
	info1.incomingID = 0x01;
	info1.outgoingID = 0x101;
	info1.next_hop_portNo =  12345;
	info1.prev_hop_portNo = 12311;

	cirMap.insertIncomingCircuitDao(info1.incomingID,info1);
	cirMap.insertOutgoingCircuitDao(info1.outgoingID,info1);

	print(cirMap.getIncomingCircuitDao(0x01));
	print(cirMap.getOutgoingCircuitDao(0x101));

}
