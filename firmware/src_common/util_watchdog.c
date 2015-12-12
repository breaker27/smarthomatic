/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2015 Uwe Freese
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

#include "util_watchdog.h"
#include <avr/wdt.h>

#include "../src_common/msggrp_generic.h"

uint32_t _rfm_last_reception_ms = 0;
uint32_t _rfm_timeout_ms = 0; // 0 == watchdog disabled
uint16_t _rfm_watchdog_deviceid;
uint8_t _nres_port_nr = 0;
uint8_t _nres_pin = 0;
bool _rfm_retry_done = false;

// Setup IO pin connected to RFM NRES pin from floating to high (== no reset).
void _setup_nres(uint8_t port_nr, uint8_t pin)
{
	switch (port_nr)
	{
		case 0:
			sbi(PORTB, pin);
			sbi(DDRB, pin);
			break;
		case 1:
			sbi(PORTC, pin);
			sbi(DDRC, pin);
			break;
		case 2:
			sbi(PORTD, pin);
			sbi(DDRD, pin);
			break;
	}
	
	_delay_ms(200);
}

void rfm_watchdog_init(uint16_t deviceid, uint16_t timeout_10sec, uint8_t nres_port_nr, uint8_t nres_pin)
{
	_rfm_watchdog_deviceid = deviceid;
	_rfm_timeout_ms = (uint32_t)timeout_10sec * 10 * 1000;
	_nres_port_nr = nres_port_nr;
	_nres_pin = nres_pin;

	_setup_nres(nres_port_nr, nres_pin);
}

void rfm_watchdog_alive(void)
{
	_rfm_last_reception_ms = 0;
	_rfm_retry_done = false;
}

void _rfm12_pull_nres(volatile uint8_t *port, uint8_t pin)
{
	*port &= ~(1 << pin);
	_delay_ms(500);
	*port |= (1 << pin);
	_delay_ms(500);
}

void _rfm12_hw_reset(void)
{
	switch (_nres_port_nr)
	{
		case 0:
			_rfm12_pull_nres(&PORTB, _nres_pin);
			break;
		case 1:
			_rfm12_pull_nres(&PORTC, _nres_pin);
			break;
		case 2:
			_rfm12_pull_nres(&PORTD, _nres_pin);
			break;
	}
}

// Initiate a reset of the ATMega by enabling the watchdog and waiting infinitely
// by purpose.
void _atmega_watchdog_reset(void)
{
    wdt_enable(WDTO_15MS);

	while (1)
	{ }
}

void _rfm12_recover(void)
{
	UART_PUTS("RFM watchdog timeout! Resetting RFM module...\r\n");
	
	rfm12_sw_reset();
	_rfm12_hw_reset();
	
	_atmega_watchdog_reset();
	
	/* UNUSED CODE. May be used again later as an alternative to reset RFM12 without resetting ATMega.

	rfm12_init();
	_delay_ms(500);
	
	inc_packetcounter();

	// Set packet content
	pkg_header_init_generic_hardwareerror_status();
	pkg_header_set_senderid(_rfm_watchdog_deviceid);
	pkg_header_set_packetcounter(packetcounter);
	msg_generic_hardwareerror_set_errorcode(ERRORCODE_TRANSCEIVERWATCHDOGRESET);

	rfm12_send_bufx();

	_delay_ms(500); */
}

void rfm_watchdog_count(uint16_t ms)
{
	if (_rfm_timeout_ms) // 0 == watchdog disabled
	{
		_rfm_last_reception_ms += ms;
		
		if (!_rfm_retry_done && (_rfm_last_reception_ms > _rfm_timeout_ms))
		{
			_rfm12_recover();
			_rfm_retry_done = true;
		}
		else if (_rfm_last_reception_ms > 2 * _rfm_timeout_ms)
		{
			UART_PUTS("RFM watchdog timeout 2! ERROR, halt!\r\n");

			while (true) // endless error loop
			{
				led_blink(500, 500, 1);
			}
		}
	}
}

bool send_startup_reason(uint8_t *mcusr_mirror)
{
	if (*mcusr_mirror == 0) // register was already cleared after startup by this function
	{
		return false;
	}
	else if ((*mcusr_mirror & (~(1 << PORF))) == 0) // no exceptional startup reason
	{
		*mcusr_mirror = 0;
		return false;
	}
	else
	{
		inc_packetcounter();

		// Set packet content
		pkg_header_init_generic_hardwareerror_status();
		pkg_header_set_senderid(_rfm_watchdog_deviceid);
		pkg_header_set_packetcounter(packetcounter);
		
		if (*mcusr_mirror & (1 << EXTRF))
		{
			msg_generic_hardwareerror_set_errorcode(ERRORCODE_EXTERNALRESET);
			rfm12_send_bufx();
		}
		else if (*mcusr_mirror & (1 << BORF))
		{
			msg_generic_hardwareerror_set_errorcode(ERRORCODE_BROWNOUTRESET);
			rfm12_send_bufx();
		}
		else if (*mcusr_mirror & (1 << WDRF))
		{
			// Assume WD reset was on purpose after RFM12 fault, since normal ATMega WD is not used.
			msg_generic_hardwareerror_set_errorcode(ERRORCODE_TRANSCEIVERWATCHDOGRESET);
			rfm12_send_bufx();
		}
		
		// If none of the cases matches (which should not be possible), the message is not sent.
		// (Don't move "rfm12_send_bufx();" here to be sure not to send garbage!)
		
		*mcusr_mirror = 0;
		return true;
	}
}
