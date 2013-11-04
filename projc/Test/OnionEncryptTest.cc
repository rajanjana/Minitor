/*
 * Author: Rajan Jana
 * email: rjana@usc.edu
 *
 * Course: CSCI 551
 * Project B
 *
 * */


#include "../data/routerData.h"
#include "../encrypt/keyHandler.h"
#include "../utility/networkUtil.h"


int main(void){

	RouterInfo info1;
	RouterInfo info2;
	RouterInfo info3;
	char keyData[4096];
	int eLen;
	unsigned char* eText;
	int len;
	unsigned char* a;

	info1.keyHandler.setRouterKeys(1);
	info2.keyHandler.setRouterKeys(2);
	info3.keyHandler.setRouterKeys(3);

	NetworkUtil::printPacket((unsigned char*)info1.keyHandler.keyText,16);

	memcpy(keyData,info1.keyHandler.keyText,16);
	len = KEY_LENGTH;
	info2.keyHandler.encryptMessage((unsigned char*)keyData,len,&eText,&eLen);
	memcpy(keyData,eText,eLen);
	len = eLen;
	info3.keyHandler.encryptMessage((unsigned char*)keyData,len,&eText,&eLen);

	info3.keyHandler.decryptMessage((unsigned char*)eText,eLen,(unsigned char**)&a,&len);

	memcpy(keyData,a,len);
	eLen = len;

	info2.keyHandler.decryptMessage((unsigned char*)keyData,eLen,(unsigned char**)&a,&len);

	memcpy(keyData,a,len);
	eLen = len;

	//info1.keyHandler.decryptMessage((unsigned char*)keyData,eLen,(unsigned char**)&a,&len);

	NetworkUtil::printPacket((unsigned char*)a,len);

}
