/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2013..2015 Uwe Freese
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

#include "rfm12.h"
#include "../src_common/uart.h"

#include "../src_common/msggrp_generic.h"
#include "../src_common/msggrp_weather.h"
#include "../src_common/msggrp_gpio.h"

#include "../src_common/e2p_hardware.h"
#include "../src_common/e2p_generic.h"
#include "../src_common/e2p_basestation.h"

#include "../src_common/aes256.h"
#include "../src_common/util.h"
#include "request_buffer.h"
#include "version.h"

#define LED_PIN 7
#define LED_PORT PORTD
#define LED_DDR DDRD

#define LOOP_CNT_QUEUE 50 // cycle in which the request queue is checked for a request (don't change! it's 1s)

#define UBRR_VAL_19200 64
#define UBRR_VAL_115200 10

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

	UART_PUTS("PKT:");
	
	// Data secured by the CRC starts with "SID"
	uartbuf[0] = 0;
	UART_PUTF_B("SID=%u;", senderid);
	UART_PUTF_B("PC=%lu;", packetcounter);
	UART_PUTF_B("MT=%u;", messagetype);

	// show ReceiverID for all requests
	if ((messagetype == MESSAGETYPE_GET) || (messagetype == MESSAGETYPE_SET) || (messagetype == MESSAGETYPE_SETGET))
	{
		uint16_t receiverid = pkg_headerext_common_get_receiverid();
		UART_PUTF_B("RID=%u;", receiverid);
	}
	
	uint16_t acksenderid = 65000;
	uint32_t ackpacketcounter = 0;

	// show AckSenderID, AckPacketCounter and Error for "Ack" and "AckStatus"
	if ((messagetype == MESSAGETYPE_ACK) || (messagetype == MESSAGETYPE_ACKSTATUS))
	{
		acksenderid = pkg_headerext_common_get_acksenderid();
		ackpacketcounter = pkg_headerext_common_get_ackpacketcounter();
		uint8_t error = pkg_headerext_common_get_error();
		UART_PUTF_B("ASID=%u;", acksenderid);
		UART_PUTF_B("APC=%lu;", ackpacketcounter);
		UART_PUTF_B("E=%u;", error);
	}

	// show MessageGroupID and MessageID for all MessageTypes except "Ack"
	if (messagetype != MESSAGETYPE_ACK)
	{
		messagegroupid = pkg_headerext_common_get_messagegroupid();
		messageid = pkg_headerext_common_get_messageid();
		UART_PUTF_B("MGID=%u;", messagegroupid);
		UART_PUTF_B("MID=%u;", messageid);
	}
	
	// show raw message data for all MessageTypes with data (= all except "Get" and "Ack")
	if ((messagetype != MESSAGETYPE_GET) && (messagetype != MESSAGETYPE_ACK))
	{
		uint8_t i;
		uint16_t count = (((uint16_t)len * 8) - __HEADEROFFSETBITS + 7) / 8;
	
		//UART_PUTF4("\r\n\r\nLEN=%u, START=%u, SHIFT=%u, COUNT=%u\r\n\r\n", len, start, shift, count);
	
		// print MessageData, but truncate trailing 0-bytes
		UART_PUTS_B("MD=");
		
		while (count > 1)
		{
			if (array_read_UIntValue8(__HEADEROFFSETBITS + (count - 1) * 8, 8, 0, 255, bufx) == 0)
			{
				count--;
			}
			else
			{
				break;
			}
		}

		for (i = 0; i < count; i++)
		{
			UART_PUTF_B("%02x", array_read_UIntValue8(__HEADEROFFSETBITS + i * 8, 8, 0, 255, bufx));
		}
		
		UART_PUTS_B(";");

		// calculate CRC of string in buffer and print it at the end
		uint32_t crc = crc32((uint8_t *)uartbuf, strlen(uartbuf));
		UART_SEND_BUF;
		UART_PUTF("%08lx\r\n", crc); // print CRC32

		// additionally decode the message data for a small number of messages
		switch (messagegroupid)
		{
			case MESSAGEGROUP_GENERIC:

				switch (messageid)
				{
					case MESSAGEID_GENERIC_DEVICEINFO:
						UART_PUTF("Detected Generic_DeviceInfo_Status: DeviceType=%u;", msg_generic_deviceinfo_get_devicetype());
						UART_PUTF("VersionMajor=%u;", msg_generic_deviceinfo_get_versionmajor());
						UART_PUTF("VersionMinor=%u;", msg_generic_deviceinfo_get_versionminor());
						UART_PUTF("VersionPatch=%u;", msg_generic_deviceinfo_get_versionpatch());
						UART_PUTF("VersionHash=%08lx;\r\n", msg_generic_deviceinfo_get_versionhash());
						break;
						
					case MESSAGEID_GENERIC_BATTERYSTATUS:
						UART_PUTF("Detected Generic_BatteryStatus_Status: Percentage=%u;\r\n", msg_generic_batterystatus_get_percentage());
						break;
						
					/*DateTime Status:
					UART_PUTS("Command Name=DateTime Status;");
					UART_PUTF3("Date=%u-%02u-%02u;", bufx[6] + 2000, bufx[7], bufx[8]);
					UART_PUTF3("Time=%02u:%02u:%02u", bufx[9], bufx[10], bufx[11]);*/
				
					default:
						break;
				}
				
				break;

			case MESSAGEGROUP_WEATHER:
				
				switch (messageid)
				{
					case MESSAGEID_WEATHER_TEMPERATURE:
						UART_PUTS("Detected Weather_Temperature_Status: Temperature=");
						print_signed(msg_weather_temperature_get_temperature());
						UART_PUTS(";\r\n");
						break;
					case MESSAGEID_WEATHER_HUMIDITYTEMPERATURE:
						u16 = msg_weather_humiditytemperature_get_humidity();
						UART_PUTF2("Detected Weather_HumidityTemperature_Status: Humidity=%u.%u;Temperature=", u16 / 10, u16 % 10);
						print_signed(msg_weather_humiditytemperature_get_temperature());
						UART_PUTS(";\r\n");
						break;
					default:
						break;
				}
				
				break;

			case MESSAGEGROUP_GPIO:
				
				switch (messageid)
				{
					case MESSAGEID_GPIO_DIGITALPORT:
						UART_PUTS("Detected GPIO_DigitalPort_Status: ");
						for (u16 = 0; u16 < 8; u16++)
						{
							UART_PUTF2("On[%u]=%u;", u16, msg_gpio_digitalport_get_on(u16));
						}
						UART_PUTS("\r\n");
						break;
					case MESSAGEID_GPIO_DIGITALPORTTIMEOUT:
						UART_PUTS("Detected GPIO_DigitalPortTimeout_Status: ");
						for (u16 = 0; u16 < 8; u16++)
						{
							UART_PUTF2("On[%u]=%u;", u16, msg_gpio_digitalporttimeout_get_on(u16));
							UART_PUTF2("TimeoutSec[%u]=%u;", u16, msg_gpio_digitalporttimeout_get_timeoutsec(u16));
						}
						UART_PUTS("\r\n");
						break;
					default:
						break;
				}
				
				break;
			
			default:
				break;
		}
	}
	
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

	// load AES key (0 is first AES key)
	if (aes_key_nr >= aes_key_count)
	{
		aes_key_nr = aes_key_count - 1;
	}
	
	e2p_basestation_get_aeskey(aes_key_nr, aes_key);

	// show info
	decode_data(packet_len);
	
	// encrypt and send
	__PACKETSIZEBYTES = packet_len;
	rfm12_send_bufx();
}

int main(void)
{
	uint8_t aes_key_nr;
	uint8_t loop = 0;
	bool uart_high_speed;
	
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

	// configure UART
	uart_high_speed = e2p_basestation_get_uartbaudrate() == UARTBAUDRATE_115200;
	uart_init_ubbr(uart_high_speed ? UBRR_VAL_115200 : UBRR_VAL_19200);
	
	UART_PUTS("\r\n");
	UART_PUTF4("smarthomatic Base Station v%u.%u.%u (%08lx)\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_HASH);
	UART_PUTS("(c) 2012..2015 Uwe Freese, www.smarthomatic.org\r\n");
	UART_PUTF("Device ID: %u\r\n", device_id);
	UART_PUTF("Packet counter: %lu\r\n", packetcounter);
	UART_PUTF("AES key count: %u\r\n", aes_key_count);
	UART_PUTF("UART baud rate: %lu\r\n", uart_high_speed ? 115200 : 19200);
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
					memcpy(bufx, rfm12_rx_buffer(), len);

					/*if (aes_key_nr == 0)
					{
						UART_PUTS("Before decryption: ");
						print_bytearray(bufx, len);
					}*/
				
					e2p_basestation_get_aeskey(aes_key_nr, aes_key);
					//UART_PUTS("Trying AES key 2 ");
					//print_bytearray((uint8_t *)aes_key, 32);

					aes256_decrypt_cbc(bufx, len);

					//UART_PUTS("Decrypted bytes: ");
					//print_bytearray(bufx, len);
					
					crcok = pkg_header_check_crc32(len);
					
					if (crcok)
					{
						// print relevant PKT info immediately for quickest reaction on PC
						decode_data(len);
						
						//UART_PUTS("CRC correct, AES key found!\r\n");
						UART_PUTF("Received (AES key %u): ", aes_key_nr);
						print_bytearray(bufx, len);
						
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
			
			// set header extension fields in bufx to the values given as hex string in the user input
			uint16_t receiverid = 0;
			switch (message_type)
			{
				case MESSAGETYPE_GET:
				case MESSAGETYPE_SET:
				case MESSAGETYPE_SETGET:
					receiverid = hex_to_uint16((uint8_t *)cmdbuf, 5);
					pkg_headerext_common_set_receiverid(receiverid);
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
					string_offset_data = 22;
					break;
			}

			uint8_t messagedata_len_raw = 0;

			// copy message data, which exists in all packets except in Get and Ack packets
			if ((message_type != MESSAGETYPE_GET) && (message_type != MESSAGETYPE_ACK))
			{
				messagedata_len_raw = (strlen(cmdbuf) - 1 - string_offset_data) / 2;
				uint8_t messagedata_len_trunc = 0;
				//UART_PUTF("User entered %u bytes MessageData.\r\n", messagedata_len_raw);

				// copy message data, using __HEADEROFFSETBITS value and string_offset_data
				for (i = 0; i < messagedata_len_raw; i++)
				{
					uint8_t val = hex_to_uint8((uint8_t *)cmdbuf, string_offset_data + 2 * i + 1);
					array_write_UIntValue(__HEADEROFFSETBITS + i * 8, 8, val, bufx);
					
					if (val)
					{
						messagedata_len_trunc = i + 1;
					}
				}
				
				// truncate message data after last byte which is not 0
				if (messagedata_len_trunc < messagedata_len_raw)
				{
					UART_PUTF2("Truncate MessageData from %u to %u bytes.\r\n", messagedata_len_raw, messagedata_len_trunc);
					messagedata_len_raw = messagedata_len_trunc;
				}
			}
			
			// round packet length to x * 16 bytes
			// __HEADEROFFSETBITS == Header + Ext.Header length
			// Message Data bytes = messagedata_len_raw * 8
			//UART_PUTF("__HEADEROFFSETBITS = %d\r\n", __HEADEROFFSETBITS);
			uint8_t packet_len = ((uint16_t)__HEADEROFFSETBITS + (uint16_t)messagedata_len_raw * 8 + 7) / 8;
			packet_len = ((packet_len - 1) / 16 + 1) * 16;

			// send packet which doesn't require an acknowledge immediately
			if ((message_type != MESSAGETYPE_GET) && (message_type != MESSAGETYPE_SET) && (message_type != MESSAGETYPE_SETGET))
			{
				send_packet(aes_key_nr, packet_len);
				led_blink(200, 0, 1);
			}
			else if (receiverid == 4095)
			{
				UART_PUTS("Sending broadcast request without using queue.\r\n");
				send_packet(aes_key_nr, packet_len);
				led_blink(200, 0, 1);
			}
			else // enqueue request (don't send immediately)
			{ 
				// header size = 9 bytes!
				if (queue_request(pkg_headerext_common_get_receiverid(), message_type, aes_key_nr, bufx + 9, packet_len - 9))
				{
					UART_PUTF("Request added to queue (%u bytes packet).\r\n", packet_len);
					loop = LOOP_CNT_QUEUE; // set counter to make main loop send packet immediately
				}
				else
				{
					UART_PUTS("Warning! Request queue full. Packet will not be sent.\r\n");
				}

				//print_request_queue(); // only for debugging (takes additional time to print it out)
			}
		
			// clear cmdbuf to receive more input from UART
			send_data_avail = false;
		}

		// flash LED every second to show the device is alive
		if (loop == LOOP_CNT_QUEUE)
		{
			loop = 0;
			
			request_t* request = find_request_to_repeat(packetcounter + 1);

			if (request != 0) // if request to repeat was found in queue
			{
				UART_PUTS("Repeating request.\r\n");
				send_packet((*request).aes_key, (*request).data_bytes + 9); // header size = 9 bytes!
				led_blink(200, 0, 1);
				
				print_request_queue();
			}
			else
			{
				led_blink(10, 10, 1);
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
