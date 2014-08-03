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

#include "rfm12.h"
#include "uart.h"

#include "../src_common/msggrp_generic.h"
#include "../src_common/msggrp_powerswitch.h"

#include "../src_common/e2p_hardware.h"
#include "../src_common/e2p_generic.h"
#include "../src_common/e2p_powerswitch.h"

#include "aes256.h"
#include "util.h"
#include "version.h"

// Don't change this, because other switch count like 8 needs other status message.
// If support implemented, use EEPROM_SUPPORTEDSWITCHES_* E2P addresses.
#define SWITCH_COUNT 1

// TODO: Support more than one relais!
#define RELAIS_PORT PORTC
#define RELAIS_PIN_START 0

#define BUTTON_DDR DDRD
#define BUTTON_PORT PORTD
#define BUTTON_PINPORT PIND
#define BUTTON_PIN 3

#define SEND_STATUS_EVERY_SEC 1800 // how often should a status be sent?
#define SEND_VERSION_STATUS_CYCLE 50 // send version status x times less than switch status (~once per day)

uint16_t device_id;
uint32_t station_packetcounter;
bool switch_state[SWITCH_COUNT];
uint16_t switch_timeout[SWITCH_COUNT];

uint16_t send_status_timeout = 5;
uint8_t version_status_cycle = SEND_VERSION_STATUS_CYCLE - 1; // send promptly after startup

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
	UART_PUTS("Sending Power Switch Status:\r\n");

	print_switch_state();

	inc_packetcounter();

	// Set packet content
	pkg_header_init_powerswitch_switchstate_status();
	pkg_header_set_senderid(device_id);
	pkg_header_set_packetcounter(packetcounter);
	msg_powerswitch_switchstate_set_on(switch_state[0]); // TODO: Support > 1 switch
	msg_powerswitch_switchstate_set_timeoutsec(switch_timeout[0]); // TODO: Support > 1 switch
	pkg_header_calc_crc32();

	rfm12_send_bufx();
}

void send_version_status(void)
{
	inc_packetcounter();

	UART_PUTF4("Sending Version: v%u.%u.%u (%08lx)\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_HASH);
	
	// Set packet content
	pkg_header_init_generic_version_status();
	pkg_header_set_senderid(device_id);
	pkg_header_set_packetcounter(packetcounter);
	msg_generic_version_set_major(VERSION_MAJOR);
	msg_generic_version_set_minor(VERSION_MINOR);
	msg_generic_version_set_patch(VERSION_PATCH);
	msg_generic_version_set_hash(VERSION_HASH);
	pkg_header_calc_crc32();

	rfm12_send_bufx();
}

void switchRelais(int8_t num, bool on, uint16_t timeout)
{
	UART_PUTF3("Switching relais %u to %u with timeout %us.\r\n", num + 1, on, timeout);

	bool change_state = switch_state[num] != on;
	bool change_timeout = switch_timeout[num] != timeout;
			
	if (change_state)
	{
		switch_state[num] = on;
		e2p_powerswitch_set_switchstate(num, on);
		
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
	
	if (change_timeout)
	{
		switch_timeout[num] = timeout;
		e2p_powerswitch_set_switchtimeout(num, timeout);
	}
}

// React accordingly on the MessageType, MessageGroup and MessageID.
void process_message(MessageTypeEnum messagetype, uint32_t messagegroupid, uint32_t messageid)
{
	UART_PUTF("MessageGroupID:%u;", messagegroupid);
	
	if (messagegroupid != MESSAGEGROUP_POWERSWITCH)
	{
		UART_PUTS("\r\nERR: Unsupported MessageGroupID.\r\n");
		return;
	}
	
	UART_PUTF("MessageID:%u;", messageid);

	if (messageid != MESSAGEID_POWERSWITCH_SWITCHSTATE)
	{
		UART_PUTS("\r\nERR: Unsupported MessageID.\r\n");
		return;
	}

	// "Set" or "SetGet" -> modify switch state
	if ((messagetype == MESSAGETYPE_SET) || (messagetype == MESSAGETYPE_SETGET))
	{
		uint8_t i;
		bool req_on = msg_powerswitch_switchstate_get_on();
		uint16_t req_timeout = msg_powerswitch_switchstate_get_timeoutsec();

		UART_PUTF("On:%u;", req_on);
		UART_PUTF("TimeoutSec:%u;\r\n", req_timeout);

		// react on changed state (version for more than one switch...)
		for (i = 0; i < SWITCH_COUNT; i++)
		{
			switchRelais(i, req_on, req_timeout);
		}
	}

	// remember some values before the packet buffer is destroyed
	uint32_t acksenderid = pkg_header_get_senderid();
	uint32_t ackpacketcounter = pkg_header_get_packetcounter();

	inc_packetcounter();

	// "Set" -> send "Ack"
	if (messagetype == MESSAGETYPE_SET)
	{
		pkg_header_init_powerswitch_switchstate_ack();

		UART_PUTS("Sending Ack\r\n");
	}
	// "Get" or "SetGet" -> send "AckStatus"
	else
	{
		pkg_header_init_powerswitch_switchstate_ackstatus();
		
		// set message data
		msg_powerswitch_switchstate_set_on(switch_state[0] & 1); // TODO: Support > 1 switch
		msg_powerswitch_switchstate_set_timeoutsec(switch_timeout[0]); // TODO: Support > 1 switch

		UART_PUTS("Sending AckStatus\r\n");
	}

	// set common fields
	pkg_header_set_senderid(device_id);
	pkg_header_set_packetcounter(packetcounter);
	
	pkg_headerext_common_set_acksenderid(acksenderid);
	pkg_headerext_common_set_ackpacketcounter(ackpacketcounter);
	pkg_headerext_common_set_error(false); // FIXME: Move code for the Ack to a function and also return an Ack when errors occur before!
	
	pkg_header_calc_crc32();
	
	rfm12_send_bufx();
	send_status_timeout = 5;
}

void process_packet(uint8_t len)
{
	pkg_header_adjust_offset();

	// check SenderID
	uint32_t senderID = pkg_header_get_senderid();
	UART_PUTF("Packet Data: SenderID:%u;", senderID);
	
	if (senderID != 0)
	{
		UART_PUTS("\r\nERR: Illegal SenderID.\r\n");
		return;
	}

	// check PacketCounter
	// TODO: Reject if packet counter lower than remembered!!
	uint32_t packcnt = pkg_header_get_packetcounter();
	UART_PUTF("PacketCounter:%lu;", packcnt);

	if (0) // packcnt <= station_packetcounter ??
	{
		UART_PUTF("\r\nERR: Received PacketCounter < %lu.\r\n", station_packetcounter);
		return;
	}
	
	// write received counter
	station_packetcounter = packcnt;
	
	e2p_powerswitch_set_basestationpacketcounter(station_packetcounter);
	
	// check MessageType
	MessageTypeEnum messagetype = pkg_header_get_messagetype();
	UART_PUTF("MessageType:%u;", messagetype);
	
	if ((messagetype != MESSAGETYPE_GET) && (messagetype != MESSAGETYPE_SET) && (messagetype != MESSAGETYPE_SETGET))
	{
		UART_PUTS("\r\nERR: Unsupported MessageType.\r\n");
		return;
	}
	
	// check device id
	uint8_t rcv_id = pkg_headerext_common_get_receiverid();

	UART_PUTF("ReceiverID:%u;", rcv_id);
	
	if (rcv_id != device_id)
	{
		UART_PUTS("\r\nWRN: DeviceID does not match.\r\n");
		return;
	}
	
	// check MessageGroup + MessageID
	uint32_t messagegroupid = pkg_headerext_common_get_messagegroupid();
	uint32_t messageid = pkg_headerext_common_get_messageid();
	
	process_message(messagetype, messagegroupid, messageid);
}

int main(void)
{
	uint8_t loop = 0;
	uint8_t i;
	uint8_t button = 0;
	uint8_t button_old = 0;
	uint8_t button_debounce = 0;

	// delay 1s to avoid further communication with uart or RFM12 when my programmer resets the MC after 500ms...
	_delay_ms(1000);

	util_init();
	
	check_eeprom_compatibility(DEVICETYPE_POWERSWITCH);

	for (i = 0; i < SWITCH_COUNT; i++)
	{
		sbi(DDRC, RELAIS_PIN_START + i);
	}

	// init button input
	cbi(BUTTON_DDR, BUTTON_PIN);
	sbi(BUTTON_PORT, BUTTON_PIN);
	
	// read packetcounter, increase by cycle and write back
	packetcounter = e2p_generic_get_packetcounter() + PACKET_COUNTER_WRITE_CYCLE;
	e2p_generic_set_packetcounter(packetcounter);

	// read last received station packetcounter
	station_packetcounter = e2p_powerswitch_get_basestationpacketcounter();
	
	// read device id
	device_id = e2p_generic_get_deviceid();

	osccal_init();

	uart_init();

	UART_PUTS ("\r\n");
	UART_PUTF4("smarthomatic Power Switch v%u.%u.%u (%08lx)\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_HASH);
	UART_PUTS("(c) 2013..2014 Uwe Freese, www.smarthomatic.org\r\n");
	osccal_info();
	UART_PUTF ("DeviceID: %u\r\n", device_id);
	UART_PUTF ("PacketCounter: %lu\r\n", packetcounter);
	print_switch_state();
	UART_PUTF ("Last received base station PacketCounter: %u\r\n\r\n", station_packetcounter);
	
	// init AES key
	e2p_generic_get_aeskey(aes_key);

	led_blink(500, 500, 3);

	// read (saved) switch state from before the eventual powerloss
	for (i = 0; i < SWITCH_COUNT; i++)
	{
		switchRelais(i, e2p_powerswitch_get_switchstate(i), e2p_powerswitch_get_switchtimeout(i));
	}

	rfm12_init();

	sei();

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
				memcpy(bufx, rfm12_rx_buffer(), len);
				
				UART_PUTS("Before decryption: ");
				print_bytearray(bufx, len);
					
				aes256_decrypt_cbc(bufx, len);

				UART_PUTS("Decrypted bytes: ");
				print_bytearray(bufx, len);

				/*
				uint32_t assumed_crc = getBuf32(0);
				uint32_t actual_crc = crc32(bufx + 4, len - 4);
				
				UART_PUTF("Received CRC32 would be %lx\r\n", assumed_crc);
				UART_PUTF("Re-calculated CRC32 is  %lx\r\n", actual_crc);

				if (assumed_crc != actual_crc)
				*/
				
				if (!pkg_header_check_crc32(len))
				{
					UART_PUTS("Received garbage (CRC wrong after decryption).\r\n");
				}
				else
				{
					process_packet(len);
				}				
			}

			// tell the implementation that the buffer can be reused for the next data.
			rfm12_rx_clear();
		}

		// flash LED every second to show the device is alive
		if (loop == 50)
		{
			if (switch_timeout[0])
			{
				led_blink(10, 10, 1);
			}

			loop = 0;

			// Check timeouts and toggle switches
			for (i = 0; i < SWITCH_COUNT; i++)
			{
				if (switch_timeout[i])
				{
					switch_timeout[i]--;
					
					if (switch_timeout[i] == 0)
					{
						UART_PUTS("Timeout! ");
						switchRelais(i, !switch_state[i], 0);
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
				led_blink(200, 0, 1);
				
				version_status_cycle++;
			}
			else if (version_status_cycle >= SEND_VERSION_STATUS_CYCLE)
			{
				version_status_cycle = 0;
				send_version_status();
				led_blink(200, 0, 1);
			}
		}
		else
		{
			_delay_ms(20);
		}

		switch_led(switch_state[0]);

		rfm12_tick();

		button = !(BUTTON_PINPORT & (1 << BUTTON_PIN));
		
		if (button_debounce > 0)
		{
			button_debounce--;
		}
		else if (button != button_old)
		{
			button_old = button;
			button_debounce = 10;
			
			if (button) // on button press
			{
				UART_PUTS("Button! ");
				switchRelais(0, !switch_state[0], 0);
				send_status_timeout = 15; // send status after 15s
			}
		}	

		loop++;
	}
	
	// never called
	// aes256_done(&aes_ctx);
}
