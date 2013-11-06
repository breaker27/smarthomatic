/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2013 Uwe Freese
*
* Original authors of RFM 12 library:
*    Peter Fuhrmann, Hans-Gert Dahmen, Soeren Heisrath
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

/******************************************************
 *                                                    *
 *           C O N F I G U R A T I O N                *
 *                                                    *
 ******************************************************/

/*
	Connect the RFM12 to the AVR as follows:

	RFM12           | AVR
	----------------+------------
	SDO             | MISO
	nIRQ            | INT0
	FSK/DATA/nFFS   | VCC
	DCLK/CFIL/FFIT  |  -
	CLK             |  -
	nRES            |  -
	GND             | GND
	ANT             |  -
	VDD             | VCC
	GND             | GND
	nINT/VDI        | -
	SDI             | MOSI
	SCK             | SCK
	nSEL            | Slave select pin defined below
*/


//Pin that the RFM12's slave select is connected to
#define DDR_SS DDRB
#define PORT_SS PORTB
#define BIT_SS 0

//SPI port
#define DDR_SPI DDRB
#define PORT_SPI PORTB
#define PIN_SPI PINB
#define BIT_MOSI 3
#define BIT_MISO 4
#define BIT_SCK  5
#define BIT_SPI_SS 2
//this is the hardware SS pin of the AVR - it 
//needs to be set to output for the spi-interface to work 
//correctly, independently of the CS pin used for the RFM12

//frequency to use
#define FREQ 868000000UL
#define RFM12_BASEBAND RFM12_BAND_868

//use this for datarates >= 2700 Baud
#define DATARATE_VALUE RFM12_DATARATE_CALC_HIGH(9600.0)

//use this for 340 Baud < datarate < 2700 Baud
//#define DATARATE_VALUE RFM12_DATARATE_CALC_LOW(1200.0)

/**** TX BUFFER SIZE
 */
#define RFM12_TX_BUFFER_SIZE 50

/**** RX BUFFER SIZE
 * there are going to be 2 Buffers of this size
 * (double_buffering)
 */
#define RFM12_RX_BUFFER_SIZE 50

/**** INTERRUPT VECTOR
 * define the interrupt vector settings here
 */
 
//the interrupt vector
#define RFM12_INT_VECT (INT0_vect)

//the interrupt mask register
// #define RFM12_INT_MSK GICR
// UF: statt GICR (General Interrupt Control register) des ATMega8
// muss das EIMSK (External Interrupt Mask Register) des ATMega88 verwendet werden.
#define RFM12_INT_MSK EIMSK

//the interrupt bit in the mask register
#define RFM12_INT_BIT (INT0)

//the interrupt flag register
// #define RFM12_INT_FLAG GIFR
// UF: statt GIFR (General Interrupt Flag Register) des ATMega8
// muss das EIFR (External Interrupt Flag Register) des ATMega88 verwendet werden.
#define RFM12_INT_FLAG EIFR

//the interrupt bit in the flag register
#define RFM12_FLAG_BIT (INTF0)

//setup the interrupt to trigger on negative edge
// #define RFM12_INT_SETUP()   MCUCR |= (1<<ISC01)
// UF: statt MCUCR (MCU Control Register) des ATMega8
// muss das EICRA (External Interrupt Control Register A) des ATMega88 verwendet werden.
// UF: Benutze (wie in RFM12 lib empfohlen) low level interrupt statt low edge triggered.
// Sonst wird Wakeup-Interrupt nicht erkannt!
//#define RFM12_INT_SETUP()   EICRA |= (1<<ISC01)
// ISC01 NICHT setzen!
#define RFM12_INT_SETUP()

/**** UART DEBUGGING
 * en- or disable debugging via uart.
 */
#define RFM12_UART_DEBUG 0

/*
This is a bitmask that defines how "rude" this library behaves
	0x01: ignore other devices when sending
	0x04: don't use return values for transmission functions
*/

/* control rate, frequency, etc during runtime
 * this setting will certainly add a bit code
 **/
#define RFM12_LIVECTRL 0
#define RFM12_NORETURNS 0
#define RFM12_USE_WAKEUP_TIMER 1 // Do not use - it leads to problems... Use Timer2 Power Safe mode of ATMega88 instead.
#define RFM12_TRANSMIT_ONLY 1
#define RFM12_NOCOLLISIONDETECTION 0
#define RFM12_USE_POLLING 0
#define RFM12_LOW_POWER 0

// FIXME: compiler shows a warning if not defined. Usually these should be defined by rfm12_core.h already.
#define RFM12_LOW_BATT_DETECTOR 0
#define RFM12_RECEIVE_ASK 0
#define RFM12_TRANSMIT_ASK 0

/* Disable interrupt vector and run purely inline. This may be useful for
 * configurations where a hardware interrupt is not available.
 */
#define RFM12_NOIRQ 0
