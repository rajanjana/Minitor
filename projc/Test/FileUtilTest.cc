/*
 * Author: Rajan Jana
 * email: rjana@usc.edu
 *
 * Course: CSCI 551
 * Project B
 *
 * */

#include "../utility/fileUtil.h"
#include "../utility/utility.h"


char fileName[100];

void InitCommandArg(int argc, char** argv){
	for(argc--,argv++; argc > 0; argc--, argv++){
		if(!strcmp(*argv,"+")){ // + to enable debug messages to be printed
			DebugInit(*argv);
		}else
			strcpy(fileName,*(argv)); // get file name from command line arg
	}
}


int main(int argc, char** argv){
	InitCommandArg(argc,argv);

	char value[10];

		int a = 10;
	DEBUG('S',"FileUtilTest.cc - File name %s\n",fileName);

	FileUtil *fileUtil = new FileUtil("stage1.proxy.out",WRITE_FILE);
	fileUtil->writeLine("%d %d\n",a,a);
	delete fileUtil;
	//fileUtil->getValueByKey("mayur",value);
	//printf("Value is %s\n",value);

}
