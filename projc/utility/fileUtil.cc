/*
 * Author: Rajan Jana
 * email: rjana@usc.edu
 *
 * Course: CSCI 551
 * Project A
 *
 * */

#include "fileUtil.h"

FileUtil::FileUtil(char *fileName, int modeFlag){

	DEBUG('S',"FileUtil.cc - File name %s\n",fileName);
	switch(modeFlag){
		case READ_FILE:
					fp = fopen(fileName,"r+");
					if(fp == NULL)
						perror("Cannot open file.\n");
					break;
		case WRITE_FILE:
					fp = fopen(fileName,"w");
					if(fp == NULL)
						perror("Cannot open file.\n");
					break;
		default:
					fprintf(stdout,"Invalid mode passed\n Pass READ_FILE(0) or WRITE_FILE(1)\n");
					break;
	}
	strcpy(this->fileName,fileName);
}


FileUtil::~FileUtil(){
	fclose(fp);
	fp = NULL;
}


int FileUtil:: readLine(char buffer[]){
	if(fgets(buffer,sizeof(buffer),fp) != NULL)
		return 1;
	return 0;
}


void FileUtil::writeLine(char buffer[], ...){
	va_list ap;
	va_start(ap,buffer);
	vfprintf(fp,buffer,ap);
	va_end(ap);
	fflush(fp);
}


void FileUtil::getValueByKey(const char* key,char* value){
	FILE *fp;
	char buffer[4096];
	fp = fopen(fileName,"r+");
	if(fp == NULL)
		fprintf(stdout,"Cannot open file\n");
	char *pos;
	int offset = 0;

	while(fgets(buffer,sizeof(buffer),fp) != NULL){
		if((pos = strchr(buffer,'#')) != NULL)
			continue;
		pos = strstr(buffer,key);
		if(pos != NULL){
			pos = strchr(pos+1,' ');
			offset = strlen(buffer) - (pos - buffer);
			strncpy(value,pos+1,offset-1);
			return;
		}
	}
	strcpy(value,"");
}



