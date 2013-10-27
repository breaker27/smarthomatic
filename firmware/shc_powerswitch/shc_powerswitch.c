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
#include "aes256.h"
#include "util.h"

#define UART_DEBUG   // Debug output over UART
// #define UART_RX      // Also allow receiving characters over UART (RX). Usually only necessary for the base station.

// How often should the packetcounter_base be increased and written to EEPROM?
// This should be 2^32 (which is the maximum transmitted packet counter) /
// 100.000 (which is the maximum amount of possible EEPROM write cycles) or more.
// Therefore 100 is a good value.
#define PACKET_COUNTER_WRITE_CYCLE 100

// Don't change this, because other switch count like 8 needs other status message.
// If support implemented, use EEPROM_SUPPORTEDSWITCHES_* E2P addresses.
#define SWITCH_COUNT 1

// TODO: Support more than one relais!
#define RELAIS_PORT PORTC
#define RELAIS_PIN_START 1

#define SEND_STATUS_EVERY_SEC 600 // how often should a status be sent?

uint8_t device_id;
uint32_t packetcounter;
uint8_t switch_state[SWITCH_COUNT];
uint16_t switch_timeout[SWITCH_COUNT];

uint16_t send_status_timeout = 5;

void printbytearray(uint8_t * b, uint8_t len)
{
#ifdef UART_DEBUG
	uint8_t i;
	
	for (i = 0; i < len; i++)
	{
		UART_PUTF("%02x ", b[i]);
	}
	
	UART_PUTS ("\r\n");
#endif
}

void rfm12_sendbuf(void)
{
	uint8_t aes_byte_count = aes256_encrypt_cbc(bufx, 16);
	rfm12_tx(aes_byte_count, 0, (uint8_t *) bufx);
}

void print_switch_state(void)
{
	uint8_t i;

	for (i = 1; i <= SWITCH_COUNT; i++)
	{
		UART_PUTF("Switch %u ", i);
		
		if (switch_state[i - 1])
		{
			UART_PUTS("ON");
		}
		else
		{
			UART_PUTS("OFF");
		}
		
		if (switch_timeout[i - 1])
		{		
			UART_PUTF(" (Timeout: %us)", switch_timeout[i - 1]);
		}

		UART_PUTS("\r\n");
	}
}

void send_power_switch_status(void)
{
	uint8_t i;
	
	UART_PUTS("Sending Power Switch Status:\r\n");

	print_switch_state();

	// set device ID (base station has ID 0 by definition)
	bufx[0] = device_id;
	
	// update packet counter
	packetcounter++;
	
	if (packetcounter % PACKET_COUNTER_WRITE_CYCLE == 0)
	{
		eeprom_write_dword((uint32_t*)0, packetcounter);
	}

	setBuf32(1, packetcounter);

	// set command ID "Power Switch Status"
	bufx[5] = 20;

	// set current switch state
	for (i = 0; i < SWITCH_COUNT; i++)
	{
		setBuf16(6 + 2 * i, (switch_timeout[i] << 1) | ((uint16_t)switch_state[i] & 0b1));
	}
	
	// set CRC32
	uint32_t crc = crc32(bufx, 12);
	setBuf32(12, crc);

	// show info
	UART_PUTF("CRC32: %lx\r\n", crc);
	uart_putstr("Unencrypted: ");
	printbytearray(bufx, 16);

	rfm12_sendbuf();
	
	UART_PUTS("Send encrypted: ");
	printbytearray(bufx, 16);
	UART_PUTS("\r\n");
}

void switchRelais(int8_t num, uint8_t on)
{
	if (on)
	{
		sbi(RELAIS_PORT, RELAIS_PIN_START + num);
		switch_led(1);
	}
	else
	{
		cbi(RELAIS_PORT, RELAIS_PIN_START + num);
		switch_led(0);
	}
}

int main ( void )
{
	uint32_t station_packetcounter;
	uint8_t loop = 0;
	uint8_t i;

	for (i = 0; i < SWITCH_COUNT; i++)
	{
		sbi(DDRC, RELAIS_PIN_START + i);
	}

	// delay 1s to avoid further communication with uart or RFM12 when my programmer resets the MC after 500ms...
	_delay_ms(1000);

	// read packetcounter, increase by cycle and write back
	packetcounter = eeprom_read_UIntValue32(EEPROM_PACKETCOUNTER_BYTE, EEPROM_PACKETCOUNTER_BIT,
		EEPROM_PACKETCOUNTER_LENGTH_BITS, EEPROM_PACKETCOUNTER_MINVAL, EEPROM_PACKETCOUNTER_MAXVAL) + PACKET_COUNTER_WRITE_CYCLE;

	eeprom_write_UIntValue(EEPROM_PACKETCOUNTER_BYTE, EEPROM_PACKETCOUNTER_BIT, EEPROM_PACKETCOUNTER_LENGTH_BITS, packetcounter);

	// read device specific config
	
	// read (saved) switch state from before the eventual powerloss
	for (i = 0; i < SWITCH_COUNT; i++)
	{
		uint16_t u16 = eeprom_read_UIntValue16(EEPROM_SWITCHSTATE_BYTE + i * 2, EEPROM_SWITCHSTATE_BIT,
			16, 0, 255);
		switch_state[i] = (uint8_t)(u16 & 0b1);
		switch_timeout[i] = u16 >> 1;
	}

	// read last received station packetcounter
	station_packetcounter = eeprom_read_UIntValue32(EEPROM_BASESTATIONPACKETCOUNTER_BYTE, EEPROM_BASESTATIONPACKETCOUNTER_BIT,
		EEPROM_BASESTATIONPACKETCOUNTER_LENGTH_BITS, EEPROM_BASESTATIONPACKETCOUNTER_MINVAL, EEPROM_BASESTATIONPACKETCOUNTER_MAXVAL);
	
	// read device id and write to send buffer
	device_id = eeprom_read_UIntValue16(EEPROM_DEVICEID_BYTE, EEPROM_DEVICEID_BIT,
		EEPROM_DEVICEID_LENGTH_BITS, EEPROM_DEVICEID_MINVAL, EEPROM_DEVICEID_MAXVAL);

#ifdef UART_DEBUG
	uart_init(false);
	UART_PUTS ("\r\n");
	UART_PUTS ("smarthomatic Power Switch V1.0 (c) 2013 Uwe Freese, www.smarthomatic.org\r\n");
	UART_PUTF ("Device ID: %u\r\n", device_id);
	UART_PUTF ("Packet counter: %lu\r\n", packetcounter);
	print_switch_state();
	UART_PUTF ("Last received station packet counter: %u\r\n\r\n", station_packetcounter);
#endif
	
	// init AES key
	eeprom_read_block(aes_key, (uint8_t *)EEPROM_AESKEY_BYTE, 32);

	rfm12_init();

	led_blink(200, 200, 5);

	// set initial switch state
	for (i = 0; i < SWITCH_COUNT; i++)
	{
		switchRelais(i, switch_state[i]);
	}

	sei();

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
				memcpy(bufx, rfm12_rx_buffer(), len);
				
				//UART_PUTS("Before decryption: ");
				//printbytearray(bufx, len);
					
				aes256_decrypt_cbc(bufx, len);

				//UART_PUTS("Decrypted bytes: ");
				//printbytearray(bufx, len);

				uint32_t assumed_crc = getBuf32(len - 4);
				uint32_t actual_crc = crc32(bufx, len - 4);
				
				//UART_PUTF("Received CRC32 would be %lx\r\n", assumed_crc);
				//UART_PUTF("Re-calculated CRC32 is  %lx\r\n", actual_crc);
				
				if (assumed_crc != actual_crc)
				{
					UART_PUTS("Received garbage (CRC wrong after decryption).\r\n");
				}
				else
				{
					//UART_PUTS("CRC correct, AES key found!\r\n");
					UART_PUTS("         Received: ");
					printbytearray(bufx, len - 4);
					
					// decode command and react
					uint8_t sender = bufx[0];
					
					UART_PUTF("           Sender: %u\r\n", sender);
					
					if (sender != 0)
					{
						UART_PUTF("Packet not from base station. Ignoring (Sender ID was: %u).\r\n", sender);
					}
					else
					{
						uint32_t packcnt = getBuf32(1);

						UART_PUTF("   Packet Counter: %lu\r\n", packcnt);

						// check received counter
						if (0) //packcnt <= station_packetcounter)
						{
							UART_PUTF2("Received packet counter %lu is lower than last received counter %lu. Ignoring packet.\r\n", packcnt, station_packetcounter);
						}
						else
						{
							// write received counter
							station_packetcounter = packcnt;
							
							eeprom_write_UIntValue(EEPROM_BASESTATIONPACKETCOUNTER_BYTE, EEPROM_BASESTATIONPACKETCOUNTER_BIT,
								EEPROM_BASESTATIONPACKETCOUNTER_LENGTH_BITS, station_packetcounter);
							
							// check command ID
							uint8_t cmd = bufx[5];

							UART_PUTF("       Command ID: %u\r\n", cmd);
							
							if (cmd != 140) // ID 140 == Power Switch Request
							{
								UART_PUTF("Received unknown command ID %u. Ignoring packet.\r\n", cmd);
							}
							else
							{
								// check device id
								uint8_t rcv_id = bufx[6];

								UART_PUTF("      Receiver ID: %u\r\n", rcv_id);
								
								if (rcv_id != device_id)
								{
									UART_PUTF("Device ID %u does not match. Ignoring packet.\r\n", rcv_id);
								}
								else
								{
									// read new switch state
									uint8_t switch_bitmask = bufx[7];
									
									uint16_t u16 = getBuf16(8);
									uint8_t req_state = u16 & 0b1;
									uint16_t req_timeout = u16 >> 1;

									UART_PUTF("   Switch Bitmask: %u\r\n", switch_bitmask); // TODO: Set binary mode like 00010110
									UART_PUTF("  Requested State: %u\r\n", req_state);
									UART_PUTF("          Timeout: %u\r\n", req_timeout);
									
									// react on changed state
									for (i = 0; i < SWITCH_COUNT; i++)
									{
										if (switch_bitmask & (1 << i)) // this switch is to be switched
										{
											UART_PUTF4("Switching relais %u from %u to %u with timeout %us.\r\n", i + 1, switch_state[i], req_state, req_timeout);

											// switch relais
											switchRelais(i, req_state);
											
											// write back switch state to EEPROM
											switch_state[i] = req_state;
											switch_timeout[i] = req_timeout;
											
											eeprom_write_UIntValue(EEPROM_SWITCHSTATE_BYTE + i * 2, EEPROM_BASESTATIONPACKETCOUNTER_BIT,
												EEPROM_BASESTATIONPACKETCOUNTER_LENGTH_BITS, u16);
										}
									}
									
									// send acknowledge
									UART_PUTS("Sending ACK:\r\n");

									// set device ID (base station has ID 0 by definition)
									bufx[0] = device_id;
									
									// update packet counter
									packetcounter++;
									
									if (packetcounter % PACKET_COUNTER_WRITE_CYCLE == 0)
									{
										eeprom_write_dword((uint32_t*)0, packetcounter);
									}

									setBuf32(1, packetcounter);

									// set command ID "Generic Acknowledge"
									bufx[5] = 1;
									
									// set sender ID of request
									bufx[6] = sender;
									
									// set Packet counter of request
									setBuf32(7, station_packetcounter);

									// zero unused bytes
									bufx[11] = 0;
									
									// set CRC32
									uint32_t crc = crc32(bufx, 12);
									setBuf32(12, crc);

									// show info
									UART_PUTF("            CRC32: %lx\r\n", crc);
									uart_putstr("      Unencrypted: ");
									printbytearray(bufx, 16);

									rfm12_sendbuf();
									
									UART_PUTS("   Send encrypted: ");
									printbytearray(bufx, 16);
									UART_PUTS("\r\n");
								
									rfm12_tick();
								
									led_blink(200, 0, 1);
									
									send_status_timeout = 5;
								}
							}
						}
					}
				}
				
			}

			// tell the implementation that the buffer can be reused for the next data.
			rfm12_rx_clear();
		}

		// flash LED every second to show the device is alive
		if (loop == 50)
		{
			sbi(PORTD, 3);
			_delay_ms(10);
			cbi(PORTD, 3);
			_delay_ms(10);
			
			loop = 0;

			// Check timeouts and toggle switches
			for (i = 0; i < SWITCH_COUNT; i++)
			{
				if (switch_timeout[i])
				{
					switch_timeout[i]--;
					
					if (switch_timeout[i] == 0)
					{
						switch_state[i] = switch_state[i] ? 0 : 1;
						
						UART_PUTF2("Timeout exceeded, switch relais %u to %u.\r\n", i + 1, switch_state[i]);
						
						// switch PIN for relais
						switchRelais(i, switch_state[i]);
						
						send_status_timeout = 1; // immediately send the status update
					}
				}
			}
			
			// send status from time to time
			send_status_timeout--;
		
			if (send_status_timeout == 0)
			{
				send_status_timeout = SEND_STATUS_EVERY_SEC;
				send_power_switch_status();
				
				rfm12_tick();

				sbi(PORTD, 3);
				_delay_ms(200);
				cbi(PORTD, 3);
			}			
		}
		else
		{
			_delay_ms(20);
		}

		rfm12_tick();

		loop++;
	}
	
	// never called
	// aes256_done(&aes_ctx);
}
