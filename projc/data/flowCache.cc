#include "flowCache.h"


bool FlowCache::compare(FlowEntry &entry1,FlowEntry &entry2){
	return (entry1.destIP == entry2.destIP &&
			entry1.destPort == entry2.destPort &&
			entry1.protocol == entry2.protocol &&
			entry1.sourceIP == entry2.sourceIP &&
			entry1.destIP == entry2.destIP);
}

bool FlowCache::compare(FlowEntry& entry,uint32_t srcIP,uint32_t dstIP,uint16_t sPort,uint16_t dPort,uint8_t protocol){
	return (entry.destIP == dstIP &&
			entry.destPort == dPort &&
			entry.protocol == protocol &&
			entry.sourceIP == srcIP &&
			entry.sourcePort == sPort);
}


bool FlowCache::lookUp(FlowEntry& flowEntry){
	map<int, FlowEntry>::iterator mapItr;
	for(mapItr=flowCache.begin();mapItr!=flowCache.end();mapItr++){
		if(compare((*mapItr).second, flowEntry))
			return 1;
	}
	return 0;
}

int FlowCache::getSize(){
	return flowCache.size();
}

void FlowCache::insertFlowEntry(int position,FlowEntry& entry){
	flowCache[position] = entry;
}


int FlowCache::lookUp(uint32_t srcIP, uint32_t destIP, uint16_t sPort, uint16_t dPort, uint8_t protocol){
	map<int, FlowEntry>::iterator mapItr;
	for(mapItr=flowCache.begin();mapItr!=flowCache.end();mapItr++){
		if(compare((*mapItr).second,srcIP,destIP,sPort,dPort,protocol))
			return (*mapItr).second.circuitID;
	}
	return -1;
}

void FlowCache::insertFlowEntry(int circuitID,uint32_t srcIP, uint32_t destIP, uint16_t sPort, uint16_t dPort, uint8_t protocol){
	FlowEntry entry;
	entry.destIP = destIP;
	entry.destPort = dPort;
	entry.protocol = protocol;
	entry.sourceIP = srcIP;
	entry.sourcePort = sPort;
	entry.circuitID = circuitID;
	entry.packetSent = 0;
	flowCache[flowCache.size()] = entry;
}

int FlowCache::updateFlowEntry(uint32_t srcIP, uint32_t destIP, uint16_t sPort, uint16_t dPort, uint8_t protocol){
	map<int, FlowEntry>::iterator mapItr;
	for(mapItr=flowCache.begin();mapItr!=flowCache.end();mapItr++){
		if(compare((*mapItr).second,srcIP,destIP,sPort,dPort,protocol)){
			(*mapItr).second.packetSent++;
			return (*mapItr).second.packetSent;
		}
	}
	return 0;
}

void FlowCache::deleteFlowEntry(uint32_t srcIP, uint32_t destIP, uint16_t sPort, uint16_t dPort, uint8_t protocol){
	map<int, FlowEntry>::iterator mapItr;
	for(mapItr=flowCache.begin();mapItr!=flowCache.end();mapItr++){
		if(compare((*mapItr).second,srcIP,destIP,sPort,dPort,protocol)){
			flowCache.erase(mapItr);
			break;
		}
	}
}
