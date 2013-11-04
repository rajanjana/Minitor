/*
 * Author: Rajan Jana
 * email: rjana@usc.edu
 *
 * Course: CSCI 551
 * Project A
 *
 * */

#ifndef FILE_UTIL_H
#define FILE_UTIL_H

#define READ_FILE 0
#define WRITE_FILE 1

#include <stdio.h>
#include<stdarg.h>
#include <string.h>
#include "../utility/utility.h"


class FileUtil {
	public:
		FileUtil(char*,int);
		~FileUtil();
		int readLine(char*);
		void writeLine(char*, ...);
		void getValueByKey(const char*,char*);

	private:
		FILE* fp;
		char fileName[100];
};

#endif
