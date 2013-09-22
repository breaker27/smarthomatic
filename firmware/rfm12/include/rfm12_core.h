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
 
#ifndef _RFM12_CORE_H
#define _RFM12_CORE_H

/************************
* VARIOUS RFM RELATED DEFINES FOR INTERNAL USE	
*(defines which shall be visible to the user are located in rfm12.h)
*/

//default preamble (altrernating 1s and 0s)
#define PREAMBLE 0xAA

//default synchronization pattern
#define SYNC_MSB 0x2D
#define SYNC_LSB 0xD4

//these are the states for the receive/transmit state machine
#define STATE_RX_IDLE 0
#define STATE_RX_ACTIVE 1
#define STATE_TX 2

//packet header length in bytes
#define PACKET_OVERHEAD 3


/************************
* LIBRARY DEFAULT SETTINGS	
 */
 
//if notreturns is not defined, we won't use this feature
#ifndef RFM12_NORETURNS
	#define RFM12_NORETURNS 0
#endif

//if transmit only is not defined, we won't use this feature
#ifndef RFM12_TRANSMIT_ONLY
	#define RFM12_TRANSMIT_ONLY 0
#endif

//if transmit only is on, we need to turn of collision detection
#if RFM12_TRANSMIT_ONLY
	//disable collision detection, as we won't be able to receive data
	#ifdef RFM12_NOCOLLISIONDETECTION
		#undef RFM12_NOCOLLISIONDETECTION
	#endif
	#define RFM12_NOCOLLISIONDETECTION 1
#endif

//if nocollisiondetection is not defined, we won't use this feature
#ifndef RFM12_NOCOLLISIONDETECTION
	#define RFM12_NOCOLLISIONDETECTION 0
#endif

//if livectrl is not defined, we won't use this feature
#ifndef RFM12_LIVECTRL
	#define RFM12_LIVECTRL 0
#endif

//if low battery detector is not defined, we won't use this feature
#ifndef RFM12_LOW_BATT_DETECTOR
	#define RFM12_LOW_BATT_DETECTOR 0
#endif

//if the low battery detector feature is used, we will set some extra pwrmgmt options
#if RFM12_LOW_BATT_DETECTOR
	//define PWRMGMT_LOW_BATT  with low batt detector
	//it will be used later
	#define PWRMGMT_LOW_BATT (RFM12_PWRMGT_EB)
	
	//check if the default power management setting has the LB bit set
	//and warn the user if it's not
	#ifdef PWRMGT_DEFAULT
		#if !((PWRMGT_DEFAULT) & RFM12_PWRMGT_EB)
			#warning "You are using the RFM12 low battery detector, but PWRMGT_DEFAULT has the low battery detector bit unset."
		#endif
	#endif
#else
	#define PWRMGMT_LOW_BATT 0
#endif /* RFM12_LOW_BATT_DETECTOR */

//if wakeuptimer is not defined, we won't use this feature
#ifndef RFM12_USE_WAKEUP_TIMER
	#define RFM12_USE_WAKEUP_TIMER 0
#endif

//if wakeuptimer is on, we will set the default power management to use the wakeup timer
#if RFM12_USE_WAKEUP_TIMER
	//define PWRMGMT_LOW_BATT  with low batt detector
	//it will be used later
	#define PWRMGMT_WKUP (RFM12_PWRMGT_EW)
	
	//check if the default power management setting has the EW bit set
	//and warn the user if it's not
	#ifdef PWRMGT_DEFAULT
		#if !((PWRMGT_DEFAULT) & RFM12_PWRMGT_EW)
			#warning "You are using the RFM12 wakeup timer, but PWRMGT_DEFAULT has the wakeup timer bit unset."
		#endif
	#endif
#else
	#define PWRMGMT_WKUP 0
#endif /* RFM12_USE_WAKEUP_TIMER */

//if ASK tx is not defined, we won't use this feature
#ifndef RFM12_TRANSMIT_ASK
	#define RFM12_TRANSMIT_ASK 0
#endif

//if receive ASK is not defined, we won't use this feature
#ifndef RFM12_RECEIVE_ASK
	#define RFM12_RECEIVE_ASK 0
#endif

//if software spi is not defined, we won't use this feature
#ifndef RFM12_SPI_SOFTWARE
	#define RFM12_SPI_SOFTWARE 0
#endif

//if uart debug is not defined, we won't use this feature
#ifndef RFM12_UART_DEBUG
	#define RFM12_UART_DEBUG 0
#endif
 
//default value for powermanagement register
#ifndef PWRMGT_DEFAULT
	#define PWRMGT_DEFAULT (RFM12_PWRMGT_DC | PWRMGMT_WKUP | PWRMGMT_LOW_BATT)
#endif

//define a default receive power management mode
#if RFM12_TRANSMIT_ONLY
	//the receiver is turned off by default in transmit only mode
	#define PWRMGT_RECEIVE (RFM12_CMD_PWRMGT | PWRMGT_DEFAULT)
#else
	//the receiver is turned on by default in normal mode
	#define PWRMGT_RECEIVE (RFM12_CMD_PWRMGT | PWRMGT_DEFAULT | RFM12_PWRMGT_ER)
#endif

//default channel free time, if not defined elsewhere
#ifndef CHANNEL_FREE_TIME
	#define CHANNEL_FREE_TIME 200
#endif

/*
 * backward compatibility for the spi stuff
 * these values weren't set in older revisions of this library
 * so they're now assumed to be on the same pin/port.
 * otherwise these defines serve to configure the software SPI ports
 */
#ifndef DDR_MOSI
	#define DDR_MOSI DDR_SPI
	#define PORT_MOSI PORT_SPI
#endif

#ifndef DDR_MISO
	#define DDR_MISO DDR_SPI
	#define PIN_MISO PIN_SPI
#endif

#ifndef DDR_SCK
	#define DDR_SCK DDR_SPI
	#define PORT_SCK PORT_SPI
#endif

#ifndef DDR_SPI_SS
	#define DDR_SPI_SS DDR_SPI
	#define PORT_SPI_SS PORT_SPI
#endif


//baseband selection
#if (RFM12_BASEBAND) == RFM12_BAND_433
	#define RFM12_FREQUENCY_CALC(x) RFM12_FREQUENCY_CALC_433(x)
#elif (RFM12_BASEBAND) == RFM12_BAND_868
	#define RFM12_FREQUENCY_CALC(x) RFM12_FREQUENCY_CALC_868(x)
#elif (RFM12_BASEBAND) == RFM12_BAND_915
	#define RFM12_FREQUENCY_CALC(x) RFM12_FREQUENCY_CALC_915(x)
#else
	#error "Unsupported RFM12 baseband selected."
#endif


/************************
* HELPER MACROS
*/

//macros to turn the int on and off
//if polling is used, just define these macros as empty
#if !(RFM12_USE_POLLING)
	#define RFM12_INT_ON() RFM12_INT_MSK |= (1<<RFM12_INT_BIT)
	#define RFM12_INT_OFF() RFM12_INT_MSK &= ~(1<<RFM12_INT_BIT)
#else
	#define RFM12_INT_ON()
	#define RFM12_INT_OFF()
#endif /* !(RFM12_USE_POLLING) */

/*
 * the following macros help to manage the rfm12 fifo
 * default fiforeset is as follows:
 * 2 Byte Sync Pattern, disable sensitive reset, fifo filled interrupt at 8 bits
 */
//default fiforeset register value to accept data
#define ACCEPT_DATA (RFM12_CMD_FIFORESET | RFM12_FIFORESET_DR | (8<<4) | RFM12_FIFORESET_FF)
#define ACCEPT_DATA_INLINE (RFM12_FIFORESET_DR | (8<<4) | RFM12_FIFORESET_FF)
//default fiforeset register value to clear fifo
#define CLEAR_FIFO (RFM12_CMD_FIFORESET | RFM12_FIFORESET_DR | (8<<4))
#define CLEAR_FIFO_INLINE (RFM12_FIFORESET_DR | (8<<4))

//this macro helps to encapsulate the return values, when noreturn is set to on
#if (RFM12_NORETURNS)
	#define TXRETURN(a)
#else
	#define TXRETURN(a) (a)
#endif

#endif /* _RFM12_CORE_H */