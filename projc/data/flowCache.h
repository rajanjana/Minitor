#ifndef FLOW_CACHE_H
#define FLOW_CACHE_H

using namespace std;

#include "../utility/socket.h"
#include <map>


class FlowEntry{
	public:
		uint32_t sourceIP;
		uint32_t destIP;

		uint16_t sourcePort;
		uint16_t destPort;
		uint8_t protocol;
		uint32_t circuitID;
		uint64_t packetSent;

};

class FlowCache{
	private:
		map<int,FlowEntry> flowCache;
	public:
		void insertFlowEntry(int,FlowEntry&);
		void insertFlowEntry(int,uint32_t,uint32_t,uint16_t,uint16_t,uint8_t);
		bool lookUp(FlowEntry&);
		int lookUp(uint32_t,uint32_t,uint16_t,uint16_t,uint8_t);
		bool compare(FlowEntry&,FlowEntry&);
		bool compare(FlowEntry&,uint32_t,uint32_t,uint16_t,uint16_t,uint8_t);
		int getSize();
		int updateFlowEntry(uint32_t,uint32_t,uint16_t,uint16_t,uint8_t);
		void deleteFlowEntry(uint32_t,uint32_t,uint16_t,uint16_t,uint8_t);
};

#endif
