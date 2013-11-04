/*
 * Author: Rajan Jana
 * email: rjana@usc.edu
 *
 * Course: CSCI 551
 * Project B
 *
 * API to store keys and encrypt and descrypt message
 *
 *
 * */

#ifndef KEY_HANDLER_H_
#define KEY_HANDLER_H_

#include "encrypt.h"

#define KEY_LENGTH 16

class KeyHandler{

private:
	AES_KEY eKey;
	AES_KEY dKey;

public:
	void setRouterKeys(unsigned char routerID);
	void extractKey(unsigned char* text);
	void encryptMessage(unsigned char* clearText, int clearTextLen, unsigned char** eText, int *eTextLen);
	void decryptMessage(unsigned char* eText, int eTextLen, unsigned char** clearText, int *clearTextLen);

	unsigned char keyText[16];
};


#endif
