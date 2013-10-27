/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2013 Uwe Freese
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

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>

#include "rfm12.h"

#define UART_DEBUG   // Debug output over UART

#include "uart.h"
#include "aes256.h"

// How often should the packetcounter_base be increased and written to EEPROM?
// This should be 2^32 (which is the maximum transmitted packet counter) /
// 100.000 (which is the maximum amount of possible EEPROM write cycles) or more.
// Therefore 100 is a good value.
#define PACKET_COUNTER_WRITE_CYCLE 100

#define LED_PIN 7
#define LED_PORT PORTD
#define LED_DDR DDRD

#include "util.h"
#include "request_buffer.h"

// check some assumptions at precompile time about flash layout
#if (EEPROM_AESKEYS_BIT != 0)
	#error AES keys do not start at a byte border. Not supported by base station (maybe fix E2P layout?).
#endif

uint32_t packetcounter;
uint16_t deviceID;
uint8_t aes_key_count;

void printbytearray(uint8_t * b, uint8_t len)
{
	uint8_t i;
	
	for (i = 0; i < len; i++)
	{
		UART_PUTF("%02x ", b[i]);
	}
	
	UART_PUTS ("\r\n");
}

uint8_t rfm12_sendbuf(uint8_t len)
{
	uint8_t aes_byte_count = aes256_encrypt_cbc(bufx, len);
	rfm12_tx(aes_byte_count, 0, (uint8_t *) bufx);	
	return aes_byte_count;
}

void print_switch_state(uint8_t max)
{
	uint8_t i;
	uint16_t u16;

	for (i = 1; i <= max; i++)
	{
		u16 = getBuf16(4 + i * 2);
		
		UART_PUTF(";Switch %u=", i);
		
		if (u16 & 0b1)
		{
			UART_PUTS("ON");
		}
		else
		{
			UART_PUTS("OFF");
		}
		
		u16 = u16 >> 1;
		
		if (u16)
		{		
			UART_PUTF2(";Timeout %u=%us", i, u16);
		}
	}
}

// printf for floating point numbers takes ~1500 bytes program size.
// Therefore, we use a smaller special function instead
// as long it is used so rarely.
void printSigned(int16_t i)
{
	if (i < 0)
	{
		UART_PUTS("-");
		i = -i;
	}
	
	UART_PUTF2("%d.%02d;", i / 100, i % 100);
}

// Show info about the received packets.
// This is only for debugging. The definition of all packets must be known at the PC program
// that's processing the data.
void decode_data(uint8_t len)
{
	uint16_t u16;
	int16_t s16;
	uint32_t u32;	

	UART_PUTF("Sender ID=%u;", bufx[0]);
	UART_PUTF("Packet Counter=%lu;", getBuf32(1));
	uint8_t cmd = bufx[5];
	UART_PUTF("Command ID=%u;", cmd);

	switch (cmd)
	{
		case 1: // Generic Acknowledge
			u32 = getBuf32(7);
			UART_PUTS("Command Name=Generic Acknowledge;");
			UART_PUTF("Request Sender ID=%u;", bufx[6]);
			UART_PUTF("Request Packet Counter=%lu\r\n", u32);
			remove_request(bufx[0], bufx[6], u32);
			break;

		case 10: // Temperature Sensor Status
			UART_PUTS("Command Name=Temperature Sensor Status;");
			s16 = (int16_t)getBuf16(7);
			UART_PUTF("Battery=%u;", bufx[6]);
			UART_PUTS("Temperature=");
			printSigned(s16);
			u16 = getBuf16(9);
			UART_PUTF2("Humidity=%u.%02u;", u16 / 100, u16 % 100);
			UART_PUTF("Brightness=%u", bufx[11]);
			break;
			
		case 20: // Power Switch Status
			UART_PUTS("Command Name=Power Switch Status");
			print_switch_state(3);
			break;

		case 21: // Power Switch Status Extended
			UART_PUTS("Command Name=Power Switch Status Extended");
			print_switch_state(8);
			break;

		case 30: // DateTime Status
			UART_PUTS("Command Name=DateTime Status;");
			UART_PUTF3("Date=%u-%02u-%02u;", bufx[6] + 2000, bufx[7], bufx[8]);
			UART_PUTF3("Time=%02u:%02u:%02u", bufx[9], bufx[10], bufx[11]);
			break;
		
		case 140: // Power Switch Request
			UART_PUTS("Command Name=Power Switch Request;");
			uint16_t u16 = getBuf16(8);
			UART_PUTF("Receiver ID=%u;", bufx[6]);
			UART_PUTF("Switch Bitmask=%u;", bufx[7]); // TODO: Set binary mode like 00010110
			UART_PUTF("Requested State=%u;", u16 & 0b1);
			UART_PUTF("Timeout=%u", u16 >> 1);
			break;
			
		case 141: // Dimmer Request
			UART_PUTS("Command Name=Dimmer Request;");
			UART_PUTF("Receiver ID=%u;", bufx[6]);
			UART_PUTF("Animation Mode=%u;", bufx[7] >> 5);
			UART_PUTF("Dimmer Bitmask=%u;", bufx[7] & 0b111);
			UART_PUTF("Timeout=%u;", getBuf16(8));
			UART_PUTF("Start Brightness=%u;", bufx[10]);
			UART_PUTF("End Brightness=%u", bufx[11]);
			break;
			
		default:
			UART_PUTS("Command Name=Unknown;");
			UART_PUTS("Data=");
			printbytearray(bufx + 6, len - 6);
	}

	if (cmd != 1)
	{
		UART_PUTS("\r\n");
	}
}

void send_packet(uint8_t aes_key_nr, uint8_t data_len)
{
	// set device ID (base station has ID 0 by definition)
	bufx[0] = deviceID;

	// update packet counter
	packetcounter++;
	
	if (packetcounter % PACKET_COUNTER_WRITE_CYCLE == 0)
	{
		eeprom_write_dword((uint32_t*)0, packetcounter);
	}

	setBuf32(1, packetcounter);

	// set CRC32
	uint32_t crc = crc32(bufx, data_len + 6);
	setBuf32(data_len + 6, crc);

	// load AES key (0 is first AES key)
	if (aes_key_nr >= aes_key_count)
	{
		aes_key_nr = aes_key_count - 1;
	}
	
	eeprom_read_block (aes_key, (uint8_t *)(EEPROM_AESKEYS_BYTE + aes_key_nr * 32), 32);
	
	// show info
	decode_data(data_len + 6);
	UART_PUTF("       AES key: %u\r\n", aes_key_nr);
	
	UART_PUTF("         CRC32: %02lx\r\n", crc);
	UART_PUTS("   Unencrypted: ");
	printbytearray(bufx, data_len + 10);

	// encrypt and send
	uint8_t aes_byte_count = rfm12_sendbuf(data_len + 10);
	
	UART_PUTS("Send encrypted: ");
	printbytearray(bufx, aes_byte_count);

	UART_PUTS("\r\n");
}

int main ( void )
{
	uint8_t aes_key_nr;
	uint8_t loop = 0;
	uint8_t loop2 = 0;
	
	uint8_t data[22];

	sbi(LED_DDR, LED_PIN);

	// delay 1s to avoid further communication with uart or RFM12 when my programmer resets the MC after 500ms...
	_delay_ms(1000);

	check_eeprom_compatibility(DEVICETYPE_BASE_STATION);

	request_queue_init();

	// read packetcounter, increase by cycle and write back
	packetcounter = eeprom_read_UIntValue32(EEPROM_PACKETCOUNTER_BYTE, EEPROM_PACKETCOUNTER_BIT,
		EEPROM_PACKETCOUNTER_LENGTH_BITS, EEPROM_PACKETCOUNTER_MINVAL, EEPROM_PACKETCOUNTER_MAXVAL) + PACKET_COUNTER_WRITE_CYCLE;

	eeprom_write_UIntValue(EEPROM_PACKETCOUNTER_BYTE, EEPROM_PACKETCOUNTER_BIT, EEPROM_PACKETCOUNTER_LENGTH_BITS, packetcounter);

	// read device specific config
	aes_key_count = eeprom_read_UIntValue8(EEPROM_AESKEYCOUNT_BYTE, EEPROM_AESKEYCOUNT_BIT,
		EEPROM_AESKEYCOUNT_LENGTH_BITS, EEPROM_AESKEYCOUNT_MINVAL, EEPROM_AESKEYCOUNT_MAXVAL);

	deviceID = eeprom_read_UIntValue16(EEPROM_DEVICEID_BYTE, EEPROM_DEVICEID_BIT,
		EEPROM_DEVICEID_LENGTH_BITS, EEPROM_DEVICEID_MINVAL, EEPROM_DEVICEID_MAXVAL);

	uart_init(true);
	UART_PUTS ("\r\n");
	UART_PUTS ("smarthomatic Base Station V1.0 (c) 2012 Uwe Freese, www.smarthomatic.org\r\n");
	UART_PUTF ("Device ID: %u\r\n", deviceID);
	UART_PUTF ("Packet counter: %lu\r\n", packetcounter);
	UART_PUTF ("AES key count: %u\r\n", aes_key_count);
	UART_PUTS ("Waiting for incoming data. Press h for help.\r\n");

	rfm12_init();
	sei();
	
	// ENCODE TEST
	/*
	uint8_t testlen = 64;
	
	eeprom_read_block (aes_key, (uint8_t *)EEPROM_AESKEYS_BYTE, 32);
	UART_PUTS("Using AES key ");
	printbytearray((uint8_t *)aes_key, 32);
			
	UART_PUTS("Before encryption: ");
	printbytearray(bufx, testlen);
  
	unsigned long crc = crc32(bufx, testlen);
	UART_PUTF("CRC32 is %lx (added as last 4 bytes)\r\n", crc);
	
	UART_PUTS("1\r\n");
	crc = crc32(bufx, testlen - 4);
	UART_PUTS("2\r\n");
	setBuf32(testlen - 4, crc);
	
	UART_PUTS("Before encryption (CRC added): ");
	printbytearray(bufx, testlen);

	UART_PUTS("1\r\n");
	uint8_t aes_byte_count = aes256_encrypt_cbc(bufx, testlen);
	UART_PUTS("2\r\n");
  
	UART_PUTS("After encryption: ");
	printbytearray(bufx, aes_byte_count);
	
	UART_PUTF("String len = %u\r\n", aes_byte_count);
	
	UART_PUTS("1\r\n");
	aes256_decrypt_cbc(bufx, aes_byte_count);
	UART_PUTS("2\r\n");
  
	UART_PUTS("After decryption: ");
	printbytearray(bufx, testlen);
	
	crc = getBuf32(testlen - 4);
	UART_PUTF("CRC32 is %lx (last 4 bytes from decrypted message)\r\n", crc);
	printbytearray(bufx, testlen);
	
	UART_PUTS("After decryption (CRC removed): ");
	printbytearray(bufx, testlen);
	
	UART_PUTF("String len = %u\r\n", testlen);
  
	while(1);
	*/

	while (42)
	{
		if (rfm12_rx_status() == STATUS_COMPLETE)
		{
			uint8_t len = rfm12_rx_len();
			
			if ((len == 0) || (len % 16 != 0))
			{
				UART_PUTF("Received garbage (%u bytes not multiple of 16): ", len);
				printbytearray(bufx, len);
			}
			else // try to decrypt with all keys stored in EEPROM
			{
				uint32_t assumed_crc = 0;
				uint32_t actual_crc = 1;

				for(aes_key_nr = 0; aes_key_nr < aes_key_count ; aes_key_nr++)
				{
					//strncpy((char *)bufx, (char *)rfm12_rx_buffer(), len);
					memcpy(bufx, rfm12_rx_buffer(), len);

					/*if (aes_key_nr == 0)
					{
						UART_PUTS("Before decryption: ");
						printbytearray(bufx, len);
					}*/
				
					eeprom_read_block (aes_key, (uint8_t *)(EEPROM_AESKEYS_BYTE + aes_key_nr * 32), 32);
					//UART_PUTS("Trying AES key ");
					//printbytearray((uint8_t *)aes_key, 32);

					aes256_decrypt_cbc(bufx, len);

					//UART_PUTS("Decrypted bytes: ");
					//printbytearray(bufx, len);

					assumed_crc = getBuf32(len - 4);
					actual_crc = crc32(bufx, len - 4);
					
					//UART_PUTF("Received CRC32 would be %lx\r\n", assumed_crc);
					//UART_PUTF("Re-calculated CRC32 is  %lx\r\n", actual_crc);
					
					if (assumed_crc == actual_crc)
					{
						//UART_PUTS("CRC correct, AES key found!\r\n");
						UART_PUTF("Received (AES key %u): ", aes_key_nr);
						printbytearray(bufx, len - 4);
						
						decode_data(len - 4);
						
						break;
					}
				}
				
				if (assumed_crc != actual_crc)
				{
					UART_PUTS("Received garbage (CRC wrong after decryption).\r\n");
				}
				
				UART_PUTS("\r\n");
			}

			//uart_hexdump((char *)bufcontents, rfm12_rx_len());
			//UART_PUTS("\r\n");

			// tell the implementation that the buffer can be reused for the next data.
			rfm12_rx_clear();
		}

		// send data, if waiting in send buffer
		if (send_data_avail)
		{
			uint8_t i;
			
			uint8_t data_len_raw = strlen(sendbuf) / 2 - 2;
			
			// round data length to 6 + 16 bytes (including padding bytes)
			uint8_t data_len = (((data_len_raw + 9) / 16) + 1) * 16 - 10;
	
			// set aes key nr
			aes_key_nr = hex_to_uint8((uint8_t *)sendbuf, 0);
			
			//UART_PUTF("AES KEY = %u\r\n", aes_key_nr);

			// set command id
			uint8_t command_id = hex_to_uint8((uint8_t *)sendbuf, 2);

			// set data
			for (i = 0; i < data_len_raw; i++)
			{
				data[i] = hex_to_uint8((uint8_t *)sendbuf, 4 + 2 * i);
			}
			
			// set padding bytes
			for (i = data_len_raw; i < data_len; i++)
			{
				data[i] = 0;
			}

			// send status packet immediately (command IDs are less than 128)
			if (command_id < 128)
			{
				// set command id
				bufx[5] = command_id;
				
				// set data
				memcpy(bufx + 6, data, data_len);
				
				send_packet(aes_key_nr, data_len);
			}
			else // enqueue request (don't send immediately)
			{
				if (queue_request(data[0], command_id, aes_key_nr, data + 1))
				{
					UART_PUTS("Adding request to queue.\r\n");
				}
				else
				{
					UART_PUTS("Warning! Request queue full. Packet will not be sent.\r\n");
				}

				print_request_queue();
			}
		
			// clear send text buffer
			send_data_avail = false;

			rfm12_tick();

			led_blink(200, 0, 1);
		}

		// flash LED every second to show the device is alive
		if (loop == 50)
		{
			led_blink(10, 10, 1);
			
			loop = 0;

			if (set_repeat_request(packetcounter + 1)) // if request to repeat was found in queue
			{
				UART_PUTS("Repeating request.\r\n");					
				send_packet(0, 6);
				print_request_queue();
			}
			
			// Auto-send something for debugging purposes...
			if (loop2 == 50)
			{
				//strcpy(sendbuf, "008c0001003d");
				//send_data_avail = true;
				
				loop2 = 0;
			}
			else
			{
				loop2++;
			}
		}
		else
		{
			_delay_ms(20);
		}

		rfm12_tick();

		loop++;
		
		process_rxbuf();
		
		if (uart_timeout > 0)
		{
			uart_timeout--;
			
			if (uart_timeout == 0)
			{
				UART_PUTS("*** UART user timeout. Input was ignored. ***\r\n");
			}
		}
	}
	
	// never called
	// aes256_done(&aes_ctx);
}
