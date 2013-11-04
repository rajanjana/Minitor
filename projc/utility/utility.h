/*
 * Author: Rajan Jana
 * email: rjana@usc.edu
 *
 * Course: CSCI 551
 * Project A
 *
 * */

#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#define ASSERT(condition)                                                     \
    if (!(condition)) {                                                       \
        fprintf(stderr, "Assertion failed: line %d, file \"%s\"\n",           \
                __LINE__, __FILE__);                                          \
	fflush(stderr);							      \
        abort();                                                              \
   }
   
extern void DebugInit(char* flags);	// enable printing debug messages
extern bool DebugIsEnabled(char flag); 	// Is this debug flag enabled?
extern void DEBUG (char flag, char* format, ...);  	// Print debug message


#endif
