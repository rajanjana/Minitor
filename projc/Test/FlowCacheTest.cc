
#include "../data/flowCache.h"

int main(void){
	FlowCache flowCache;

	FlowEntry * entry = new FlowEntry();

	entry->destIP = 10;
	entry->sourceIP = 20;
	entry->sourcePort = 30;
	entry->destPort = 40;
	entry->protocol = 6;

	flowCache.insertFlowEntry(flowCache.getSize(),*entry);

	entry = new FlowEntry();
	entry->destIP = 50;
	entry->sourceIP = 60;
	entry->sourcePort = 70;
	entry->destPort = 80;
	entry->protocol = 6;

	flowCache.insertFlowEntry(flowCache.getSize(),*entry);

	entry = new FlowEntry();
	entry->destIP = 10;
	entry->sourceIP = 20;
	entry->sourcePort = 30;
	entry->destPort = 40;
	entry->protocol = 6;

	if(flowCache.lookUp(*entry)){
		printf("Success\n");
	}

	entry = new FlowEntry();
		entry->destIP = 10;
		entry->sourceIP = 20;
		entry->sourcePort = 30;
		entry->destPort = 40;
		entry->protocol = 6;

		if(!flowCache.lookUp(*entry)){
				printf("no Success\n");
			}else
				printf("success\n");

		int circuitID;

		if((circuitID = flowCache.lookUp(10,20,5,6,17)) == -1){
			circuitID = 0x01;
			printf("INserting\n");
			flowCache.insertFlowEntry(circuitID,10,20,5,6,17);
		}

		if((circuitID = flowCache.lookUp(10,20,5,6,17)) == -1){
					circuitID = 0x01;
					printf("INserting\n");
					flowCache.insertFlowEntry(circuitID,10,20,5,6,17);
				}else{
					printf("CIrcuit ID = %d\n",circuitID);
				}



}
