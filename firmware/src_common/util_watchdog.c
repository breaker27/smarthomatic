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

#include "../src_common/msggrp_generic.h"

uint32_t _rfm_last_reception_ms = 0;
uint32_t _rfm_timeout_ms = 0; // 0 == watchdog disabled
uint16_t _rfm_watchdog_deviceid;
bool _rfm_retry_done = false;

void rfm_watchdog_init(uint16_t deviceid, uint16_t timeout_sec)
{
	_rfm_watchdog_deviceid = deviceid;
	_rfm_timeout_ms = (uint32_t)timeout_sec * 1000;
}

void rfm_watchdog_reset(void)
{
	_rfm_last_reception_ms = 0;
	_rfm_retry_done = false;
}

void _rfm12_recover(void)
{
	UART_PUTS("RFM watchdog timeout! Recovering...\r\n");
	
	rfm12_reset();
	_delay_ms(150);
	
	rfm12_init();
	_delay_ms(500);
	
	inc_packetcounter();

	// Set packet content
	pkg_header_init_generic_hardwareerror_status();
	pkg_header_set_senderid(_rfm_watchdog_deviceid);
	pkg_header_set_packetcounter(packetcounter);
	msg_generic_hardwareerror_set_errorcode(ERRORCODE_TRANSCEIVERWATCHDOGRESET);

	rfm12_send_bufx();
	
	_delay_ms(500);
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
			while (true) // endless error loop
			{
				led_blink(500, 500, 1);
			}
		}
	}
}
