/*
 * Author: Rajan Jana
 * email: rjana@usc.edu
 *
 * Course: CSCI 551
 * Project B
 *
 * API implementation for router access information
 *
 * */

#include "routerData.h"
#include "../utility/utility.h"

RouterData::RouterData(int len){
	length = len;
	length = length + 2;
	routerInfo = new RouterInfo[length];
}

RouterData::~RouterData(){

}

void RouterData::insertRouterData(int index, RouterInfo* rInfo){
	ASSERT(index >=0 && index < length);
	routerInfo[index] = *rInfo;
}

RouterInfo* RouterData::getRouterInformation(int index){
	ASSERT(index >=0 && index < length);
	return &routerInfo[index];
}

void RouterData::print(){
	for(int i=1;i<length-1;i++){
		printf("Router = %d Port no  = %d\n",i+1,routerInfo[i].portNo);
	}
}
