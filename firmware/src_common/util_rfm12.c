/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2014 Uwe Freese
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

#include "util_rfm12.h"
#include "util_hw.h"
#include "util_watchdog.h"
#include "../rfm12/rfm12.h"

// Make 20ms delay, call rfm12 tick (4 times, in 5ms cycle) and remember watchdog time.
void rfm12_delay20(void)
{
	uint8_t i;

	for (i = 0; i < 4; i++)
	{
		_delay_ms(5);
		rfm12_tick();
	}

	rfm_watchdog_count(20);
}

void rfm12_delay10_led(void)
{
	switch_led(true);
	rfm12_tick();
	_delay_ms(5);
	rfm12_tick();
	_delay_ms(5);
	switch_led(false);
	rfm_watchdog_count(10);
}

// Send out waiting RFM12 packet from tx buffer.
// Let LED stay on for 150ms.
// Wait max. 300ms for sending out the data.
// Typical and minimal time is 150ms (according CHANNEL_FREE_TIME in rfm12.c and 5ms cycle of rfm12_tick).
// Functions returns the time needed to send out the packet (depends on availability of air channel).
uint16_t rfm12_send_wait_led(void)
{
	uint8_t i;
	uint8_t j = 0;

	switch_led(true);

	for (i = 1; i <= 60; i++)
	{
		rfm12_tick();
		_delay_ms(5);

		// remember when tx packet was sent
		if ((j == 0) && (rfm12_tx_status() == STATUS_FREE))
			j = i;

		if (i == 30)
			switch_led(false);

		// end loop immediately when packet was sent, min. after 150ms, max. 300ms
		if ((i >= 30) && (j > 0))
			break;
	}

	if (j == 0)
		j = 60;

	rfm_watchdog_count(i * 5);
	return (uint16_t)j * 5;
}
