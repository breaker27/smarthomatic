/*
* This file is part of smarthomatic, http://www.smarthomatic.com.
* Copyright (c) 2013 Uwe Freese
*
* Original authors:
*    Copyright (c) 2007-2009 Ilya O. Levin, http://www.literatecode.com
*    Other contributors: Hal Finney
*
* Development for smarthomatic by Uwe Freese started by adding a
* function to encrypt and decrypt a byte buffer directly and adding CBC mode.
*
* smarthomatic is free software: you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation, either version 3 of the License, or (at your
* option) any later version.
*
* smarthomatic is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
* Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with smarthomatic. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef AES256_H
#define AES256_H

#ifndef uint8_t
#define uint8_t unsigned char
#endif

typedef struct {
	uint8_t key[32]; 
	uint8_t enckey[32]; 
	uint8_t deckey[32];
} aes256_context; 

void aes256_init(aes256_context *, uint8_t * /* key */);
void aes256_done(aes256_context *);
void aes256_encrypt_ecb(aes256_context *, uint8_t * /* plaintext */);
void aes256_decrypt_ecb(aes256_context *, uint8_t * /* cipertext */);

extern aes256_context aes_ctx; // UF
extern uint8_t aes_key[32]; // UF
extern uint8_t aes_buf[16]; // UF

uint8_t aes256_encrypt_cbc(uint8_t *buffer, uint8_t len); // UF
void aes256_decrypt_cbc(uint8_t *buffer, uint8_t len); // UF

#endif
