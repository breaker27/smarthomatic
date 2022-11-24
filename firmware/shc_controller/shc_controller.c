/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2022 Uwe Freese
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
#include "../src_common/msggrp_gpio.h"

#include "../src_common/e2p_hardware.h"
#include "../src_common/e2p_generic.h"
#include "../src_common/e2p_controller.h"

#include "../src_common/aes256.h"
#include "../src_common/util.h"
#include "version.h"

#define BUTTON_DDR DDRD // TODO: Configurable pins like in env sensor
#define BUTTON_PORT PORTD
#define BUTTON_PINPORT PIND
#define BUTTON_PIN 3

#define SWITCH_DDR DDRB // TODO: Configurable pins like in env sensor
#define SWITCH_PORT PORTB
#define SWITCH_PINPORT PINB
#define SWITCH_PIN 7

// Power of RFM12B (since PCB rev 1.1) or RFM12 NRES (Reset) pin may be connected to PC3.
// If not, only sw reset is used.
#define RFM_RESET_PIN 3
#define RFM_RESET_PORT_NR 1
#define RFM_RESET_PIN_STATE 1

#include "../src_common/util_watchdog_init.h"

uint16_t device_id;
uint32_t station_packetcounter;

uint16_t port_status_cycle;
uint16_t version_status_cycle;
uint16_t send_status_timeout = 5;

void send_deviceinfo_status(void)
{
	inc_packetcounter();

	UART_PUTF("Send DeviceInfo: DeviceType %u,", DEVICETYPE_CONTROLLER);
	UART_PUTF4(" v%u.%u.%u (%08lx)\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_HASH);

	// Set packet content
	pkg_header_init_generic_deviceinfo_status();
	pkg_header_set_senderid(device_id);
	pkg_header_set_packetcounter(packetcounter);
	msg_generic_deviceinfo_set_devicetype(DEVICETYPE_CONTROLLER);
	msg_generic_deviceinfo_set_versionmajor(VERSION_MAJOR);
	msg_generic_deviceinfo_set_versionminor(VERSION_MINOR);
	msg_generic_deviceinfo_set_versionpatch(VERSION_PATCH);
	msg_generic_deviceinfo_set_versionhash(VERSION_HASH);

	rfm12_send_bufx();
}

// Process a request to this device.
// React accordingly on the MessageType, MessageGroup and MessageID
// and send an Ack in any case. It may be an error ack if request is not supported.
void process_request(MessageTypeEnum messagetype, uint32_t messagegroupid, uint32_t messageid)
{
	/*
	// remember some values before the packet buffer is destroyed
	uint32_t acksenderid = pkg_header_get_senderid();
	uint32_t ackpacketcounter = pkg_header_get_packetcounter();

	UART_PUTF("MessageGroupID:%u;", messagegroupid);

	if (messagegroupid != MESSAGEGROUP_GPIO)
	{
		UART_PUTS("\r\nERR: Unsupported MessageGroupID.\r\n");
		send_ack(acksenderid, ackpacketcounter, true);
		return;
	}

	UART_PUTF("MessageID:%u;", messageid);

	switch (messageid)
	{
		case MESSAGEID_GPIO_DIGITALPORT:
			process_gpio_digitalport(messagetype);
			break;
		case MESSAGEID_GPIO_DIGITALPIN:
			process_gpio_digitalpin(messagetype);
			break;
		case MESSAGEID_GPIO_DIGITALPORTTIMEOUT:
			process_gpio_digitalporttimeout(messagetype);
			break;
		case MESSAGEID_GPIO_DIGITALPINTIMEOUT:
			process_gpio_digitalpintimeout(messagetype);
			break;
		default:
			UART_PUTS("\r\nERR: Unsupported MessageID.");
			send_ack(acksenderid, ackpacketcounter, true);
			return;
	}

	UART_PUTS("\r\n");

	// In all cases, use the digitalporttimeout message as answer.
	// It contains the data for *all* pins and *all* timer values.

	// "Set" -> send "Ack"
	if (messagetype == MESSAGETYPE_SET)
	{
		pkg_header_init_gpio_digitalporttimeout_ack();

		UART_PUTS("Sending Ack\r\n");
	}
	// "Get" or "SetGet" -> send "AckStatus"
	else
	{
		pkg_header_init_gpio_digitalporttimeout_ackstatus();

		uint8_t i;

		// react on changed state (version for more than one switch...)
		for (i = 0; i < RELAIS_COUNT; i++)
		{
			// set command state incl. current timeout
			msg_gpio_digitalporttimeout_set_on(i, cmd_state[i]);
			msg_gpio_digitalporttimeout_set_timeoutsec(i, cmd_timeout[i]);

			// set physical manual switch states incl. current timeout with offset 3
			msg_gpio_digitalporttimeout_set_on(i + 3, switch_state_physical[i]);
			msg_gpio_digitalporttimeout_set_timeoutsec(i + 3,
				switch_state_physical[i] ? switch_on_delay_counter[i] : switch_off_delay_counter[i]);

			// set relais state with offset 6
			if (i < 2)
				msg_gpio_digitalporttimeout_set_on(i + 6, relais_state[i]);
		}

		UART_PUTS("Sending AckStatus\r\n");
	}

	send_ack(acksenderid, ackpacketcounter, false);
	send_status_timeout = 5;
	*/
}

// Check if incoming message is a legitimate request for this device.
// If not, ignore it.
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

	e2p_controller_set_basestationpacketcounter(station_packetcounter);

	// check MessageType
	MessageTypeEnum messagetype = pkg_header_get_messagetype();
	UART_PUTF("MessageType:%u;", messagetype);

	if ((messagetype != MESSAGETYPE_GET) && (messagetype != MESSAGETYPE_SET) && (messagetype != MESSAGETYPE_SETGET))
	{
		UART_PUTS("\r\nERR: Unsupported MessageType.\r\n");
		return;
	}

	// check device id
	uint16_t rcv_id = pkg_headerext_common_get_receiverid();

	UART_PUTF("ReceiverID:%u;", rcv_id);

	if (rcv_id != device_id)
	{
		UART_PUTS("\r\nWRN: DeviceID does not match.\r\n");
		return;
	}

	// check MessageGroup + MessageID
	uint32_t messagegroupid = pkg_headerext_common_get_messagegroupid();
	uint32_t messageid = pkg_headerext_common_get_messageid();

	process_request(messagetype, messagegroupid, messageid);
}

int main(void)
{
	uint8_t loop = 0;
	uint8_t i;
	uint8_t button = 0;
	uint8_t button_old = 0;
	uint8_t button_debounce = 0;
	uint16_t version_status_cycle;
	uint16_t version_status_cycle_counter;

	// delay 1s to avoid further communication with uart or RFM12 when my programmer resets the MC after 500ms...
	_delay_ms(1000);

	util_init();

	check_eeprom_compatibility(DEVICETYPE_CONTROLLER);

	// init button input
//	cbi(BUTTON_DDR, BUTTON_PIN);
//	sbi(BUTTON_PORT, BUTTON_PIN);

	// read packetcounter, increase by cycle and write back
	packetcounter = e2p_generic_get_packetcounter() + PACKET_COUNTER_WRITE_CYCLE;
	e2p_generic_set_packetcounter(packetcounter);

	// read last received station packetcounter
	station_packetcounter = e2p_controller_get_basestationpacketcounter();

	port_status_cycle = (uint16_t)e2p_controller_get_statuscycle() * 60;
	version_status_cycle = (uint16_t)(90000UL / port_status_cycle); // once every 25 hours
	version_status_cycle_counter = version_status_cycle - 1; // send right after startup

	// read device id
	device_id = e2p_generic_get_deviceid();

	osccal_init();

	uart_init();

	UART_PUTS ("\r\n");
	UART_PUTF4("smarthomatic Controller v%u.%u.%u (%08lx)\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_HASH);
	UART_PUTS("(c) 2022 Uwe Freese, www.smarthomatic.org\r\n");
	osccal_info();
	UART_PUTF ("DeviceID: %u\r\n", device_id);
	UART_PUTF ("PacketCounter: %lu\r\n", packetcounter);
	UART_PUTF ("Last received base station PacketCounter: %u\r\n", station_packetcounter);
	UART_PUTF ("Port status cycle: %us\r\n", port_status_cycle);
	UART_PUTF ("Version status cycle: %u * port status cycle\r\n", version_status_cycle);

	// init AES key
	e2p_generic_get_aeskey(aes_key);

	led_blink(500, 500, 3);

	UART_PUTS ("\r\n");

	rfm_watchdog_init(device_id, e2p_controller_get_transceiverwatchdogtimeout(), RFM_RESET_PORT_NR, RFM_RESET_PIN, RFM_RESET_PIN_STATE);
	rfm12_init();

	sei();

	while (42)
	{
		if (rfm12_rx_status() == STATUS_COMPLETE)
		{
			uint8_t len = rfm12_rx_len();

			rfm_watchdog_alive();

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

		// tasks that are done every second
		if (loop == 50)
		{
			loop = 0;

			// when timeout active, flash LED
			/*if (cmd_timeout[0])
			{
				led_blink(10, 10, 1);
			}
			else
			{
				_delay_ms(20);
// 			}*/

			// send status from time to time
			if (!send_startup_reason(&mcusr_mirror))
			{
				send_status_timeout--;

				if (send_status_timeout == 0)
				{
//					send_status_timeout = port_status_cycle;
//					send_gpio_digitalporttimeout_status();
					led_blink(200, 0, 1);

					version_status_cycle_counter++;
				}
				else if (version_status_cycle_counter >= version_status_cycle)
				{
					version_status_cycle_counter = 0;
//					send_deviceinfo_status();
					led_blink(200, 0, 1);
				}
			}
		}
		else
		{
			_delay_ms(20);
		}

		rfm_watchdog_count(20);

		rfm12_tick();

		//TODO: Button handling.

		loop++;
	}

	// never called
	// aes256_done(&aes_ctx);
}
