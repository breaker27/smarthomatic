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
//#define UART_DEBUG_CALCULATIONS

#define EEPROM_POS_DEVICE_ID 8
#define EEPROM_POS_PACKET_COUNTER 9
#define EEPROM_POS_STATION_PACKET_COUNTER 13
#define EEPROM_POS_DIMMER_STATE 17 // 2 Byte
#define EEPROM_POS_USE_PWM_TRANSLATION 19
#define EEPROM_POS_AES_KEY 32
#define EEPROM_POS_DIM_CORRECTION 64 // 101 values for 0..100%

// How often should the packetcounter_base be increased and written to EEPROM?
// This should be 2^32 (which is the maximum transmitted packet counter) /
// 100.000 (which is the maximum amount of possible EEPROM write cycles) or more.
// Therefore 100 is a good value.
#define PACKET_COUNTER_WRITE_CYCLE 100

#define SEND_STATUS_EVERY_SEC 600 // how often should a status be sent

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

// These are the PWM values for 0%..100% (AKA 1V..10V output),
// calculated by measuring the output voltage and linear interpolation
// of the measured voltages.
static uint16_t pwm_lookup [] = { 40, 130, 205, 270, 325, 375, 417, 455, 490, 520, 550, 575, 598, 620, 640, 658,
	674, 690, 704, 717, 730, 743, 753, 763, 773, 783, 792, 800, 808, 816, 823, 830, 836, 843, 849, 854,
	860, 865, 870, 875, 879, 884, 888, 892, 896, 900, 904, 907, 911, 914, 917, 920, 923, 926, 929, 932,
	935, 937, 940, 942, 944, 947, 949, 951, 953, 955, 957, 959, 961, 963, 965, 967, 968, 970, 972, 973,
	975, 976, 978, 979, 980, 982, 983, 984, 986, 987, 988, 989, 990, 991, 992, 994, 995, 996, 997, 998,
	999, 1000, 1001, 1002, 1003 };

#define ANIMATION_CYCLE_MS 32.768 // make one animation step every 32,768 ms, triggered by timer 0
#define ANIMATION_UPDATE_MS 200   // update the brightness every ANIMATION_UPDATE_MS ms, done by the main loop

// variables used for the animation
uint8_t animation_mode;
uint32_t animation_length = 0;   // if > 0, an animation is running
uint32_t animation_position = 0; // current position in the animation, from 0 to animation_length
uint8_t start_brightness = 0;
uint8_t end_brightness = 0;
float current_brightness = 0;

uint8_t device_id;
uint8_t use_pwm_translation = 1;
uint32_t packetcounter;
uint8_t switch_off_delay = 0; // If 0% brightness is reached, switch off power (relais) with a delay to 1) dim down before switching off and to 2) avoid switching power off at manual dimming.

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

void send_dimmer_status(void)
{
	UART_PUTS("Sending Dimmer Status:\r\n");

	// set device ID
	bufx[0] = device_id;
	
	// update packet counter
	packetcounter++;
	
	if (packetcounter % PACKET_COUNTER_WRITE_CYCLE == 0)
	{
		eeprom_write_dword((uint32_t*)0, packetcounter);
	}

	setBuf32(1, packetcounter);

	// set command ID "Dimmer Status"
	bufx[5] = 30; // TODO: Move command IDs to global definition file as defines

	// set current brightness
	bufx[6] = (uint8_t)(current_brightness * 255 / 100);
	
	// set end brightness
	bufx[7] = end_brightness;
	
	// set total animation time
	setBuf16(8, (uint16_t)(animation_length * ANIMATION_CYCLE_MS / 1000));

	// set time until animation finishes
	setBuf16(10, (uint16_t)((animation_length - animation_position) * ANIMATION_CYCLE_MS / 1000));

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
		if (switch_off_delay < 8)
		{
			switch_off_delay++;
		}
		else
		{
			switchRelais(0);
		}
	}
	else
	{
		switch_off_delay = 0;
		switchRelais(1);
	}
}

void setPWMDutyCycle(float percent)
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
#else
	//UART_PUTF2 ("Percent=%d.%02d\r\n", (uint16_t)percent, (uint16_t)(percent * 100) % 100);
#endif
	
	// My OSRAM CFL lamp does not react to the 1..10V input in a linear way.
	// So translate promille to the translated promille value first (if enabled).
	if (use_pwm_translation)
	{
		index = (uint8_t)percent;
		index2 = index >= 100 ? 100 : index + 1;
		modulo = percent - index; // decimal places only, e.g. 0.12 when percent is 73.12
		
		uint8_t t1 = eeprom_read_byte((uint8_t*)(EEPROM_POS_DIM_CORRECTION + index));
		uint8_t t2 = eeprom_read_byte((uint8_t*)(EEPROM_POS_DIM_CORRECTION + index2));
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

int main(void)
{
	uint16_t send_status_timeout = 25;
	uint32_t station_packetcounter;
	uint32_t pos;
	uint8_t button_state = 0;
	uint8_t manual_dim_direction = 0;

	// delay 1s to avoid further communication with uart or RFM12 when my programmer resets the MC after 500ms...
	_delay_ms(1000);

	util_init();
	check_eeprom_compatibility(DEVICETYPE_DIMMER);

	osccal_init();
	
	// read packetcounter, increase by cycle and write back
	packetcounter = eeprom_read_dword((uint32_t*)EEPROM_POS_PACKET_COUNTER) + PACKET_COUNTER_WRITE_CYCLE;
	eeprom_write_dword((uint32_t*)EEPROM_POS_PACKET_COUNTER, packetcounter);

	// read device id and write to send buffer
	device_id = eeprom_read_byte((uint8_t*)EEPROM_POS_DEVICE_ID);	
	use_pwm_translation = 1; //eeprom_read_byte((uint8_t*)EEPROM_POS_USE_PWM_TRANSLATION);	
	
	// TODO: read (saved) dimmer state from before the eventual powerloss
	/*for (i = 0; i < SWITCH_COUNT; i++)
	{
		uint16_t u16 = eeprom_read_word((uint16_t*)EEPROM_POS_SWITCH_STATE + i * 2);
		switch_state[i] = (uint8_t)(u16 & 0b1);
		switch_timeout[i] = u16 >> 1;
	}*/
	
	// read last received station packetcounter
	station_packetcounter = eeprom_read_dword((uint32_t*)EEPROM_POS_STATION_PACKET_COUNTER);

	led_blink(200, 200, 5);

#ifdef UART_DEBUG
	uart_init(false);
	UART_PUTS ("\r\n");
	UART_PUTS ("smarthomatic Dimmer V1.0 (c) 2013 Uwe Freese, www.smarthomatic.org\r\n");
	osccal_info();
	UART_PUTF ("Device ID: %u\r\n", device_id);
	UART_PUTF ("Packet counter: %lu\r\n", packetcounter);
	UART_PUTF ("Use PWM translation table: %u\r\n", use_pwm_translation);
	UART_PUTF ("Last received station packet counter: %u\r\n\r\n", station_packetcounter);
#endif

	// init AES key
	eeprom_read_block (aes_key, (uint8_t *)EEPROM_POS_AES_KEY, 32);

	rfm12_init();
	PWM_init();
	io_init();
	setPWMDutyCycle(0);
	timer0_init();

	// DEMO to measure the voltages of different PWM settings to calculate the pwm_lookup table
	/*while (42)
	{
		uint16_t i;
		
		for (i = 0; i <= 1024; i = i + 100)
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
							eeprom_write_dword((uint32_t*)EEPROM_POS_STATION_PACKET_COUNTER, station_packetcounter);
							
							// check command ID
							uint8_t cmd = bufx[5];

							UART_PUTF("       Command ID: %u\r\n", cmd);
							
							if (cmd != 141) // ID 141 == Dimmer Request
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
									// read animation mode and parameters
									uint8_t animation_mode = bufx[7] >> 5;
									
									// TODO: Implement support for multiple dimmers (e.g. RGB)
									// uint8_t dimmer_bitmask = bufx[7] & 0b111;
									
									animation_length = getBuf16(8);
									start_brightness = bufx[10];
									end_brightness = bufx[11];

									UART_PUTF("   Animation Mode: %u\r\n", animation_mode); // TODO: Set binary mode like 00010110
									//UART_PUTF("   Dimmer Bitmask: %u\r\n", dimmer_bitmask);
									UART_PUTF("   Animation Time: %us\r\n", animation_length);
									UART_PUTF(" Start Brightness: %u\r\n", start_brightness);
									UART_PUTF("   End Brightness: %u\r\n", end_brightness);
									
									animation_length = (uint32_t)((float)animation_length * 1000 / ANIMATION_CYCLE_MS);
									animation_position = 0;
									
									/* TODO: Write to EEPROM (?)
									// write back switch state to EEPROM
									switch_state[i] = req_state;
									switch_timeout[i] = req_timeout;
									
									eeprom_write_word((uint16_t*)EEPROM_POS_SWITCH_STATE + i * 2, u16);
									
									*/
									
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
									
									send_status_timeout = 25;
								}
							}
						}
					}
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
						setPWMDutyCycle(current_brightness);
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
						setPWMDutyCycle(current_brightness);
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
					setPWMDutyCycle(0);
				}
				else
				{
					UART_PUTS(" -> 100%\r\n");
					setPWMDutyCycle(100);
				}
			}
			else
			{
				// reverse dim direction
				manual_dim_direction = !manual_dim_direction;
			}
			
			button_state = 0;
		}
				
		// update brightness according animation_position, updated by timer0
		if (animation_length > 0)
		{
			pos = animation_position; // copy value to avoid that it changes in between by timer interrupt
			UART_PUTF2("%lu/%lu, ", pos, animation_length);
			
			if (pos == animation_length)
			{
				UART_PUTF("END Brightness %u%%, ", end_brightness * 100 / 255);
				setPWMDutyCycle((float)end_brightness * 100 / 255);
				animation_length = 0;
				animation_position = 0;
			}
			else
			{			
				float brightness = (start_brightness + ((float)end_brightness - start_brightness) * pos / animation_length) * 100 / 255;
				UART_PUTF("Br.%u%%, ", (uint32_t)(brightness));
				setPWMDutyCycle(brightness);
			}
		}			
		
		// send status from time to time
		if (send_status_timeout == 0)
		{
			send_status_timeout = SEND_STATUS_EVERY_SEC * (1000 / ANIMATION_UPDATE_MS);
			send_dimmer_status();
			led_blink(200, 0, 1);
		}

		rfm12_tick();
		send_status_timeout--;
		checkSwitchOff();
	}
	
	// never called
	// aes256_done(&aes_ctx);
}
