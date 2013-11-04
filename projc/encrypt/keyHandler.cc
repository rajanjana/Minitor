/*
 * Author: Rajan Jana
 * email: rjana@usc.edu
 *
 * Course: CSCI 551
 * Project B
 *
 *
 *
 *	API implementation for storing, accessing, encrypting and decrypting messages.
 *
 * */

#include "keyHandler.h"

void KeyHandler::setRouterKeys(unsigned char routerID){
	int key[4]; // 16 bytes random key;
	unsigned char* singleByte; // get single byte for xor

	key[0] = rand();
	key[1] = rand();
	key[2] = rand();
	key[3] = rand();

	singleByte = (unsigned char*)key;

	for(int i=0;i<16;i++){
		keyText[i] = singleByte[i] ^ routerID;
	}
	class_AES_set_encrypt_key(keyText,&eKey);
	class_AES_set_decrypt_key(keyText,&dKey);
}


void KeyHandler::extractKey(unsigned char* keyData){
	memcpy(keyText,keyData,16);

	class_AES_set_encrypt_key(keyText,&eKey);
	class_AES_set_decrypt_key(keyText,&dKey);

}


void KeyHandler::encryptMessage(unsigned char* clearText, int clearTextLen, unsigned char** eText, int *eTextLen){
	class_AES_encrypt_with_padding(clearText, clearTextLen,eText,eTextLen,&eKey);
}

void KeyHandler::decryptMessage(unsigned char* eText, int eTextLen, unsigned char** clearText, int *clearTextLen){
	class_AES_decrypt_with_padding(eText,eTextLen,clearText,clearTextLen,&dKey);
}




