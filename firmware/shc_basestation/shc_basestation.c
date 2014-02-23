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

#include "../src_common/e2p_hardware.h"
#include "../src_common/e2p_generic.h"
#include "../src_common/e2p_basestation.h"

#include "aes256.h"
#include "util.h"
#include "request_buffer.h"
#include "version.h"

#define LED_PIN 7
#define LED_PORT PORTD
#define LED_DDR DDRD

// check some assumptions at precompile time about flash layout
#if (EEPROM_AESKEYS_BIT != 0)
	#error AES keys do not start at a byte border. Not supported by base station (maybe fix E2P layout?).
#endif

uint16_t device_id;
uint8_t aes_key_count;

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
	
	uint16_t acksenderid = 65000;
	uint32_t ackpacketcounter = 0;

	// show AckSenderID, AckPacketCounter and Error for "Ack" and "AckStatus"
	if ((messagetype == MESSAGETYPE_ACK) || (messagetype == MESSAGETYPE_ACKSTATUS))
	{
		acksenderid = pkg_headerext_common_get_acksenderid();
		ackpacketcounter = pkg_headerext_common_get_ackpacketcounter();
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
	
	// show raw message data for all MessageTypes with data (= all except "Get" and "Ack")
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
					case MESSAGEID_GENERIC_VERSION:
						UART_PUTF("Major=%u;", msg_generic_version_get_major());
						UART_PUTF("Minor=%u;", msg_generic_version_get_minor());
						UART_PUTF("Patch=%u;", msg_generic_version_get_patch());
						UART_PUTF("Hash=%08lx;", msg_generic_version_get_hash());
						break;
						
					case MESSAGEID_GENERIC_BATTERYSTATUS:
						UART_PUTF("Percentage=%u;", msg_generic_batterystatus_get_percentage());
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
						print_signed(msg_tempsensor_temphumbristatus_get_temperature());
						u16 = msg_tempsensor_temphumbristatus_get_humidity();
						UART_PUTF2(";Humidity=%u.%u;", u16 / 10, u16 % 10);
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
	
	// Detect and process Acknowledges to base station, whose requests have to be removed from the request queue
	if ((messagetype == MESSAGETYPE_ACK) || (messagetype == MESSAGETYPE_ACKSTATUS))
	{
		if (acksenderid == device_id) // request sent from base station
		{
			remove_request(senderid, device_id, ackpacketcounter);
		}
	}
}

// Set senderid, packetcounter and CRC into the partly filled packet, encrypt it using the given AES key number and send it.
void send_packet(uint8_t aes_key_nr, uint8_t packet_len)
{
	pkg_header_set_senderid(device_id);

	inc_packetcounter();
	pkg_header_set_packetcounter(packetcounter);

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

	// encrypt and send
	__PACKETSIZEBYTES = packet_len;
	rfm12_sendbuf();
}

int main(void)
{
	uint8_t aes_key_nr;
	uint8_t loop = 0;
	uint8_t loop2 = 0;
	
	// delay 1s to avoid further communication with uart or RFM12 when my programmer resets the MC after 500ms...
	_delay_ms(1000);

	util_init();

	check_eeprom_compatibility(DEVICETYPE_BASESTATION);

	request_queue_init();

	// read packetcounter, increase by cycle and write back
	packetcounter = e2p_generic_get_packetcounter() + PACKET_COUNTER_WRITE_CYCLE;
	e2p_generic_set_packetcounter(packetcounter);

	// read device specific config
	aes_key_count = e2p_basestation_get_aeskeycount();

	device_id = e2p_generic_get_deviceid();

	uart_init();
	UART_PUTS("\r\n");
	UART_PUTF4("smarthomatic Base Station v%u.%u.%u (%08lx)\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_HASH);
	UART_PUTS("(c) 2012..2014 Uwe Freese, www.smarthomatic.org\r\n");
	UART_PUTF("Device ID: %u\r\n", device_id);
	UART_PUTF("Packet counter: %lu\r\n", packetcounter);
	UART_PUTF("AES key count: %u\r\n", aes_key_count);
	UART_PUTS("Waiting for incoming data. Press h for help.\r\n\r\n");

	led_blink(500, 500, 3);

	rfm12_init();
	sei();
	
	// ENCODE TEST (Move to unit test some day...)
	/*
	uint8_t testlen = 32;
	uint8_t aes_key_num = 0;
	
	memset(&bufx[0], 0, sizeof(bufx));
	bufx[0] = 0xff;
	bufx[1] = 0xb0;
	bufx[2] = 0xa0;
	bufx[3] = 0x3f;
	bufx[4] = 0x01;
	bufx[5] = 0x70;
	bufx[6] = 0x00;
	bufx[7] = 0x0c;
	bufx[8] = 0xa8;
	bufx[9] = 0x00;
	bufx[10] = 0x20;
	bufx[20] = 0x20;

	eeprom_read_block (aes_key, (uint8_t *)(EEPROM_AESKEYS_BYTE + aes_key_num * 32), 32);
	UART_PUTS("Using AES key ");
	print_bytearray((uint8_t *)aes_key, 32);
	
	UART_PUTS("Before encryption: ");
	print_bytearray(bufx, testlen);
	
	uint8_t aes_byte_count = aes256_encrypt_cbc(bufx, testlen);
	
	UART_PUTF("byte count = %u\r\n", aes_byte_count);
	
	UART_PUTS("After encryption: ");
	print_bytearray(bufx, aes_byte_count);
	
	aes256_decrypt_cbc(bufx, aes_byte_count);
  
	UART_PUTS("After decryption: ");
	print_bytearray(bufx, testlen);
	
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
				print_bytearray(bufx, len);
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
					//print_bytearray(bufx, len);
					
					crcok = pkg_header_check_crc32(len);
					
					if (crcok)
					{
						//UART_PUTS("CRC correct, AES key found!\r\n");
						UART_PUTF("Received (AES key %u): ", aes_key_nr);
						print_bytearray(bufx, len);
						
						decode_data(len);
						
						break;
					}
				}
				
				if (!crcok)
				{
					UART_PUTS("Received garbage (CRC wrong after decryption): ");
					memcpy(bufx, rfm12_rx_buffer(), len);
					print_bytearray(bufx, len);
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
			
			// set AES key nr
			aes_key_nr = hex_to_uint8((uint8_t *)cmdbuf, 1);
			//UART_PUTF("AES KEY = %u\r\n", aes_key_nr);

			// init packet buffer
			memset(&bufx[0], 0, sizeof(bufx));

			// set message type
			uint8_t message_type = hex_to_uint8((uint8_t *)cmdbuf, 3);
			pkg_header_set_messagetype(message_type);
			pkg_header_adjust_offset();
			//UART_PUTF("MessageType = %u\r\n", message_type);

			uint8_t string_offset_data = 0;
			
			/*
			UART_PUTS("sKK00RRRRGGMM.............Get\r\n");
			UART_PUTS("sKK01RRRRGGMMDD...........Set\r\n");
			UART_PUTS("sKK02RRRRGGMMDD...........SetGet\r\n");
			UART_PUTS("sKK08GGMMDD...............Status\r\n");
			UART_PUTS("sKK09SSSSPPPPPPEE.........Ack\r\n");
			UART_PUTS("sKK0ASSSSPPPPPPEEGGMMDD...AckStatus\r\n");
			*/
			
			// set header extension fields to the values given as hex string in the user input
			switch (message_type)
			{
				case MESSAGETYPE_GET:
				case MESSAGETYPE_SET:
				case MESSAGETYPE_SETGET:
					pkg_headerext_common_set_receiverid(hex_to_uint16((uint8_t *)cmdbuf, 5));
					pkg_headerext_common_set_messagegroupid(hex_to_uint8((uint8_t *)cmdbuf, 9));
					pkg_headerext_common_set_messageid(hex_to_uint8((uint8_t *)cmdbuf, 11));
					string_offset_data = 12;
					break;
				case MESSAGETYPE_STATUS:
					pkg_headerext_common_set_messagegroupid(hex_to_uint8((uint8_t *)cmdbuf, 5));
					pkg_headerext_common_set_messageid(hex_to_uint8((uint8_t *)cmdbuf, 7));
					string_offset_data = 8;
					break;
				case MESSAGETYPE_ACK:
					pkg_headerext_common_set_acksenderid(hex_to_uint16((uint8_t *)cmdbuf, 5));
					pkg_headerext_common_set_ackpacketcounter(hex_to_uint24((uint8_t *)cmdbuf, 9));
					pkg_headerext_common_set_error(hex_to_uint8((uint8_t *)cmdbuf, 15));
					// fallthrough!
				case MESSAGETYPE_ACKSTATUS:
					pkg_headerext_common_set_messagegroupid(hex_to_uint8((uint8_t *)cmdbuf, 17));
					pkg_headerext_common_set_messageid(hex_to_uint8((uint8_t *)cmdbuf, 19));
					string_offset_data = 20;
					break;
			}

			uint8_t data_len_raw = 0;

			// copy message data, which exists in all packets except in Get and Ack packets
			if ((message_type != MESSAGETYPE_GET) && (message_type != MESSAGETYPE_ACK))
			{
				uint8_t data_len_raw = (strlen(cmdbuf) - 1 - string_offset_data) / 2;
				//UART_PUTF("Data bytes = %u\r\n", data_len_raw);
				
				uint8_t start = __HEADEROFFSETBITS / 8;
				uint8_t shift = __HEADEROFFSETBITS % 8;

				// copy message data, using __HEADEROFFSETBITS value and string_offset_data
				for (i = 0; i < data_len_raw; i++)
				{
					uint8_t val = hex_to_uint8((uint8_t *)cmdbuf, string_offset_data + 2 * i + 1);
					array_write_UIntValue(start + i, shift, 8, val, bufx);
				}
			}
			
			// round packet length to x * 16 bytes
			uint8_t packet_len = ((uint16_t)__HEADEROFFSETBITS + (uint16_t)data_len_raw * 8) / 8;
			packet_len = ((packet_len - 1) / 16 + 1) * 16;

			// send packet which doesn't require an acknowledge immediately
			if ((message_type != MESSAGETYPE_GET) && (message_type != MESSAGETYPE_SET) && (message_type != MESSAGETYPE_SETGET))
			{
				send_packet(aes_key_nr, packet_len);
			}
			else // enqueue request (don't send immediately)
			{
				// header size = 9 bytes!
				if (queue_request(pkg_headerext_common_get_receiverid(), message_type, aes_key_nr, bufx + 9, packet_len - 9))
				{
					UART_PUTF("Request added to queue (%u bytes packet).\r\n", packet_len);
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
			
			request_t* request = find_request_to_repeat(packetcounter + 1);

			if (request != 0) // if request to repeat was found in queue
			{
				UART_PUTS("Repeating request.\r\n");					
				send_packet((*request).aes_key, (*request).data_bytes + 9); // header size = 9 bytes!
				print_request_queue();
			}
			
			// Auto-send something for debugging purposes...
			if (loop2 == 50)
			{
				//strcpy(cmdbuf, "s000102828300");
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
