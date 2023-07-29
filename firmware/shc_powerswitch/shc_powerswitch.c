/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2013..2023 Uwe Freese
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
#include "../src_common/e2p_powerswitch.h"

#include "../src_common/aes256.h"
#include "../src_common/util.h"
#include "version.h"

// Wording:
// "RELAIS" is used for the connected relais to switch.
// "BUTTON" is a manual button to toggle the relais (only one supported currently).
// "SWITCH" is a manual "override" switch to switch the relais (only one supported currently).
//          The behaviour depends on the according E2P setting.

#define RELAIS_COUNT 3 // Don't change! (PC0 to PC2 are supported)
#define RELAIS_PORT PORTC // TODO: Configurable pins like in env sensor
#define RELAIS_PIN_START 0

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
#define RFM_RESET_PIN_STATE 0

#include "../src_common/util_watchdog_init.h"

uint16_t device_id;
uint32_t station_packetcounter;

bool cmd_state[RELAIS_COUNT];             // state as requested by command
bool switch_state_physical[RELAIS_COUNT]; // the physical state of the manual switch according I/O pin voltage level
bool switch_state_logical[RELAIS_COUNT];  // the logical switch state, considering the on/off delays
bool relais_state[RELAIS_COUNT];          // resulting relais state as combination of cmd_state and switch_state_logical
uint16_t cmd_timeout[RELAIS_COUNT];       // timeout as requested by command
uint8_t switch_mode[RELAIS_COUNT];        // defines how cmd_state and switch_state_logical are combined to set relais state
uint16_t switch_on_delay[RELAIS_COUNT];   // delay until switch 'on' state is considered
uint16_t switch_off_delay[RELAIS_COUNT];  // delay until switch 'off' state is considered
uint16_t switch_on_delay_counter[RELAIS_COUNT];  // count the seconds until switch state change is considered
uint16_t switch_off_delay_counter[RELAIS_COUNT]; // count the seconds until switch state change is considered
uint16_t port_status_cycle;
uint16_t version_status_cycle;            // send version status x times less than switch status (~once per day)
uint16_t version_status_cycle_counter;    // send early after startup
uint16_t send_status_timeout = 5;

void print_states(void)
{
	uint8_t i;

	for (i = 1; i <= RELAIS_COUNT; i++)
	{
		UART_PUTF("Relais %u: CMD=", i);

		if (cmd_state[i - 1])
		{
			UART_PUTS("ON");
		}
		else
		{
			UART_PUTS("OFF");
		}

		if (cmd_timeout[i - 1])
		{
			UART_PUTF(" (Timeout: %us)", cmd_timeout[i - 1]);
		}

		UART_PUTS(", SWITCH PHY=");

		if (switch_state_physical[i - 1])
		{
			UART_PUTS("ON");
		}
		else
		{
			UART_PUTS("OFF");
		}

		UART_PUTS(", SWITCH LOG=");

		if (switch_state_logical[i - 1])
		{
			UART_PUTS("ON");
		}
		else
		{
			UART_PUTS("OFF");
		}

		UART_PUTF(" (Mode: %u) -> RELAIS=", switch_mode[i - 1]);

		if (relais_state[i - 1])
		{
			UART_PUTS("ON");
		}
		else
		{
			UART_PUTS("OFF");
		}

		UART_PUTS("\r\n");
	}
}

void send_gpio_digitalporttimeout_status(void)
{
	uint8_t i;

	UART_PUTS("Sending GPIO DigitalPortTimeout Status:\r\n");

	print_states();

	inc_packetcounter();

	// Set packet content
	pkg_header_init_gpio_digitalporttimeout_status();
	pkg_header_set_senderid(device_id);
	pkg_header_set_packetcounter(packetcounter);

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

	rfm12_send_bufx();
}

void send_deviceinfo_status(void)
{
	inc_packetcounter();

	UART_PUTF("Send DeviceInfo: DeviceType %u,", DEVICETYPE_POWERSWITCH);
	UART_PUTF4(" v%u.%u.%u (%08lx)\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_HASH);

	// Set packet content
	pkg_header_init_generic_deviceinfo_status();
	pkg_header_set_senderid(device_id);
	pkg_header_set_packetcounter(packetcounter);
	msg_generic_deviceinfo_set_devicetype(DEVICETYPE_POWERSWITCH);
	msg_generic_deviceinfo_set_versionmajor(VERSION_MAJOR);
	msg_generic_deviceinfo_set_versionminor(VERSION_MINOR);
	msg_generic_deviceinfo_set_versionpatch(VERSION_PATCH);
	msg_generic_deviceinfo_set_versionhash(VERSION_HASH);

	rfm12_send_bufx();
}

// Calculate the relais state out of cmd_state, switch_state_logical and switch_mode
// and switch the relais.
void update_relais_states(void)
{
	uint8_t i;
	uint8_t relaisStateOld;

	for (i = 0; i < RELAIS_COUNT; i++)
	{
		relaisStateOld = relais_state[i];

		switch (switch_mode[i])
		{
			case SWITCHMODE_SW:
				relais_state[i] = switch_state_logical[i];
				break;
			case SWITCHMODE_NOT_SW:
				relais_state[i] = !switch_state_logical[i];
				break;
			case SWITCHMODE_CMD_AND_SW:
				relais_state[i] = cmd_state[i] && switch_state_logical[i];
				break;
			case SWITCHMODE_CMD_AND_NOT_SW:
				relais_state[i] = cmd_state[i] && (!switch_state_logical[i]);
				break;
			case SWITCHMODE_CMD_OR_SW:
				relais_state[i] = cmd_state[i] || switch_state_logical[i];
				break;
			case SWITCHMODE_CMD_OR_NOT_SW:
				relais_state[i] = cmd_state[i] || (!switch_state_logical[i]);
				break;
			case SWITCHMODE_CMD_XOR_SW:
				relais_state[i] = cmd_state[i] ^ switch_state_logical[i];
				break;
			case SWITCHMODE_CMD_XOR_NOT_SW:
				relais_state[i] = cmd_state[i] ^ (!switch_state_logical[i]);
				break;
			default: // SWITCHMODE_CMD (= default) and all other values
				relais_state[i] = cmd_state[i];
				break;
		}

		if (relaisStateOld != relais_state[i])
		{
			if (relais_state[i])
			{
				sbi(RELAIS_PORT, RELAIS_PIN_START + i);

				if (i == 0)
				{
					switch_led(1);
				}
			}
			else
			{
				cbi(RELAIS_PORT, RELAIS_PIN_START + i);

				if (i == 0)
				{
					switch_led(0);
				}
			}
		}
	}
}

// Switch the relais according explicit parameters and switch_mode set in e2p.
void set_cmd_state(int8_t num, bool on, uint16_t timeout, bool dbgmsg)
{
	if (dbgmsg)
	{
		UART_PUTF3("Switching cmd state %u to %u with timeout %us.\r\n", num + 1, on, timeout);
	}

	if (num >= RELAIS_COUNT)
	{
		UART_PUTF("\r\nRelais number %u > RELAIS_COUNT, ignoring.", num);
		return;
	}

	cmd_state[num] = on;
	cmd_timeout[num] = timeout;

	if (e2p_powerswitch_get_cmdstate(num) != on)
	{
		e2p_powerswitch_set_cmdstate(num, on);
	}

	if (e2p_powerswitch_get_cmdtimeout(num) != timeout)
	{
		e2p_powerswitch_set_cmdtimeout(num, timeout);
	}
}

// Read the states of the manual switches (from IO pins).
// Currently only one switch is supported.
// Return if there was any change.
bool update_switch_states(bool ignore_delay)
{
	uint8_t oldState;
	bool change = false;

	if (switch_mode[0] != SWITCHMODE_CMD)
	{
		oldState = switch_state_physical[0];
		switch_state_physical[0] = !(SWITCH_PINPORT & (1 << SWITCH_PIN));

		if (oldState != switch_state_physical[0]) {
			change = true;

			if (switch_state_physical[0]) { // ON
				if (switch_off_delay_counter[0]) // off delay still running, phy switch still on
					switch_off_delay_counter[0] = 0;
				else
				{
					if (ignore_delay || (switch_on_delay[0] == 0))
						switch_state_logical[0] = switch_state_physical[0];
					else
						switch_on_delay_counter[0] = switch_on_delay[0];
				}
			}
			else
			{ // OFF
				if (switch_on_delay_counter[0]) // on delay still running, phy switch still off
					switch_on_delay_counter[0] = 0;
				else
				{
					if (ignore_delay || (switch_off_delay[0] == 0))
						switch_state_logical[0] = switch_state_physical[0];
					else
						switch_off_delay_counter[0] = switch_off_delay[0];
				}
			}
		}
	}

	return change;
}

void process_gpio_digitalport(MessageTypeEnum messagetype)
{
	// "Set" or "SetGet" -> modify switch state
	if ((messagetype == MESSAGETYPE_SET) || (messagetype == MESSAGETYPE_SETGET))
	{
		uint8_t i;

		// react on changed state (version for more than one switch...)
		for (i = 0; i < RELAIS_COUNT; i++)
		{
			bool req_on = msg_gpio_digitalport_get_on(i);
			UART_PUTF2("On[%u]:%u;", i, req_on);
			set_cmd_state(i, req_on, 0, false);
			update_relais_states();
		}
	}
}

void process_gpio_digitalpin(MessageTypeEnum messagetype)
{
	// "Set" or "SetGet" -> modify switch state
	if ((messagetype == MESSAGETYPE_SET) || (messagetype == MESSAGETYPE_SETGET))
	{
		uint8_t req_pos = msg_gpio_digitalpin_get_pos();
		bool req_on = msg_gpio_digitalpin_get_on();
		UART_PUTF2("Pos:%u;On:%u;", req_pos, req_on);
		set_cmd_state(req_pos, req_on, 0, false);
		update_relais_states();
	}
}

void process_gpio_digitalporttimeout(MessageTypeEnum messagetype)
{
	// "Set" or "SetGet" -> modify switch state
	if ((messagetype == MESSAGETYPE_SET) || (messagetype == MESSAGETYPE_SETGET))
	{
		uint8_t i;

		// react on changed state (version for more than one switch...)
		for (i = 0; i < RELAIS_COUNT; i++)
		{
			bool req_on = msg_gpio_digitalporttimeout_get_on(i);
			uint16_t req_timeout = msg_gpio_digitalporttimeout_get_timeoutsec(i);

			UART_PUTF2("On[%u]:%u;", i, req_on);
			UART_PUTF2("TimeoutSec[%u]:%u;", i, req_timeout);

			set_cmd_state(i, req_on, req_timeout, false);
			update_relais_states();
		}
	}
}

void process_gpio_digitalpintimeout(MessageTypeEnum messagetype)
{
	// "Set" or "SetGet" -> modify switch state
	if ((messagetype == MESSAGETYPE_SET) || (messagetype == MESSAGETYPE_SETGET))
	{
		uint8_t req_pos = msg_gpio_digitalpintimeout_get_pos();
		bool req_on = msg_gpio_digitalpintimeout_get_on();
		uint16_t req_timeout = msg_gpio_digitalpintimeout_get_timeoutsec();
		UART_PUTF2("Pos:%u;On:%u;", req_pos, req_on);
		UART_PUTF("TimeoutSec:%u;", req_timeout);
		set_cmd_state(req_pos, req_on, req_timeout, false);
		update_relais_states();
	}
}

void send_ack(uint32_t acksenderid, uint32_t ackpacketcounter, bool error)
{
	// any message can be used as ack, because they are the same anyway
	if (error)
	{
		UART_PUTS("Send error Ack\r\n");
		pkg_header_init_gpio_digitalporttimeout_ack();
	}

	inc_packetcounter();

	// set common fields
	pkg_header_set_senderid(device_id);
	pkg_header_set_packetcounter(packetcounter);

	pkg_headerext_common_set_acksenderid(acksenderid);
	pkg_headerext_common_set_ackpacketcounter(ackpacketcounter);
	pkg_headerext_common_set_error(error);

	rfm12_send_bufx();
}

// Process a request to this device.
// React accordingly on the MessageType, MessageGroup and MessageID
// and send an Ack in any case. It may be an error ack if request is not supported.
void process_request(MessageTypeEnum messagetype, uint32_t messagegroupid, uint32_t messageid)
{
	// remember some values before the packet buffer is destroyed
	uint32_t acksenderid = pkg_header_get_senderid();
	uint32_t ackpacketcounter = pkg_header_get_packetcounter();

	UART_PUTF("MessageGroupID:%u;", messagegroupid);

	if (messagegroupid != MESSAGEGROUP_GPIO)
	{
		UART_PUTS("\r\nERR: Unsupported MessageGroupID.\r\n");
		send_ack(acksenderid, ackpacketcounter, true);
		rfm12_send_wait_led();
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
			rfm12_send_wait_led();
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
	rfm12_send_wait_led();
	send_status_timeout = 5;
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

	// delay 1s to avoid further communication with uart or RFM12 when my programmer resets the MC after 500ms...
	_delay_ms(1000);

	util_init();

	check_eeprom_compatibility(DEVICETYPE_POWERSWITCH);

	for (i = 0; i < RELAIS_COUNT; i++)
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

	port_status_cycle = (uint16_t)e2p_powerswitch_get_statuscycle() * 60;
	version_status_cycle = (uint16_t)(90000UL / port_status_cycle); // once every 25 hours
	version_status_cycle_counter = version_status_cycle - 1; // send right after startup

	// read device id
	device_id = e2p_generic_get_deviceid();

	// read switch mode
	for (i = 0; i < RELAIS_COUNT; i++)
	{
		switch_mode[i] = e2p_powerswitch_get_switchmode(i);
		switch_on_delay[i] = e2p_powerswitch_get_switchondelay(i);
		switch_off_delay[i] = e2p_powerswitch_get_switchoffdelay(i);
	}

	// Init IO pin(s) for switches.
	// Only one switch is currently supported!
	if (switch_mode[0] != SWITCHMODE_CMD)
	{
		cbi(SWITCH_DDR, SWITCH_PIN);
		sbi(SWITCH_PORT, SWITCH_PIN);
	}

	osccal_init();

	uart_init();

	UART_PUTS ("\r\n");
	UART_PUTF4("smarthomatic Power Switch v%u.%u.%u (%08lx)\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_HASH);
	UART_PUTS("(c) 2013..2022 Uwe Freese, www.smarthomatic.org\r\n");
	osccal_info();
	UART_PUTF ("DeviceID: %u\r\n", device_id);
	UART_PUTF ("PacketCounter: %lu\r\n", packetcounter);
	UART_PUTF ("Last received base station PacketCounter: %u\r\n", station_packetcounter);
	UART_PUTF ("Port status cycle: %us\r\n", port_status_cycle);
	UART_PUTF ("Version status cycle: %u * port status cycle\r\n", version_status_cycle);

	// init AES key
	e2p_generic_get_aeskey(aes_key);

	led_blink(500, 500, 3);

	// read (saved) cmd state from before the eventual powerloss
	for (i = 0; i < RELAIS_COUNT; i++)
	{
		set_cmd_state(i, e2p_powerswitch_get_cmdstate(i), e2p_powerswitch_get_cmdtimeout(i), true);
	}

	update_switch_states(true);
	update_relais_states();
	print_states();

	UART_PUTS ("\r\n");

	rfm_watchdog_init(device_id, e2p_powerswitch_get_transceiverwatchdogtimeout(), RFM_RESET_PORT_NR, RFM_RESET_PIN, RFM_RESET_PIN_STATE);
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
			if (cmd_timeout[0])
			{
				rfm12_delay10_led();
			}
			else
			{
				rfm12_delay20();
			}

			// Count down switch delays
			for (i = 0; i < RELAIS_COUNT; i++)
			{
				if (switch_on_delay_counter[i] > 0) {
					switch_on_delay_counter[i]--;
					UART_PUTF("Switch on in %us\r\n", switch_on_delay_counter[0]);

					if (switch_on_delay_counter[i] == 0) {
						switch_state_logical[i] = switch_state_physical[i];
						UART_PUTF("Logical switch state changed to %u\r\n", switch_state_logical[0]);
						update_relais_states();
						send_status_timeout = 1;
					}
				}

				if (switch_off_delay_counter[i] > 0) {
					switch_off_delay_counter[i]--;
					UART_PUTF("Switch off in %us\r\n", switch_off_delay_counter[0]);

					if (switch_off_delay_counter[i] == 0) {
						switch_state_logical[i] = switch_state_physical[i];
						UART_PUTF("Logical switch state changed to %u\r\n", switch_state_logical[0]);
						update_relais_states();
						send_status_timeout = 1;
					}
				}
			}

			// Check timeouts and toggle switches
			for (i = 0; i < RELAIS_COUNT; i++)
			{
				if (cmd_timeout[i])
				{
					cmd_timeout[i]--;

					if (cmd_timeout[i] == 0)
					{
						UART_PUTS("Timeout! ");
						set_cmd_state(i, !cmd_state[i], 0, true);
						update_relais_states();
						send_status_timeout = 1;
					}
				}
			}

			// send status from time to time
			if (!send_startup_reason(&mcusr_mirror))
			{
				send_status_timeout--;

				if (send_status_timeout == 0)
				{
					send_status_timeout = port_status_cycle;
					send_gpio_digitalporttimeout_status();
					rfm12_send_wait_led();

					version_status_cycle_counter++;
				}
				else if (version_status_cycle_counter >= version_status_cycle)
				{
					version_status_cycle_counter = 0;
					send_deviceinfo_status();
					rfm12_send_wait_led();
				}
			}
		}
		else
		{
			rfm12_delay20();
		}

		switch_led(cmd_state[0]);

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
				set_cmd_state(0, !cmd_state[0], 0, true);
				update_relais_states();
				// send status after 15s delay to avoid sending it once per
				// second in case the button toggles all the time
				send_status_timeout = 15;
			}
		}

		if (loop % 12 == 0) // update every 240ms
		{
			if (update_switch_states(false))
			{
				UART_PUTF("Physical switch state changed to %u\r\n", switch_state_physical[0]);
				update_relais_states();
				// send status after 15s delay to avoid sending it once per
				// second in case the switch toggles all the time
				send_status_timeout = 15;
			}
		}

		loop++;
	}

	// never called
	// aes256_done(&aes_ctx);
}
