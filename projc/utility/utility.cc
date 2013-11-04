/*
 * Author: Rajan Jana
 * email: rjana@usc.edu
 *
 * Course: CSCI 551
 * Project A
 *
 * */

#include "utility.h"


static char *enableFlags = NULL; // controls which DEBUG messages are printed 

void DebugInit(char *flagList)
{
    enableFlags = flagList;
}

// check if debugging is enabled
bool DebugIsEnabled(char flag)
{
    if (enableFlags != NULL)
       return (strchr(enableFlags, flag) != 0) 
		|| (strchr(enableFlags, '+') != 0);
    else
      return false;
}

//print the debug message if debug is enabled
void DEBUG(char flag, char *format, ...)
{
    if (DebugIsEnabled(flag)) {
		va_list ap;
		va_start(ap, format);
		vfprintf(stdout, format, ap);
		va_end(ap);
		fflush(stdout);
    }
}

