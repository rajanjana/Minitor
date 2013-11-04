/*
 * Author: Rajan Jana
 * email: rjana@usc.edu
 *
 * Course: CSCI 551
 * Project A
 *
 * */

#include "start.h"


FileUtil *configFileReader;
char file[30];

FileUtil *proxyLogger;
int numOfRouters;
int minHops;
uint64_t dieAfterPackets;

/*
 * Initialize command line arguments
 * */
void InitCommandArg(int argc, char** argv){
	for(argc--,argv++; argc > 0; argc--, argv++){
		if(!strcmp(*argv,"+")){ // + to enable debug messages to be printed
			DebugInit(*argv);
		}else
			strcpy(file,*(argv)); // get file name from command line arg
	}
	configFileReader = new FileUtil(file,READ_FILE);

}

/*
 * This is the main function. It does all initialization functions like
 *
 * */
int main(int argc, char** argv){
	InitCommandArg(argc,argv);
	DEBUG('S',"Start.cc - File name %s\n",file);

	int forkedRouters = 0,status;
	char value[10];
	char proxyFileName[100];
	char ifaceName[IFNAMSIZ];

	configFileReader->getValueByKey("stage",value);
	sscanf(value,"%d",&stage);

	configFileReader->getValueByKey("num_routers",value);
	sscanf(value,"%d",&numOfRouters);

	configFileReader->getValueByKey("minitor_hops",value);
	sscanf(value,"%d",&minHops);

	configFileReader->getValueByKey("die_after",value);
	sscanf(value,"%d",&dieAfterPackets);

	sprintf(proxyFileName,"stage%d.proxy.out",stage);

	proxyLogger = new FileUtil(proxyFileName,WRITE_FILE);
	getProxyUp();

	DEBUG('S',"Start.cc - Number of routers %d\n",numOfRouters);
	DEBUG('S',"Start.cc - Min number of hops %d\n",minHops);
	DEBUG('S',"Start.cc - Die after number of packets %d\n",dieAfterPackets);
	routerData = new RouterData(numOfRouters);

	pid_t pID;
	RouterInfo* routerInfo;
	while(forkedRouters < numOfRouters){
		forkedRouters++;
		sprintf(ifaceName,"eth%d",forkedRouters);

		pID = fork();
		if(pID < 0)
			perror("\nFork failed\n");
		if(pID != 0){
			routerInfo = new RouterInfo();
			memcpy(&(routerInfo->iface),(char*)ifaceName,IFNAMSIZ-1);
			memcpy(&(routerInfo->ifaceAddr),(char*)NetworkUtil::getInterfaceAddress(ifaceName),INET6_ADDRSTRLEN);
			routerData->insertRouterData(forkedRouters,routerInfo);
		}
		if(pID == 0){
			DEBUG('S',"Start.cc - Forking router %d with interface %s\n", forkedRouters,ifaceName);
			DEBUG('S',"and address %s\n",NetworkUtil::getInterfaceAddress(ifaceName));
			routerSubroutine(forkedRouters,ifaceName,NetworkUtil::getInterfaceAddress(ifaceName)); // fork router
			return 0;
		}
	}

	proxySubroutine(); // call proxy subroutine

	forkedRouters = 0;
	while(1){
		//wait for forked users to finish so as to avoid main process from terminating
		wait(&status);
		if(WIFEXITED (status))
			forkedRouters++;
		if(forkedRouters == numOfRouters)
			break;
	}
	return 0;

}
