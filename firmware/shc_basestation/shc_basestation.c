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
#include "uart.h"

#include "../src_common/msggrp_generic.h"
#include "../src_common/msggrp_tempsensor.h"
#include "../src_common/msggrp_powerswitch.h"

#include "aes256.h"
#include "util.h"
#include "request_buffer.h"

// How often should the packetcounter_base be increased and written to EEPROM?
// This should be 2^32 (which is the maximum transmitted packet counter) /
// 100.000 (which is the maximum amount of possible EEPROM write cycles) or more.
// Therefore 100 is a good value.
#define PACKET_COUNTER_WRITE_CYCLE 100

#define LED_PIN 7
#define LED_PORT PORTD
#define LED_DDR DDRD

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

// Show info about the received packets.
// This is only for debugging and only few messages are supported. The definition
// of all packets must be known at the PC program that's processing the data.
void decode_data(uint8_t len)
{
	uint32_t messagegroupid, messageid;
	uint16_t u16;
	
	pkg_header_adjust_offset();

	uint16_t senderid = pkg_header_get_senderid();
	uint32_t packetcounter = pkg_header_get_packetcounter();
	MessageTypeEnum messagetype = pkg_header_get_messagetype();

	UART_PUTF("Packet Data: SenderID=%u;", senderid);
	UART_PUTF("PacketCounter=%lu;", packetcounter);
	UART_PUTF("MessageType=%u;", messagetype);

	// show ReceiverID for all requests
	if ((messagetype == MESSAGETYPE_GET) || (messagetype == MESSAGETYPE_SET) || (messagetype == MESSAGETYPE_SETGET))
	{
		uint16_t receiverid = pkg_headerext_common_get_receiverid();
		UART_PUTF("ReceiverID=%u;", receiverid);
	}
	
	// show AckSenderID, AckPacketCounter and Error for "Ack" and "AckStatus"
	if ((messagetype == MESSAGETYPE_ACK) || (messagetype == MESSAGETYPE_ACKSTATUS))
	{
		uint16_t acksenderid = pkg_headerext_common_get_acksenderid();
		uint32_t ackpacketcounter = pkg_headerext_common_get_ackpacketcounter();
		uint8_t error = pkg_headerext_common_get_error();
		UART_PUTF("AckSenderID=%u;", acksenderid);
		UART_PUTF("AckPacketCounter=%lu;", ackpacketcounter);
		UART_PUTF("Error=%u;", error);
	}

	// show MessageGroupID and MessageID for all MessageTypes except "Ack"
	if (messagetype != MESSAGETYPE_ACK)
	{
		messagegroupid = pkg_headerext_common_get_messagegroupid();
		messageid = pkg_headerext_common_get_messageid();
		UART_PUTF("MessageGroupID=%u;", messagegroupid);
		UART_PUTF("MessageID=%u;", messageid);
	}
	
	// show raw message data for all MessageTypes except "Get" and "Ack"
	if ((messagetype != MESSAGETYPE_GET) && (messagetype != MESSAGETYPE_ACK))
	{
		uint8_t i;
		uint8_t start = __HEADEROFFSETBITS / 8;
		uint8_t shift = __HEADEROFFSETBITS % 8;
		uint16_t count = (((uint16_t)len * 8) - __HEADEROFFSETBITS + 7) / 8;
	
		//UART_PUTF4("\r\n\r\nLEN=%u, START=%u, SHIFT=%u, COUNT=%u\r\n\r\n", len, start, shift, count);
	
		UART_PUTS("MessageData=");
	
		for (i = start; i < start + count; i++)
		{
			UART_PUTF("%02x", array_read_UIntValue8(i, shift, 8, 0, 255, bufx));
		}
		
		UART_PUTS(";");

		// additionally decode the message data for a small number of messages
		switch (messagegroupid)
		{
			case MESSAGEGROUP_GENERIC:

				switch (messageid)
				{
					case MESSAGEID_GENERIC_BATTERYSTATUS:
						UART_PUTF("Percentage=%u;", msg_generic_batterystatus_get_percentage);
						break;
						
					/*DateTime Status:
					UART_PUTS("Command Name=DateTime Status;");
					UART_PUTF3("Date=%u-%02u-%02u;", bufx[6] + 2000, bufx[7], bufx[8]);
					UART_PUTF3("Time=%02u:%02u:%02u", bufx[9], bufx[10], bufx[11]);*/
				
					default:
						break;
				}
				
				break;

			case MESSAGEGROUP_TEMPSENSOR:
				
				switch (messageid)
				{
					case MESSAGEID_TEMPSENSOR_TEMPHUMBRISTATUS:
						UART_PUTS("Temperature=");
						printSigned(msg_tempsensor_temphumbristatus_get_temperature());
						u16 = msg_tempsensor_temphumbristatus_get_humidity();
						UART_PUTF2(";Humidity=%u.%02u;", u16 / 100, u16 % 100);
						UART_PUTF("Brightness=%u;", msg_tempsensor_temphumbristatus_get_brightness());
						break;
					default:
						break;
				}
				
				break;

			case MESSAGEGROUP_POWERSWITCH:
				
				switch (messageid)
				{
					case MESSAGEID_POWERSWITCH_SWITCHSTATE:
						UART_PUTF("On=%u;", msg_powerswitch_switchstate_get_on());
						UART_PUTF("TimeoutSec=%u;", msg_powerswitch_switchstate_get_timeoutsec());
						break;
					default:
						break;
				}
				
				break;
			
			default:
				break;
		}
	}

	UART_PUTS("\r\n");
}

void send_packet(uint8_t aes_key_nr, uint8_t message_type, uint8_t packet_len)
{
	// init packet buffer
	//memset(&bufx[0], 0, sizeof(bufx));
	pkg_header_set_senderid(deviceID);

	// update packet counter
	packetcounter++;
	
	if (packetcounter % PACKET_COUNTER_WRITE_CYCLE == 0)
	{
		eeprom_write_UIntValue(EEPROM_PACKETCOUNTER_BYTE, EEPROM_PACKETCOUNTER_BIT, EEPROM_PACKETCOUNTER_LENGTH_BITS, packetcounter);
	}

	pkg_header_set_packetcounter(packetcounter);

	// set message type
	pkg_header_set_messagetype(message_type);

	// set CRC32
	pkg_header_set_crc32(crc32(bufx + 4, packet_len - 4));
	
	// load AES key (0 is first AES key)
	if (aes_key_nr >= aes_key_count)
	{
		aes_key_nr = aes_key_count - 1;
	}
	
	eeprom_read_block (aes_key, (uint8_t *)(EEPROM_AESKEYS_BYTE + aes_key_nr * 32), 32);
	
	// show info
	decode_data(packet_len);
	UART_PUTF("       AES key: %u\r\n", aes_key_nr);
	UART_PUTS("   Unencrypted: ");
	printbytearray(bufx, packet_len);

	// encrypt and send
	uint8_t aes_byte_count = rfm12_sendbuf(packet_len);
	
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

	uart_init();
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
				bool crcok = false;

				for (aes_key_nr = 0; aes_key_nr < aes_key_count ; aes_key_nr++)
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
					
					crcok = pkg_header_check_crc32(len);
					
					if (crcok)
					{
						//UART_PUTS("CRC correct, AES key found!\r\n");
						UART_PUTF("Received (AES key %u): ", aes_key_nr);
						printbytearray(bufx, len);
						
						decode_data(len);
						
						break;
					}
				}
				
				if (!crcok)
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
			
			// round data length to 7 + x * 16 bytes (including padding bytes)
			uint8_t data_len = ((data_len_raw + 9 - 1) / 16 + 1) * 16 - 9;
			
			//UART_PUTF("Data len raw = %u\r\n", data_len_raw);
			//UART_PUTF("Data len = %u\r\n", data_len);
			
			// set AES key nr
			aes_key_nr = hex_to_uint8((uint8_t *)sendbuf, 0);
			
			//UART_PUTF("AES KEY = %u\r\n", aes_key_nr);

			// init packet buffer
			memset(&bufx[0], 0, sizeof(bufx));
			pkg_header_set_senderid(deviceID);

			// set message type
			uint8_t message_type = hex_to_uint8((uint8_t *)sendbuf, 2);
			pkg_header_set_messagetype(message_type);
			
			//UART_PUTF("MessageType = %u\r\n", message_type);

			// set data
			for (i = 0; i < data_len_raw; i++)
			{
				data[i] = hex_to_uint8((uint8_t *)sendbuf, 4 + 2 * i);
				
				//UART_PUTF2("Data byte %u = %u\r\n", i, data[i]);
			}
			
			// set padding bytes
			for (i = data_len_raw; i < data_len; i++)
			{
				data[i] = 0;
			}

			// send packets other than known request types immediately
			if (true) // ((messagetype != MESSAGETYPE_GET) && (messagetype != MESSAGETYPE_SET) && (messagetype != MESSAGETYPE_SETGET))
			{
				// set data
				memcpy(bufx + 9, data, data_len); // header size = 9 bytes
				
				send_packet(aes_key_nr, message_type, data_len + 9);
			}
			else // enqueue request (don't send immediately)
			{
				/*if (queue_request(data[0], command_id, aes_key_nr, data + 1))
				{
					UART_PUTS("Adding request to queue.\r\n");
				}
				else
				{
					UART_PUTS("Warning! Request queue full. Packet will not be sent.\r\n");
				}

				print_request_queue();
				*/
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
				send_packet(0, 1, 16);
				print_request_queue();
			}
			
			// Auto-send something for debugging purposes...
			if (loop2 == 50)
			{
				strcpy(sendbuf, "000102828300");
				send_data_avail = true;
				
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
