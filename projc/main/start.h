/*
 * Author: Rajan Jana
 * email: rjana@usc.edu
 *
 * Course: CSCI 551
 * Project A
 *
 * */

#ifndef START_H
#define START_H


#include <stdlib.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/ip.h>
#include <linux/icmp.h>

#include "../utility/socket.h"
#include "../utility/fileUtil.h"
#include "../utility/utility.h"
#include "../utility/networkUtil.h"
#include "../data/routerData.h"
#include "../control/controlMessage.h"
#include "../data/flowCache.h"



extern int proxyPort;
extern FileUtil *proxyLogger;
extern RouterData *routerData;
extern int stage;
extern int numOfRouters;
extern int minHops;
extern int dieAfterPackets;

extern void logHex(unsigned char* message, int length, FileUtil* util);
extern unsigned short csum(unsigned short *buf, int nwords);
extern void logHexData(unsigned char* message, int length, FileUtil *util);
extern void proxySubroutine();
extern void getProxyUp();
extern int tun_alloc(char *dev, int flags);
extern unsigned short in_cksum(unsigned short *addr,int len);

extern char file[30];
extern void routerSubroutine(int,char*,char*);
extern void handleMessage(char*);

#endif
