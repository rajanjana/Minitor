/*
 * Author: Rajan Jana
 * email: rjana@usc.edu
 *
 * Course: CSCI 551
 * Project B
 *
 * */

#ifndef ENCRYPT_H_
#define ENCRYPT_H_

#include <openssl/aes.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

extern const  int AES_KEY_LENGTH_IN_BITS;
extern const  int AES_KEY_LENGTH_IN_CHARS;

void class_AES_set_encrypt_key(unsigned char *key_text, AES_KEY *enc_key);
void class_AES_set_decrypt_key(unsigned char *key_text, AES_KEY *dec_key);

void class_AES_encrypt_with_padding(unsigned char *in, int len, unsigned char **out, int *out_len, AES_KEY *enc_key);
void class_AES_decrypt_with_padding(unsigned char *in, int len, unsigned char **out, int *out_len, AES_KEY *dec_key);

#endif
