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
#include <stdlib.h>
#include <avr/eeprom.h>
#include <string.h>
#include "onewire.h"
#include "uart.h"
#include "util.h"

bool enable_write_eeprom = false;
uint8_t bytes_to_read = 0;
uint8_t bytes_pos = 0;

#ifdef ONEWIRE_SUPPORT
uint8_t ow_timer=3;
#endif

// This buffer is used for sending strings over UART using UART_PUT... functions.
char uartbuf[65]; 

#ifdef UART_RX
	// All received bytes from UART are stored in this buffer by the interrupt routine. This is a ringbuffer.
	#define RXBUF_LENGTH 50
	char rxbuf[RXBUF_LENGTH];
	uint8_t rxbuf_startpos = 0; // points to the first (oldest) byte to be processed from the buffer
	uint8_t rxbuf_count = 0; // number of bytes currently in the buffer

	// This buffer stores the command string, which is a copy of the allowed bytes from the rxbuf, filled in the main loop (not ISR!).
	char cmdbuf[50];

	uint8_t uart_timeout = 0;
#endif

// This buffer is used to store the bytes that will be sent by the RFM module.
char sendbuf[50];
bool send_data_avail = false;

// Take two characters and return the hex value they represent.
// If characters are not 0..9, a..f, A..F, character is interpreted as 0 or f.
// 0 = 48, 9 = 57
// A = 65, F = 70
// a = 97, f = 102
uint8_t hex_to_byte(char c)
{
	if (c <= 48) // 0
	{
		return 0;
	}
	else if (c <= 57) // 1..9
	{
		return c - 48;
	}
	else if (c <= 65) // A
	{
		return 10;
	}
	else if (c <= 70) // B..F
	{
		return c - 55;
	}
	else if (c <= 97) // a
	{
		return 10;
	}
	else if (c <= 102) // b..f
	{
		return c - 87;
	}
	else // f
	{
		return 15;
	}		
}

#ifdef UART_RX
void process_cmd(void)
{
	uart_putstr("Processing command: ");
	uart_putstr(cmdbuf);
	UART_PUTS("\r\n");
	
	if ((cmdbuf[0] == 'w') && (strlen(cmdbuf) == 5))
	{
		if (enable_write_eeprom)
		{
			uint16_t adr = hex_to_byte((uint8_t)cmdbuf[1]) * 16 + hex_to_byte((uint8_t)cmdbuf[2]);
			uint8_t val = hex_to_uint8((uint8_t *)cmdbuf, 3);
			UART_PUTF2("Writing data 0x%x to EEPROM pos 0x%x.\r\n", val, adr);
			eeprom_write_byte((uint8_t *)adr, val);
		}
		else
		{
			UART_PUTS("Ignoring EEPROM write, since write mode is DISABLED.\r\n");
		}
	}
	else if ((cmdbuf[0] == 'r') && (strlen(cmdbuf) == 3))
	{
		
		uint16_t adr = hex_to_byte((uint8_t)cmdbuf[1]) * 16 + hex_to_byte((uint8_t)cmdbuf[2]);
		uint8_t val = eeprom_read_byte((uint8_t *)adr);
		UART_PUTF2("EEPROM value at position 0x%x is 0x%x.\r\n", adr, val);
	}
	else if ((cmdbuf[0] == 's') && (strlen(cmdbuf) > 4))
	{
		strcpy(sendbuf, cmdbuf + 1);
		send_data_avail = true;
	}
	else
	{
		UART_PUTS("Unknown command.\r\n");
	}
}

// Store received byte in ringbuffer. No processing.
ISR(USART_RX_vect)
{
	if (rxbuf_count < RXBUF_LENGTH)
	{
		rxbuf[(rxbuf_startpos + rxbuf_count) % RXBUF_LENGTH] = UDR0;
		rxbuf_count++;
	} // else: Buffer overflow (undetected!)
}

// Process all bytes in the rxbuffer. This function should be called in the main loop.
// It can be interrupted by a UART RX interrupt, so additional bytes can be added into the ringbuffer while this function is running.
void process_rxbuf(void)
{
	while (rxbuf_count > 0)
	{
		char input;
		
		// get one char from the ringbuffer and reduce it's size without interruption through the UART ISR
		cli();
		input = rxbuf[rxbuf_startpos];
		rxbuf_startpos = (rxbuf_startpos + 1) % RXBUF_LENGTH;
		rxbuf_count--;
		sei();
		
		// process character	
		if (uart_timeout == 0)
		{
			bytes_to_read = bytes_pos = 0;
		}
		
		if (bytes_to_read > bytes_pos)
		{
			if (input == 13)
			{
				bytes_to_read = bytes_pos;
			}
			else if (((input >= 48) && (input <= 57)) || ((input >= 65) && (input <= 70)) || ((input >= 97) && (input <= 102)))
			{
				cmdbuf[bytes_pos] = input;
				bytes_pos++;
				UART_PUTF4("*** Received character %c (ASCII %u) = value 0x%x, %u bytes to go. ***\r\n", input, input, hex_to_byte(input), bytes_to_read - bytes_pos);
			}
			else
			{
				UART_PUTS("*** Illegal character. Use only 0..9, a..f, A..F. ***\r\n");
			}
			
			if (bytes_pos == bytes_to_read)
			{
				cmdbuf[bytes_pos] = '\0';
				process_cmd();
			}
		}	
		else if (input == 'h')
		{
			UART_PUTS("*** Help ***\r\n");
			UART_PUTS("h.........this help\r\n");
			UART_PUTS("rAA.......read EEPROM at hex address AA\r\n");
			UART_PUTS("wAAXX.....write EEPROM at hex address AA to hex value XX\r\n");
			UART_PUTS("x.........enable writing to EEPROM\r\n");
			UART_PUTS("z.........disable writing to EEPROM\r\n");
			UART_PUTS("sKKCCXX...Use AES key KK to send a packet with command ID CC and data XX (0..22 bytes).\r\n");
			UART_PUTS("          End data with ENTER. Packet number and CRC are automatically added.\r\n");
			UART_PUTS("o.........start onewire thermal sensor conversion\r\n");
		}
		else if (input == 'x')
		{
			enable_write_eeprom = true;
			UART_PUTS("*** Writing to EEPROM is now ENABLED. ***\r\n");
		}
		else if (input == 'z')
		{
			enable_write_eeprom = false;
			UART_PUTS("*** Writing to EEPROM is now DISABLED. ***\r\n");
		}
		else if (input == 'o'){
			ow_timer=3;
		}
		else if (input == 'r')
		{
			UART_PUTS("*** Read from EEPROM. Enter address (2 characters). ***\r\n");
			cmdbuf[0] = 'r';
			bytes_to_read = 3;
			bytes_pos = 1;
		}
		else if (input == 'w')
		{
			UART_PUTS("*** Write to EEPROM. Enter address and data (4 characters). ***\r\n");
			cmdbuf[0] = 'w';
			bytes_to_read = 5;
			bytes_pos = 1;
		}
		else if (input == 's')
		{
			UART_PUTS("*** Enter AES key nr, command ID and data in hex format to send, finish with ENTER. ***\r\n");
			cmdbuf[0] = 's';
			bytes_to_read = 49; // 's' + 2 characters for key nr + 2 characters for command ID + 2*22 characters for data
			bytes_pos = 1;
		}
		else
		{
			UART_PUTS("*** Character ignored. Press h for help. ***\r\n");
		}
		
		// enable user timeout if waiting for further input
		uart_timeout = bytes_to_read == bytes_pos ? 0 : 255;
	}
}
#endif

void uart_init(bool enable_RX) {
	PORTD |= 0x01;				            // Pullup an RXD an

 	UCSR0B |= (1 << TXEN0);			        // UART TX einschalten
 	UCSR0C |= (1 << USBS0) | (3 << UCSZ00);	// Asynchron 8N1
 	UCSR0B |= (1 << RXEN0 );			    // Uart RX einschalten
 
 	UBRR0H = (uint8_t)((UBRR_VAL) >> 8);
 	UBRR0L = (uint8_t)((UBRR_VAL) & 0xFF);

	// UF: Baudraten-Berechnung stimmt irgendwie nicht, obwohl F_CPU auf 4 MHz angepasst wurde. Daher manuell eingetragene Werte nehmen.
	// 1 MHz
	//UBRR0L = 0x03;
	//UBRR0H = 0x00;
	
	// 4 MHz
	//UBRR0L = 0x0c;
	//UBRR0H = 0x00;
	
	// 12 MHz
	//UBRR0L = 0x26;
	//UBRR0H = 0x00;

	if (enable_RX)
	{
		// activate rx IRQ
		UCSR0B |= (1 << RXCIE0);
	}
}

void uart_putc(char c) {
	// UF: UDRE0 statt UDRE, UDR0 statt UDR
	while (!(UCSR0A & (1<<UDRE0))); /* warten bis Senden moeglich                   */
	UDR0 = c;                       /* schreibt das Zeichen x auf die Schnittstelle */
}

void uart_putstr(char *str) {
	while(*str) {
		uart_putc(*str++);
	}
}

void uart_putstr_P(PGM_P str) {
	char tmp;
	while((tmp = pgm_read_byte(str))) {
		uart_putc(tmp);
		str++;
	}
}

uint16_t hex_to_uint8(uint8_t * buf, uint8_t offset)
{
	return hex_to_byte(buf[offset]) * 16 + hex_to_byte(buf[offset + 1]);
}
