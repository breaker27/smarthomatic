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

#ifndef _UART_H
#define _UART_H

#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>
#include <avr/pgmspace.h>

// http://www.mikrocontroller.net/articles/AVR-GCC-Tutorial/Der_UART#UART_initialisieren
#ifndef UART_BAUD_RATE
	#error UART_BAUD_RATE not defined!
#endif

#define UBRR_VAL ((F_CPU + UART_BAUD_RATE * 8) / (UART_BAUD_RATE * 16) - 1)    // round value
#define BAUD_REAL (F_CPU / (16l * (UBRR_VAL + 1)))                             // real baudrate
#define BAUD_ERROR ((BAUD_REAL * 1000l) / UART_BAUD_RATE)                      // error in promille, 1000 = optimum

#if ((BAUD_ERROR < 985) || (BAUD_ERROR > 1015))
	#error Systematic UART baud rate error is greater than 1,5% and therefore too high!
#endif 

/* The unbuffered functions UART_PUTS, UART_PUTF,... send out characters immediately to UART.
   The buffered versions of the functions, UART_PUTS_B, UART_PUTF_B,... write characters only to uart_bufx.
   The characters are sent to UART using UART_SEND_BUF, which also clears the buffer counter to 0. */

#ifdef UART_DEBUG
	#define UART_PUTS(X)                 uart_putstr_P(PSTR((X)));
	#define UART_PUTF(X, A)              {sprintf_P(uartbuf, PSTR((X)), (A)); uart_putstr(uartbuf);}
	#define UART_PUTF2(X, A, B)          {sprintf_P(uartbuf, PSTR((X)), (A), (B)); uart_putstr(uartbuf);}
	#define UART_PUTF3(X, A, B, C)       {sprintf_P(uartbuf, PSTR((X)), (A), (B), (C)); uart_putstr(uartbuf);}
	#define UART_PUTF4(X, A, B, C, D)    {sprintf_P(uartbuf, PSTR((X)), (A), (B), (C), (D)); uart_putstr(uartbuf);}

	#define UART_PUTS_B(X)               uart_putstr_P_B(PSTR((X)));
	#define UART_PUTF_B(X, A)            {sprintf_P(uartbuf + strlen(uartbuf), PSTR((X)), (A));}
	#define UART_PUTF2_B(X, A, B)        {sprintf_P(uartbuf + strlen(uartbuf), PSTR((X)), (A), (B));}
	#define UART_PUTF3_B(X, A, B, C)     {sprintf_P(uartbuf + strlen(uartbuf), PSTR((X)), (A), (B), (C));}
	#define UART_PUTF4_B(X, A, B, C, D)  {sprintf_P(uartbuf + strlen(uartbuf), PSTR((X)), (A), (B), (C), (D));}
	#define UART_SEND_BUF                {uart_putstr(uartbuf); uartbuf[0] = 0;}
#else
	#define UART_PUTS(X)                 /* noop */
	#define UART_PUTF(X, A)              /* noop */
	#define UART_PUTF2(X, A, B)          /* noop */
	#define UART_PUTF3(X, A, B, C)       /* noop */
	#define UART_PUTF4(X, A, B, C, D)    /* noop */

	#define UART_PUTS_B(X)               /* noop */
	#define UART_PUTF_B(X, A)            /* noop */
	#define UART_PUTF2_B(X, A, B)        /* noop */
	#define UART_PUTF3_B(X, A, B, C)     /* noop */
	#define UART_PUTF4_B(X, A, B, C, D)  /* noop */
#endif

extern char uartbuf[];

#ifdef UART_RX
	extern char cmdbuf[];
	extern uint8_t uart_timeout;
	extern bool send_data_avail;
#endif

void uart_init_ubbr(uint16_t ubrr_val);
void uart_init(void);
void uart_putstr(char * str);
void uart_putstr_P(PGM_P str);
void uart_putstr_P_B(PGM_P str);

void print_signed(int16_t i);
void print_bytearray(uint8_t * b, uint8_t len);

#ifdef UART_RX
	void process_rxbuf(void);
#endif

#endif /* _UART_H */
