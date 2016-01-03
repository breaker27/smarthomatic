/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2015 Uwe Freese
*               2015 Andreas Richter
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

#include "../src_common/aes256.h"
#include "../src_common/util.h"
#include "version.h"

#include "../src_common/util_watchdog_init.h"

#include "Dcf77.h"

#define SEND_STATUS_EVERY_SEC 1800 // how often should a status be sent?
#define SEND_VERSION_STATUS_CYCLE 50 // send version status x times less than switch status (~once per day)

#define DCF_PINREG PINC
#define DCF_PORTREG PORTC
#define DCF_PIN 5

uint16_t device_id;
uint32_t station_packetcounter;

uint16_t send_status_timeout = 5;
uint8_t version_status_cycle = SEND_VERSION_STATUS_CYCLE - 1; // send promptly after startup

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

void send_time_currenttime_status(void)
{
	// TODO: Send current time status.
}

void process_gpio_digitalpintimeout(MessageTypeEnum messagetype)
{
	// TODO: Process received temperature
}

// Process a request to this device.
// React accordingly on the MessageType, MessageGroup and MessageID
// and send an Ack in any case. It may be an error ack if request is not supported.
void process_request(MessageTypeEnum messagetype, uint32_t messagegroupid, uint32_t messageid)
{
	// remember some values before the packet buffer is destroyed
/*	uint32_t acksenderid = pkg_header_get_senderid();
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

	// In all cases, use the digitalporttimer message as answer.
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
		for (i = 0; i < SWITCH_COUNT; i++)
		{
			// set message data
			msg_gpio_digitalporttimeout_set_on(i, switch_state[i]);
			msg_gpio_digitalporttimeout_set_timeoutsec(i, switch_timeout[i]);
		}
		
		UART_PUTS("Sending AckStatus\r\n");
	}

	send_ack(acksenderid, ackpacketcounter, false);
	send_status_timeout = 5; */
}

// Check if incoming message is a legitimate request for this device.
// If not, ignore it.
void process_packet(uint8_t len)
{
	pkg_header_adjust_offset();

	// check SenderID
	/*
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
	
	process_request(messagetype, messagegroupid, messageid);*/
}

#define USE_TIMER1
#ifdef USE_TIMER1
#define TIMER1_FREQ 100
#define T1_PRESCALE 8
#define T1_OVL(FREQ) ((F_CPU/T1_PRESCALE/FREQ)-1)
#define T1_MAX T1_OVL(TIMER1_FREQ)

#if ((T1_MAX) > 0xffff)
#error overflow, increase prescaler
#endif

inline void timer1_init(void)
{
	OCR1A = 0;
	OCR1B = 0;
	ICR1 = T1_MAX-1;
	TCCR1A =	(0<<WGM11)|(0<<WGM10);
	
	uint8_t tccr1b = (1<<WGM12)|(1<<WGM13);	// Mode12
	
#if (T1_PRESCALE == 1)
	tccr1b|= (1<<CS10);
#elif (T1_PRESCALE == 8)
	tccr1b|= (1<<CS11);
#elif (T1_PRESCALE == 64)
	tccr1b|= (1<<CS10)|(1<<CS11);
#elif (T1_PRESCALE == 256)
	tccr1b|= (1<<CS12);
#elif (T1_PRESCALE == 1024)
	tccr1b|= (1<<CS10)|(1<<CS12);
#else
 #error unknown prescaler
#endif
	
	TCCR1B = tccr1b;
	TIMSK1 = (1<<ICIE1);
}


// Called every 10ms.
ISR (TIMER1_CAPT_vect)
#else

// Timer0 is used to detect length of DFC pulses.
// 1 clock pulse = 1 ms
// -> set overflow to 10
void timer0_init(void)
{
	// Clock source = I/O clock, 1/1024 prescaler
	// -> 1000 Hz @ 1 MHz CPU clock
	TCCR0B = (1 << CS02) | (1 << CS00);

	// Clear timer on compare match
	TCCR0A = (1 << WGM01);

	// set compare match value to 10 = 10 ms @ 1 MHz CPU clock
	OCR0A = 100;

	// Timer/Counter0 Output Compare Match A Interrupt Enable
	TIMSK0 = (1 << OCIE0A);
}

// Called every 10ms.
ISR (TIMER0_COMPA_vect)
#endif
{
	dcf77_timer100((DCF_PINREG & (1 << DCF_PIN)) == 0);
}

int main(void)
{
	uint8_t loop = 0;
	
	uint8_t state = 0;
	uint8_t last_state = -1;
	
	int8_t error = 0;
	int8_t last_error = 0;
	
	DateTime now = {0};

	// delay 1s to avoid further communication with uart or RFM12 when my programmer resets the MC after 500ms...
	_delay_ms(1000);

	util_init();
	
	//check_eeprom_compatibility(DEVICETYPE_POWERSWITCH);

	// read packetcounter, increase by cycle and write back
	packetcounter = e2p_generic_get_packetcounter() + PACKET_COUNTER_WRITE_CYCLE;
	e2p_generic_set_packetcounter(packetcounter);

	// read device id
	device_id = e2p_generic_get_deviceid();

	osccal_init();

	uart_init();

	UART_PUTS ("\r\n");
	UART_PUTF4("smarthomatic Clock v%u.%u.%u (%08lx)\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_HASH);
	UART_PUTS("(c) 2015 Uwe Freese, www.smarthomatic.org\r\n");
	osccal_info();
	UART_PUTF ("DeviceID: %u\r\n", device_id);
	UART_PUTF ("PacketCounter: %lu\r\n", packetcounter);
	
	// init AES key
	e2p_generic_get_aeskey(aes_key);

	led_blink(500, 500, 3);

//	rfm_watchdog_init(device_id, e2p_clock_get_transceiverwatchdogtimeout());
	rfm12_init();

	sei();

	// set pullup for DCF receiver
	sbi(DCF_PORTREG, DCF_PIN);

#ifdef USE_TIMER1
	timer1_init();
#else
	timer0_init();
#endif

	while (1)
	{
		state = dcf77_get_rcv_state();
		if (state != last_state)
		{
			last_state = state;
			static const char* StStr[] = {"NO_SIGNAL", "RCV_PROGRESS", "RCV_OK"};
			UART_PUTF("DCF State: %s\r\n", StStr[state]);
		}
		error = dcf77_get_last_error();
		if (error != last_error)
		{
			last_error = error;
			if (error < 0)
			{
				UART_PUTF("DCF Error: %d\r\n", error);
			}
		}
		
		if (dcf77_get_current(&now))
		{
			static const char* WdStr[] = {"Mo","Di","Mi","Do","Fr","Sa","So"};
			UART_PUTF4("Date: %u.%u.%u %s\r\n", now.Day, now.Month, now.Year, WdStr[now.WDay]);
			UART_PUTF3("Time: %u:%u:%u\r\n", now.Hour, now.Minute, dcf77_get_seconds());
		}
		
	}

	while (42)
	{
		if (rfm12_rx_status() == STATUS_COMPLETE)
		{
			uint8_t len = rfm12_rx_len();
			
			//rfm_watchdog_reset();
			
			if ((len == 0) || (len % 16 != 0))
			{
				UART_PUTF("Received garbage (%u bytes not multiple of 16): ", len);
				print_bytearray(bufx, len);
			}
			else // try to decrypt with all keys stored in EEPROM
			{
				memcpy(bufx, rfm12_rx_buffer(), len);
				
				//UART_PUTS("Before decryption: ");
				//print_bytearray(bufx, len);
					
				aes256_decrypt_cbc(bufx, len);

				//UART_PUTS("Decrypted bytes: ");
				//print_bytearray(bufx, len);

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
			loop = 0;
			
			// send status from time to time
			if (!send_startup_reason(&mcusr_mirror))
			{
				send_status_timeout--;
			
				if (send_status_timeout == 0)
				{
					send_status_timeout = SEND_STATUS_EVERY_SEC;
					send_time_currenttime_status();
					led_blink(200, 0, 1);
					
					version_status_cycle++;
				}
				else if (version_status_cycle >= SEND_VERSION_STATUS_CYCLE)
				{
					version_status_cycle = 0;
					send_deviceinfo_status();
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

		loop++;
	}
	
	// never called
	// aes256_done(&aes_ctx);
}
