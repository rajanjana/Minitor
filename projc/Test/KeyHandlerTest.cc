/*
 * Author: Rajan Jana
 * email: rjana@usc.edu
 *
 * Course: CSCI 551
 * Project B
 *
 * */


#include "../encrypt/keyHandler.h"


int main(void){

	char* cleartext = "rajan";
	unsigned char* eText;
	int eLen;
	int len = 6;

	KeyHandler handler1;
	KeyHandler handler2;

	handler1.setRouterKeys(1);
	handler2.extractKey(handler1.keyText);

	handler1.encryptMessage((unsigned char*)cleartext,6,&eText,&eLen);
	printf("Encrypt length %d\n",eLen);
	handler2.encryptMessage((unsigned char*)cleartext,6,&eText,&eLen);
	printf("Encrypt length %d\n",eLen);

	handler1.decryptMessage(eText,eLen,(unsigned char**)&cleartext,&len);
	printf("Message is %s length %d\n",cleartext, len);
	handler2.decryptMessage(eText,eLen,(unsigned char**)&cleartext,&len);
	printf("Message is %s length %d\n",cleartext, len);
}
