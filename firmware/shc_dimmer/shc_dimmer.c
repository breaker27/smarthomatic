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
#include "../src_common/uart.h"

#include "../src_common/msggrp_generic.h"
#include "../src_common/msggrp_dimmer.h"

#include "../src_common/e2p_hardware.h"
#include "../src_common/e2p_generic.h"
#include "../src_common/e2p_dimmer.h"

#include "../src_common/aes256.h"
#include "../src_common/util.h"
#include "version.h"

#include "../src_common/util_watchdog_init.h"

//#define UART_DEBUG_CALCULATIONS

#define SEND_STATUS_EVERY_SEC 1800 // how often should a status be sent
#define SEND_VERSION_STATUS_CYCLE 50 // send version status x times less than switch status (~once per day)

#define DIMMER_DDR DDRB
#define DIMMER_PORT PORTB
#define DIMMER_PIN 1 // AKA "OC1A"
#define RELAIS_DDR DDRB
#define RELAIS_PORT PORTB
#define RELAIS_PIN 2

// TODO: Create macros so that each output is only defined as e.g. "PD3" and the other strings are generated ba macros.
#define BUTTON_DDR DDRD
#define BUTTON_PORTPORT PORTD
#define BUTTON_PORT PIND
#define BUTTON_PIN 3

// Power of RFM12B (since PCB rev 1.3) or RFM12 NRES (Reset) pin may be connected to PC3.
// If not, only sw reset is used.
#define RFM_RESET_PIN 3
#define RFM_RESET_PORT_NR 1

// These are the PWM values for 0%..100% (AKA 1V..10V output),
// calculated by measuring the output voltage and linear interpolation
// of the measured voltages.
static uint16_t pwm_lookup [] = { 119, 199, 265, 325, 376, 421, 462, 498, 528, 559, 584, 607, 629, 650,
	668, 683, 700, 714, 727, 740, 752, 763, 773, 783, 792, 801, 809, 817, 824, 831, 837, 844, 850, 856,
	861, 866, 871, 876, 881, 885, 890, 894, 898, 902, 905, 909, 912, 916, 919, 922, 925, 928, 931, 933,
	936, 938, 941, 943, 946, 948, 950, 952, 954, 956, 958, 960, 962, 964, 966, 967, 969, 971, 972, 974,
	975, 977, 978, 980, 981, 982, 984, 985, 986, 988, 989, 990, 991, 992, 993, 995, 996, 997, 998, 999,
	1000, 1001, 1002, 1003, 1004, 1004, 1005 };

static uint8_t brightness_translation[101];

#define ANIMATION_CYCLE_MS 32.768   // make one animation step every 32,768 ms, triggered by timer 0
#define ANIMATION_UPDATE_MS 200     // update the brightness every ANIMATION_UPDATE_MS ms, done by the main loop

#define SWITCH_OFF_TIMEOUT_MS 3000  // If 0% brightness is reached, switch off power (relais) with a delay to
                                    // 1) dim down before switching off and to
                                    // 2) avoid switching power off at manual dimming.

// variables used for the animation
uint8_t animation_mode;
uint32_t animation_length = 0;   // if > 0, an animation is running
uint32_t animation_position = 0; // current position in the animation, from 0 to animation_length
uint8_t start_brightness = 0;
uint8_t end_brightness = 0;
float current_brightness = 0;

uint16_t device_id;
uint8_t use_pwm_translation = 1;
uint32_t station_packetcounter;
uint8_t switch_off_counter = 0;
uint8_t version_status_cycle = SEND_VERSION_STATUS_CYCLE - 1; // send promptly after startup

void switchRelais(uint8_t on)
{
	if (on)
	{
		sbi(RELAIS_PORT, RELAIS_PIN);
	}
	else
	{
		cbi(RELAIS_PORT, RELAIS_PIN);
	}
}

// Read for more information about PWM:
// http://www.protostack.com/blog/2011/06/atmega168a-pulse-width-modulation-pwm/
// http://extremeelectronics.co.in/avr-tutorials/pwm-signal-generation-by-using-avr-timers-part-ii/
void PWM_init(void)
{
	// Enable output pins
	DDRB |= (1 << RELAIS_PIN) | (1 << DIMMER_PIN);

	// Fast PWM, 10 Bit, TOP = 0x03FF = 1023, Inverting output at OC1A
	TCCR1A = (1 << WGM11) | (1 << WGM10) | (1 << COM1A1) | (1 << COM1A0);

	// The output voltages are a very little different with a prescaler of 64 and very different with a prescaler of 1024.
	// Also the voltage fluctuates much with a prescaler of 1024 (PWM too slow).
	// Leave it at prescaler 1/8, this seems to produce a good result.
	TCCR1B = (1 << CS11); // Clock source = I/O clock, 1/8 prescaler
}

// Timer0 is used for accurate counting of the animation time.
void timer0_init(void)
{
	// Clock source = I/O clock, 1/1024 prescaler
	TCCR0B = (1 << CS02) | (1 << CS00);

	// Timer/Counter0 Overflow Interrupt Enable
	TIMSK0 = (1 << TOIE0);
}

void io_init(void)
{
	// set button pin to input
	cbi(BUTTON_DDR, BUTTON_PIN);

	// switch on pullup for button input pin
	sbi(BUTTON_PORTPORT, BUTTON_PIN);
}

void checkSwitchOff(void)
{
	if (current_brightness == 0)
	{
		if (switch_off_counter < (SWITCH_OFF_TIMEOUT_MS / ANIMATION_UPDATE_MS))
		{
			switch_off_counter++;
		}
		else
		{
			switchRelais(0);
		}
	}
	else
	{
		switch_off_counter = 0;
		switchRelais(1);
		
		// Switching on relais (and lamp) leads to interferences.
		// Avoid sending with RFM12 directly afterwards by making a short delay.
		_delay_ms(250);
	}
}

void setPWMDutyCyclePercent(float percent)
{
	uint8_t index, index2;
	float modulo;

	if (percent > 100)
	{
		percent = 100;
	}

	current_brightness = percent; // used for status packet

#ifdef UART_DEBUG_CALCULATIONS
	UART_PUTF2 ("Percent requested: %d.%02d\r\n", (uint16_t)percent, (uint16_t)(percent * 100) % 100);
#endif
	
	// My OSRAM CFL lamp does not react to the 1..10V input in a linear way.
	// So translate promille to the translated promille value first (if enabled).
	if (use_pwm_translation)
	{
		index = (uint8_t)percent;
		index2 = index >= 100 ? 100 : index + 1;
		modulo = percent - index; // decimal places only, e.g. 0.12 when percent is 73.12
		
		uint8_t t1 = brightness_translation[index];
		uint8_t t2 = brightness_translation[index2];
		
		percent = linear_interpolate_f(modulo, 0.0, 1.0, t1, t2) * 100 / 255;
		
#ifdef UART_DEBUG_CALCULATIONS	
		UART_PUTF ("  Index 1: %u\r\n", index);
		UART_PUTF ("  Index 2: %u\r\n", index2);
		UART_PUTF ("  Translation value 1: %u\r\n", t1);
		UART_PUTF ("  Translation value 2: %u\r\n", t2);
		UART_PUTF ("  Modulo: 0.%003u\r\n", (uint16_t)(modulo * 1000));
		UART_PUTF2 ("  Percent corrected: %d.%02d\r\n", (uint16_t)percent, (uint16_t)(percent * 100) % 100);
#endif
	}
	
	// convert percentage to corrected percentage
	index = (uint8_t)percent;
	index2 = index >= 100 ? 100 : index + 1;
	modulo = percent - index; // decimal places only, e.g. 0.12 when percent is 73.12

	uint32_t pwm = (uint32_t)linear_interpolate_f(modulo, 0.0, 1.0, pwm_lookup[index], pwm_lookup[index2]);

#ifdef UART_DEBUG_CALCULATIONS	
	UART_PUTF ("    PWM value 1: %u\r\n", pwm_lookup[index]);
	UART_PUTF ("    PWM value 2: %u\r\n", pwm_lookup[index2]);
	UART_PUTF ("    Modulo: 0.%003u\r\n", (uint16_t)(modulo * 1000));
	UART_PUTF ("    PWM value interpolated: %u\r\n", pwm);
#else
	UART_PUTF ("PWM=%u\r\n", pwm);
#endif

	OCR1A = pwm;
}

// Count up the animation_position every 1/8000000 * 1024 * 256 ms = 32,768 ms,
// if animation is running.
ISR (TIMER0_OVF_vect)
{
	if (animation_position < animation_length)
	{
		animation_position++;
	}
}

void send_dimmer_status(void)
{
	UART_PUTS("Sending Dimmer Status:\r\n");

	inc_packetcounter();
	uint8_t bri = (uint8_t)current_brightness;
	
	// Set packet content
	pkg_header_init_dimmer_brightness_status();
	pkg_header_set_senderid(device_id);
	pkg_header_set_packetcounter(packetcounter);
	msg_dimmer_brightness_set_brightness(bri);
	
	UART_PUTF("CRC32 is %lx (added as first 4 bytes)\r\n", getBuf32(0));
	UART_PUTF("Brightness: %u%%\r\n", bri);

	rfm12_send_bufx();
}

void send_deviceinfo_status(void)
{
	inc_packetcounter();

	UART_PUTF("Send DeviceInfo: DeviceType %u,", DEVICETYPE_DIMMER);
	UART_PUTF4(" v%u.%u.%u (%08lx)\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_HASH);
	
	// Set packet content
	pkg_header_init_generic_deviceinfo_status();
	pkg_header_set_senderid(device_id);
	pkg_header_set_packetcounter(packetcounter);
	msg_generic_deviceinfo_set_devicetype(DEVICETYPE_DIMMER);
	msg_generic_deviceinfo_set_versionmajor(VERSION_MAJOR);
	msg_generic_deviceinfo_set_versionminor(VERSION_MINOR);
	msg_generic_deviceinfo_set_versionpatch(VERSION_PATCH);
	msg_generic_deviceinfo_set_versionhash(VERSION_HASH);

	rfm12_send_bufx();
}

void send_ack(uint32_t acksenderid, uint32_t ackpacketcounter, bool error)
{
	// any message can be used as ack, because they are the same anyway
	if (error)
	{
		UART_PUTS("Send error Ack\r\n");
		pkg_header_init_dimmer_animation_ack();
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

// process request "brightness"
void process_brightness(MessageTypeEnum messagetype)
{
	// "Set" or "SetGet" -> modify dimmer state and abort any running animation
	if ((messagetype == MESSAGETYPE_SET) || (messagetype == MESSAGETYPE_SETGET))
	{
		start_brightness = end_brightness = msg_dimmer_brightness_get_brightness();

		UART_PUTF("Requested Brightness: %u%%;", start_brightness);
		
		animation_length = 0;
		animation_mode = 0;
		animation_position = 0;

		setPWMDutyCyclePercent(start_brightness);
						
		/* TODO: Write to EEPROM (?)
		// write back switch state to EEPROM
		switch_state[i] = req_state;
		switch_timeout[i] = req_timeout;
		
		eeprom_write_word((uint16_t*)EEPROM_POS_SWITCH_STATE + i * 2, u16);
		*/
	}

	// remember some values before the packet buffer is destroyed
	uint32_t acksenderid = pkg_header_get_senderid();
	uint32_t ackpacketcounter = pkg_header_get_packetcounter();

	// "Set" -> send "Ack"
	if (messagetype == MESSAGETYPE_SET)
	{
		pkg_header_init_dimmer_brightness_ack();

		UART_PUTS("Sending Ack\r\n");
	}
	// "Get" or "SetGet" -> send "AckStatus"
	else
	{
		pkg_header_init_dimmer_brightness_ackstatus();
		
		// set message data
		msg_dimmer_brightness_set_brightness(start_brightness);

		UART_PUTS("Sending AckStatus\r\n");
	}

	send_ack(acksenderid, ackpacketcounter, false);
}

// process request "animation"
void process_animation(MessageTypeEnum messagetype)
{
	// "Set" or "SetGet" -> modify dimmer state and start new animation
	if ((messagetype == MESSAGETYPE_SET) || (messagetype == MESSAGETYPE_SETGET))
	{
		animation_mode = msg_dimmer_animation_get_animationmode();
		animation_length = msg_dimmer_animation_get_timeoutsec();
		start_brightness = msg_dimmer_animation_get_startbrightness();
		end_brightness = msg_dimmer_animation_get_endbrightness();

		UART_PUTF("   Animation Mode: %u\r\n", animation_mode);
		UART_PUTF("   Animation Time: %us\r\n", animation_length);
		UART_PUTF(" Start Brightness: %u%%\r\n", start_brightness);
		UART_PUTF("   End Brightness: %u%%\r\n", end_brightness);
		
		animation_length = (uint32_t)((float)animation_length * 1000 / ANIMATION_CYCLE_MS);
		animation_position = 0;
		
		setPWMDutyCyclePercent(start_brightness);
		
		/* TODO: Write to EEPROM (?)
		// write back switch state to EEPROM
		switch_state[i] = req_state;
		switch_timeout[i] = req_timeout;
		
		eeprom_write_word((uint16_t*)EEPROM_POS_SWITCH_STATE + i * 2, u16);
		*/
	}

	// remember some values before the packet buffer is destroyed
	uint32_t acksenderid = pkg_header_get_senderid();
	uint32_t ackpacketcounter = pkg_header_get_packetcounter();

	// "Set" -> send "Ack"
	if (messagetype == MESSAGETYPE_SET)
	{
		pkg_header_init_dimmer_animation_ack();

		UART_PUTS("Sending Ack\r\n");
	}
	// "Get" or "SetGet" -> send "AckStatus"
	else
	{
		pkg_header_init_dimmer_animation_ackstatus();
		
		// set message data
		msg_dimmer_animation_set_animationmode(animation_mode);
		msg_dimmer_animation_set_timeoutsec(animation_length);
		msg_dimmer_animation_set_startbrightness(start_brightness);
		msg_dimmer_animation_set_endbrightness(end_brightness);

		UART_PUTS("Sending AckStatus\r\n");
	}

	send_ack(acksenderid, ackpacketcounter, false);
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
	
	e2p_dimmer_set_basestationpacketcounter(station_packetcounter);
	
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
	
	// remember some values before the packet buffer is destroyed
	uint32_t acksenderid = pkg_header_get_senderid();
	uint32_t ackpacketcounter = pkg_header_get_packetcounter();
	
	// check MessageGroup + MessageID
	uint32_t messagegroupid = pkg_headerext_common_get_messagegroupid();
	uint32_t messageid = pkg_headerext_common_get_messageid();
	
	UART_PUTF("MessageGroupID:%u;", messagegroupid);
	
	if (messagegroupid != MESSAGEGROUP_DIMMER)
	{
		UART_PUTS("\r\nERR: Unsupported MessageGroupID.\r\n");
		send_ack(acksenderid, ackpacketcounter, true);
		return;
	}
	
	UART_PUTF("MessageID:%u;\r\n", messageid);

	switch (messageid)
	{
		case MESSAGEID_DIMMER_BRIGHTNESS:
			process_brightness(messagetype);
			break;
		case MESSAGEID_DIMMER_ANIMATION:
			process_animation(messagetype);
			break;
		default:
			UART_PUTS("ERR: Unsupported MessageID.\r\n");
			send_ack(acksenderid, ackpacketcounter, true);
			break;
	}
	
	UART_PUTS("\r\n");
}

int main(void)
{
	uint16_t send_status_timeout = 25;
	uint32_t pos;
	uint8_t button_state = 0;
	uint8_t manual_dim_direction = 0;

	// delay 1s to avoid further communication with uart or RFM12 when my programmer resets the MC after 500ms...
	_delay_ms(1000);

	util_init();
	
	check_eeprom_compatibility(DEVICETYPE_DIMMER);
	
	// read packetcounter, increase by cycle and write back
	packetcounter = e2p_generic_get_packetcounter() + PACKET_COUNTER_WRITE_CYCLE;
	e2p_generic_set_packetcounter(packetcounter);

	// read device id
	device_id = e2p_generic_get_deviceid();

	// pwm translation table is not used if first byte is 0xFF
	e2p_dimmer_get_brightnesstranslationtable(brightness_translation);
	use_pwm_translation = (0xFF != brightness_translation[0]);
	
	// TODO: read (saved) dimmer state from before the eventual powerloss
	/*for (i = 0; i < SWITCH_COUNT; i++)
	{
		uint16_t u16 = eeprom_read_word((uint16_t*)EEPROM_POS_SWITCH_STATE + i * 2);
		switch_state[i] = (uint8_t)(u16 & 0b1);
		switch_timeout[i] = u16 >> 1;
	}*/
	
	// read last received station packetcounter
	station_packetcounter = e2p_dimmer_get_basestationpacketcounter();
	
	led_blink(500, 500, 3);

	osccal_init();

	uart_init();
	UART_PUTS ("\r\n");
	UART_PUTF4("smarthomatic Dimmer v%u.%u.%u (%08lx)\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_HASH);
	UART_PUTS("(c) 2013..2015 Uwe Freese, www.smarthomatic.org\r\n");
	osccal_info();
	UART_PUTF ("DeviceID: %u\r\n", device_id);
	UART_PUTF ("PacketCounter: %lu\r\n", packetcounter);
	UART_PUTF ("Use PWM translation table: %u\r\n", use_pwm_translation);
	UART_PUTF ("Last received base station PacketCounter: %u\r\n\r\n", station_packetcounter);

	// init AES key
	e2p_generic_get_aeskey(aes_key);
	
	rfm_watchdog_init(device_id, e2p_dimmer_get_transceiverwatchdogtimeout(), RFM_RESET_PORT_NR, RFM_RESET_PIN);
	rfm12_init();

	PWM_init();
	io_init();
	setPWMDutyCyclePercent(0);
	timer0_init();

	// DEMO to measure the voltages of different PWM settings to calculate the pwm_lookup table
	/*while (42)
	{
		uint16_t i;
		
		for (i = 800; i <= 1024; i = i + 10)
		{
			UART_PUTF ("PWM value OCR1A: %u\r\n", i);
			OCR1A = i;
			led_blink(500, 6500, 1);
		}
	}*/
	
	// DEMO 0..100..0%, using the pwm_lookup table and the translation table in EEPROM.
	/*while (42)
	{
		float i;
		
		for (i = 0; i <= 100; i = i + 0.05)
		{
			led_blink(10, 10, 1);
			setPWMDutyCycle(i);
		}
		
		for (i = 99.95; i > 0; i = i - 0.05)
		{
			led_blink(10, 10, 1);
			setPWMDutyCycle(i);
		}
	}*/

	// set initial switch state
	/*for (i = 0; i < SWITCH_COUNT; i++)
	{
		switchRelais(i, switch_state[i]);
	}*/

	sei();

	// DEMO 30s
	/*animation_length = 30;
	animation_length = (uint32_t)((float)animation_length * 1000 / ANIMATION_CYCLE_MS);
	start_brightness = 0;
	end_brightness = 255;
	animation_position = 0;*/
	
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

		_delay_ms(ANIMATION_UPDATE_MS);
		
		// React on button press.
		// - abort animation
		// - switch off, when brightness > 0
		// - switch on otherwise
		if (!(BUTTON_PORT & (1 << BUTTON_PIN))) // button press
		{
			if (button_state == 0)
			{
				UART_PUTS("Button pressed\r\n");
				animation_length = 0;
				animation_position = 0;
			}
			
			if (button_state < 5)
			{
				button_state++;
			}
			else // manual dimming
			{
				if (manual_dim_direction) // UP
				{
					if (current_brightness < 100)
					{
						current_brightness = (uint8_t)current_brightness / 2 * 2 + 2;
						setPWMDutyCyclePercent(current_brightness);
					}
					else
					{
						UART_PUTS("manual dimming DOWN\r\n");
						manual_dim_direction = 0;
					}
				}
				else // DOWN
				{
					if (current_brightness > 0)
					{
						current_brightness = (((uint8_t)current_brightness - 1) / 2) * 2;
						setPWMDutyCyclePercent(current_brightness);
					}
					else
					{
						UART_PUTS("manual dimming UP\r\n");
						manual_dim_direction = 1;
					}
				}
				
			}
		}
		else if (button_state && (BUTTON_PORT & (1 << BUTTON_PIN))) // button release
		{
			UART_PUTS("Button released\r\n");
			
			if (button_state < 5) // short button press
			{
				if (current_brightness > 0)
				{
					UART_PUTS(" -> 0%\r\n");
					setPWMDutyCyclePercent(0);
				}
				else
				{
					UART_PUTS(" -> 100%\r\n");
					setPWMDutyCyclePercent(100);
				}
			}
			else
			{
				// reverse dim direction
				manual_dim_direction = !manual_dim_direction;
			}
			
			send_status_timeout = 10;
			button_state = 0;
		}
				
		// update brightness according animation_position, updated by timer0
		if (animation_length > 0)
		{
			pos = animation_position; // copy value to avoid that it changes in between by timer interrupt
			UART_PUTF2("%lu/%lu, ", pos, animation_length);
			
			if (pos == animation_length)
			{
				UART_PUTF("END Brightness %u%%, ", end_brightness);
				setPWMDutyCyclePercent((float)end_brightness);
				send_status_timeout = 10;
				animation_length = 0;
				animation_position = 0;
			}
			else
			{			
				float brightness = (start_brightness + ((float)end_brightness - start_brightness) * pos / animation_length);
				UART_PUTF("Br.%u%%, ", (uint32_t)(brightness));
				setPWMDutyCyclePercent(brightness);
			}
		}			
		
		// send status from time to time
		if (!send_startup_reason(&mcusr_mirror))
		{
			if (send_status_timeout == 0)
			{
				send_status_timeout = SEND_STATUS_EVERY_SEC * (1000 / ANIMATION_UPDATE_MS);
				send_dimmer_status();
				led_blink(200, 0, 1);
			}
			else if (version_status_cycle >= SEND_VERSION_STATUS_CYCLE)
			{
				version_status_cycle = 0;
				send_deviceinfo_status();
				led_blink(200, 0, 1);
			}
		}

		checkSwitchOff();

		rfm_watchdog_count(ANIMATION_UPDATE_MS);

		rfm12_tick();
		send_status_timeout--;
	}
	
	// never called
	// aes256_done(&aes_ctx);
}
