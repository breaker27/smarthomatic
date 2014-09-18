/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
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

#include "aes256.h"

#include <string.h>

#include "aes.h"
#include "aes256_dec.h"
#include "aes256_enc.h"

aes256_ctx_t aes_ctx;
uint8_t aes_key[32];


// Encode the data at the given memory location using CBC (cypher block chaining).
// If len is not a multiple of 16, the remaining bytes are written to 0 before encoding.
// The then encoded number of bytes is therefore always a multiple of 16.
// As a result, (((len - 1) / 16) + 1) * 16 bytes will be used. The count will be returned as result.
// The cyphertext of the blocks will be used to XOR the plaintext of the further blocks beginning with the second before encoding.
// This is to force that also the further blocks will never result in the same encrypted pattern
// if they don't change (assuming that the first block changes every time because of the packet counter).
uint8_t aes256_encrypt_cbc(uint8_t *buffer, uint8_t len)
{
	uint8_t n = len % 16;
	if (n)
	{
		n = 16 - n;
		memset(buffer + len, 0, n);	// fill up with zeros
		len += n;
	}
	aes256_init(aes_key, &aes_ctx);
	
	uint8_t offset;
	for (offset = 0; offset < len; offset += 16)
	{
		uint8_t i;
		
		// XOR the later buffers with the cyphertext from the block before
		if (offset > 0)
		{
			for (i = 0; i < 16; i++)
			{
				buffer[offset + i] ^= buffer[(offset - 16) + i];
			}
		}
		aes256_enc(buffer + offset, &aes_ctx);
	}
	return offset;
}

// Decode the data at the given memory location. The length len has to be always
// divisable by 16, because AES256 always encodes 16-byte blocks.
// If the result is a string, use strlen to get its length, which
// is usually shorter than the input buffer.
void aes256_decrypt_cbc(uint8_t *buffer, uint8_t len)
{
	uint8_t n = len % 16;
	if (n)
	{
		// Should not happen, but double check here since mismatch could lead to a buffer over-/under-flow here!
		n = 16 - n;
		len += n;
	}
	aes256_init(aes_key, &aes_ctx);
	
	uint8_t offset = len;
	while (offset > 0)
	{
		offset -= 16;
		aes256_dec(buffer + offset, &aes_ctx);
		if (offset > 0)
		{
			uint8_t i;
			// XOR the later buffers with the cyphertext from the block before
			for (i = 0; i < 16; i++)
			{
				buffer[offset + i] ^= buffer[(offset - 16) + i];
			}
		}
	}
}
