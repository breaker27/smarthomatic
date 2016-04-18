/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2016 Uwe Freese
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
#include "lcd.h"

#include "../src_common/msggrp_generic.h"
#include "../src_common/msggrp_gpio.h"

#include "../src_common/e2p_hardware.h"
#include "../src_common/e2p_generic.h"
#include "../src_common/e2p_teamaker.h"

#include "onewire.h"

#include "../src_common/aes256.h"
#include "../src_common/util.h"
#include "version.h"

#include "../src_common/util_watchdog_init.h"

#define SWITCH_COUNT 6 // Don't change! (PC0 to PC5 are supported)

#define RELAIS_PORT PORTC // TODO: Configurable pins like in env sensor
#define RELAIS_PIN_START 0

#define BUTTON_DDR DDRB
#define BUTTON_PORT PORTB
#define BUTTON_PINPORT PINB
#define BUTTON_UP_PIN    1
#define BUTTON_DOWN_PIN  2
#define BUTTON_LEFT_PIN  3
#define BUTTON_RIGHT_PIN 4

#define BUTTON_DEBOUNCE_COUNT 3
#define BUTTON_DEBOUNCE_DELAY_MS 30

// Power of RFM12B (since PCB rev 1.1) or RFM12 NRES (Reset) pin may be connected to PC3.
// If not, only sw reset is used.
#define RFM_RESET_PIN 3
#define RFM_RESET_PORT_NR 1

#define SEND_STATUS_EVERY_SEC 1800 // how often should a status be sent?
#define SEND_VERSION_STATUS_CYCLE 50 // send version status x times less than switch status (~once per day)

// PWM range around 1500ms. E.g. use 300 to use a range of 1.2..1.8 ms pulse. Don't use values above 500!
#define PWM_RANGE 300

uint16_t device_id;
uint32_t station_packetcounter;
bool switch_state[SWITCH_COUNT];
uint16_t switch_timeout[SWITCH_COUNT];

uint8_t rom_id[8]; // for 1-wire

uint16_t send_status_timeout = 5;
uint8_t version_status_cycle = SEND_VERSION_STATUS_CYCLE - 1; // send promptly after startup

#define WINCH_PIN 1
#define WINCH_DDR DDRB

// These are the values from the current preset, read out of the E2P.
//char *   preset_name = "1234567890123456\x00";
char preset_name[17] = {0x73,0x63,0x68,0x77,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00};
uint16_t heating_temperature = 0;
uint8_t  heating_temperature_drop = 0;
uint16_t last_heating_time_sec = 0;
uint16_t brewing_temperature = 0;
uint16_t brewing_time_sec = 0;
uint16_t warming_temperature = 0;
uint16_t warming_time_sec = 0;

typedef enum
{
	MENU_CONFIRMATION,
	MENU_BREWING_PRESET,
	MENU_START_MODE_NOW,
	MENU_START_MODE_TIMER,
	MENU_START_MODE_COUNTDOWN,
	MENU_WAIT_START_TIME
} menu_pos_t;

typedef enum
{
	BUTTON_UP,
	BUTTON_DOWN,
	BUTTON_RIGHT,
	BUTTON_LEFT,
	BUTTON_NONE
} button_t;


void io_init(void)
{
	// init I/O pins for buttons as input with pull-up active
	cbi(BUTTON_DDR,  BUTTON_UP_PIN);
	sbi(BUTTON_PORT, BUTTON_UP_PIN);
	cbi(BUTTON_DDR,  BUTTON_DOWN_PIN);
	sbi(BUTTON_PORT, BUTTON_DOWN_PIN);
	cbi(BUTTON_DDR,  BUTTON_LEFT_PIN);
	sbi(BUTTON_PORT, BUTTON_LEFT_PIN);
	cbi(BUTTON_DDR,  BUTTON_RIGHT_PIN);
	sbi(BUTTON_PORT, BUTTON_RIGHT_PIN);
}

// Timer1 (16 Bit) is used for the winch.
// The servo drive needs a cycle of 20ms with a pulse of 1-2 ms, where 1,5ms means the
// servo is standing still.
void PWM_init(void)
{
	// Enable output pins
	WINCH_DDR |= (1 << WINCH_PIN);

	// OC1A: Fast PWM, 16 Bit, TOP = ICR1, non-inverting output
	TCCR1A = (1 << WGM11) | (1 << COM1A1);
	TCCR1B = (1 << WGM12) | (1 << WGM13) | (1 << CS10);

	// count from 0 - 20000 = 20ms @ 1 MHz
	ICR1 = 20000;
	OCR1A = 1500; // stand still
}

// Use speed percentage in the range -100..100.
void winch_speed(int8_t percentage)
{
	if (percentage > 100)
	{
		percentage = 100;
	}
	else if (percentage < -100)
	{
		percentage = -100;
	}

	OCR1A = 1500 + PWM_RANGE * (int16_t)percentage / 100;
}

// Accelerate from one speed to another one in the given amount of ms.
// The speed (PWM value) is adjusted every 20ms, which is also the PWM speed of the servo.
void winch_accelerate(int8_t startPercentage, int8_t endPercentage, uint16_t ms)
{
	int16_t steps = ms / 20;
	int16_t speed;
	
	/*
	UART_PUTF("startPercentage: %d\r\n", startPercentage);
	UART_PUTF("endPercentage: %d\r\n", endPercentage);
	UART_PUTF("ms: %d\r\n", ms);
	UART_PUTF("steps: %d\r\n", steps); */
	
	for (uint8_t i = 0; i < steps; i++)
	{
		speed = startPercentage + ((int16_t)endPercentage - startPercentage) * i / steps;
		winch_speed(speed);
		_delay_ms(20);
	}
	
	winch_speed(endPercentage);
	UART_PUTF("end: %d\r\n", endPercentage);
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

void send_gpio_digitalporttimeout_status(void)
{	
	uint8_t i;
	
	UART_PUTS("Sending GPIO DigitalPortTimeout Status:\r\n");

	print_switch_state();

	inc_packetcounter();

	// Set packet content
	pkg_header_init_gpio_digitalporttimeout_status();
	pkg_header_set_senderid(device_id);
	pkg_header_set_packetcounter(packetcounter);
	
	for (i = 0; i < SWITCH_COUNT; i++)
	{
		msg_gpio_digitalporttimeout_set_on(i, switch_state[i]);
		msg_gpio_digitalporttimeout_set_timeoutsec(i, switch_timeout[i]);
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

/*
void switchRelais(int8_t num, bool on, uint16_t timeout, bool dbgmsg)
{
	if (dbgmsg)
	{
		UART_PUTF3("Switching relais %u to %u with timeout %us.\r\n", num + 1, on, timeout);
	}

	if (num >= SWITCH_COUNT)
	{
		UART_PUTF("\r\nRelais number %u > SWITCH_COUNT, ignoring.", num);
		return;
	}

	if (switch_state[num] != on)
	{
		switch_state[num] = on;
		
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
	
	if (e2p_powerswitch_get_switchstate(num) != on)
	{
		e2p_powerswitch_set_switchstate(num, on);
	}
	
	if (switch_timeout[num] != timeout)
	{
		switch_timeout[num] = timeout;
	}

	if (e2p_powerswitch_get_switchtimeout(num) != timeout)
	{
		e2p_powerswitch_set_switchtimeout(num, timeout);
	}
}
*/

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
	
	e2p_teamaker_set_basestationpacketcounter(station_packetcounter);
	
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
	
	//process_request(messagetype, messagegroupid, messageid);
}

// Return currently pressed button, without using debouncing.
// If more than one is pressed, prefer them in the following order:
// UP, DOWN, LEFT, RIGHT
button_t get_button(void)
{
	uint8_t state = BUTTON_PINPORT;
	
	if (0 == (state & (1 << BUTTON_UP_PIN)))
	{
		return BUTTON_UP;
	}
	else if (0 == (state & (1 << BUTTON_DOWN_PIN)))
	{
		return BUTTON_DOWN;
	}
	else if (0 == (state & (1 << BUTTON_LEFT_PIN)))
	{
		return BUTTON_LEFT;
	}
	else if (0 == (state & (1 << BUTTON_RIGHT_PIN)))
	{
		return BUTTON_RIGHT;
	}
	else
	{
		return BUTTON_NONE;
	}
}

// wait until all buttons are released
void button_wait_up(void)
{
	uint8_t count = 0;
	
	while (count < BUTTON_DEBOUNCE_COUNT)
	{
		_delay_ms(BUTTON_DEBOUNCE_DELAY_MS);
		
		if (get_button() == BUTTON_NONE)
		{
			count++;
		}
		else
		{
			count = 0;
		}
	}
}

// wait for button press and return which button was pressed
button_t button_wait_down(void)
{
	uint8_t count = 0;
	button_t last;
	button_t current = BUTTON_NONE;
	
	while (count < BUTTON_DEBOUNCE_COUNT)
	{
		_delay_ms(BUTTON_DEBOUNCE_DELAY_MS);
		
		last = current;
		current = get_button();
		
		if ((last == current) && (current != BUTTON_NONE))
		{
			count++;
		}
		else
		{
			count = 0;
		}
	}
	
	return current;
}

button_t button(void)
{
	button_wait_up();
	return button_wait_down();
}

// Lead the given preset from E2P into variables.
void load_preset(uint8_t nr)
{
	// TODO: Load name (as char*).
	e2p_teamaker_get_presetname(nr, (void*)&preset_name);
	heating_temperature = e2p_teamaker_get_heatingtemperature(nr);
	heating_temperature_drop = e2p_teamaker_get_heatingtemperaturedrop(nr);
	last_heating_time_sec = e2p_teamaker_get_lastheatingtimesec(nr);
	brewing_temperature = e2p_teamaker_get_brewingtemperature(nr);
	brewing_time_sec = e2p_teamaker_get_brewingtimesec(nr);
	warming_temperature = e2p_teamaker_get_warmingtemperature(nr);
	warming_time_sec = e2p_teamaker_get_warmingtimesec(nr);
}

int main(void)
{
	uint8_t loop = 0;
	uint16_t temperature;
	uint8_t i;
	uint8_t j;

	// delay 1s to avoid further communication with uart or RFM12 when my programmer resets the MC after 500ms...
	_delay_ms(700);

	util_init();
	/*
	//check_eeprom_compatibility(DEVICETYPE_POWERSWITCH);

	// read packetcounter, increase by cycle and write back
	packetcounter = e2p_generic_get_packetcounter() + PACKET_COUNTER_WRITE_CYCLE;
	e2p_generic_set_packetcounter(packetcounter);

	// read last received station packetcounter
	station_packetcounter = e2p_powerswitch_get_basestationpacketcounter();
	
	// read device id
	device_id = e2p_generic_get_deviceid();

	osccal_init();
*/
	uart_init();

	UART_PUTS ("\r\n");
	UART_PUTF4("smarthomatic Tea Maker v%u.%u.%u (%08lx)\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_HASH);
	UART_PUTS("(c) 2016 Uwe Freese, www.smarthomatic.org\r\n");
	osccal_info();
	UART_PUTF ("DeviceID: %u\r\n", device_id);
	UART_PUTF ("PacketCounter: %lu\r\n", packetcounter);
	print_switch_state();
	UART_PUTF ("Last received base station PacketCounter: %u\r\n\r\n", station_packetcounter);
	
	lcd_init();
	
	/* // show all characters
	i = 0;
	char xx[2] = {0x73,0x00};
	
	while (1)
	{
		lcd_gotoxy(0, 0);
		LCD_PUTF("i = %d ", i);

		lcd_gotoxy(0, 1);
		
		for (j = 0; j < 16; j++)
		{
			xx[0] = i * 16 + j;
			
			LCD_PUTF("%s", xx);
			i++;
		}
		
		_delay_ms(10000);
		
		i = (i + 1) % 16;
	}
	*/
	
	LCD_PUTS("--- TeaMaker ---");
	lcd_gotoxy(0, 1);
	LCD_PUTF4("%u.%u.%u %08lx", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_HASH);

	// init AES key
	e2p_generic_get_aeskey(aes_key);
	
	//PWM_init();
	
	io_init();
	
	onewire_init();
	
	bool res = onewire_get_rom_id(rom_id);
		
	if (res) // error, no slave found
	{
		while (1)
		{
			led_blink(50, 50, 1);
		}
	}
	
	UART_PUTS("1-wire ROM ID: ");
	print_bytearray(rom_id, 8);

	/* // button and one wire temperature sensor check
	while(1)
	{
		lcd_gotoxy(0, 0);
		LCD_PUTF2("%u %u", 0b00011110 & BUTTON_PINPORT, get_button());
		lcd_gotoxy(0, 1);
		temperature = onewire_get_temperature(rom_id);
		LCD_PUTF2("Temp: %u.%02u degC", temperature / 100, temperature % 100);
		_delay_ms(100);
	} */
	
	menu_pos_t menu_pos = MENU_CONFIRMATION;
	uint8_t brewing_preset_pos = 0;
	uint8_t brewing_preset_count = 5;
	uint8_t timer_pos = 0;
	uint8_t timer_count = 5;
	uint8_t countdown_pos = 0;
	uint8_t countdown_count = 5;
	
	uint8_t brewing_regulation_range_above = 0;
	uint8_t brewing_regulation_range_below = 0;
	uint8_t warming_regulation_range_above = 0;
	uint8_t warming_regulation_range_below = 0;
	uint8_t brewing_PWM_percentage = 0;
	uint8_t brewing_PWM_cycle_cec = 0;
	uint8_t warming_PWM_percentage = 0;
	uint8_t warming_PWM_cycle_cec = 0;
	
	load_preset(0);
	
	
	while (1)
	{
		switch (menu_pos)
		{
			case MENU_CONFIRMATION:
				lcd_clear();
				LCD_PUTS(" Wasser und Tee ");
				lcd_gotoxy(0,1);
				LCD_PUTS("   einfüllen!   ");
				
				switch (button())
				{
					case BUTTON_RIGHT:
						menu_pos = MENU_BREWING_PRESET;
						break;
					default:
						break;
				}

				break;

			case MENU_BREWING_PRESET: 
				lcd_clear();
				LCD_PUTF("%s", preset_name); // user perceived number counts from 1 on
				lcd_gotoxy(0,1);
				
				LCD_PUTF3("%d:%02d   %d°C", brewing_time_sec / 60, brewing_time_sec % 60, brewing_temperature / 10);
				
				switch (button())
				{
					case BUTTON_UP:
						brewing_preset_pos = (brewing_preset_pos + brewing_preset_count - 1) % brewing_preset_count;
						load_preset(brewing_preset_pos);
						break;
					case BUTTON_DOWN:
						brewing_preset_pos = (brewing_preset_pos + 1) % brewing_preset_count;
						load_preset(brewing_preset_pos);
						break;
					case BUTTON_RIGHT:
						menu_pos = MENU_START_MODE_NOW;
						break;
					case BUTTON_LEFT:
						menu_pos = MENU_CONFIRMATION;
						break;
					default:
						break;
				}
				
				break;
			
			case MENU_START_MODE_NOW:
				lcd_clear();
				LCD_PUTS("Start now");
				
				switch (button())
				{
					case BUTTON_UP:
						menu_pos = MENU_START_MODE_TIMER;
						break;
					case BUTTON_DOWN:
						menu_pos = MENU_START_MODE_COUNTDOWN;
						break;
					case BUTTON_RIGHT:
						menu_pos = MENU_WAIT_START_TIME;
						break;
					case BUTTON_LEFT:
						menu_pos = MENU_BREWING_PRESET;
						break;
					default:
						break;
				}
				
				break;
				
			case MENU_START_MODE_TIMER:
				lcd_clear();
				LCD_PUTF("Timer %d", timer_pos);
				
				switch (button())
				{
					case BUTTON_UP:
						break;
					case BUTTON_DOWN:
						break;
					case BUTTON_RIGHT:
						break;
					case BUTTON_LEFT:
						menu_pos = MENU_BREWING_PRESET;
						break;
					default:
						break;
				}

				break;
				
			case MENU_START_MODE_COUNTDOWN:
				lcd_clear();
				LCD_PUTF("Countdown %d", countdown_pos);
				
				switch (button())
				{
					case BUTTON_UP:
						break;
					case BUTTON_DOWN:
						break;
					case BUTTON_RIGHT:
						break;
					case BUTTON_LEFT:
						menu_pos = MENU_BREWING_PRESET;
						break;
					default:
						break;
				}

				break;
				
			case MENU_WAIT_START_TIME:
				lcd_clear();
				LCD_PUTS("Wait for stime");
				_delay_ms(5000);
				menu_pos = MENU_CONFIRMATION;
				
				break;
		}
	}
	
	
	
	/* // winch test
	while (42)
	{
		winch_accelerate(0, 20, 500);
		winch_accelerate(20, 100, 500);
		_delay_ms(500);
		winch_accelerate(100, 15, 1000);
		_delay_ms(500);
		winch_accelerate(15, 0, 500);
		_delay_ms(3000);
	} */
	

//	rfm_watchdog_init(device_id, e2p_powerswitch_get_transceiverwatchdogtimeout(), RFM_RESET_PORT_NR, RFM_RESET_PIN);
//	rfm12_init();

//	sei();

}
