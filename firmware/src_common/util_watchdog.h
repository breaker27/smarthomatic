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

#ifndef _UTIL_WATCHDOG_H
#define _UTIL_WATCHDOG_H

/*
 * Initialize watchdog with needed parameters. Call once after startup.
 * Use nres_port_nr > 2 to not use the HW reset (NRES) pin.
 */
void rfm_watchdog_init(uint16_t deviceid, uint16_t timeout_10sec, uint8_t nres_port_nr, uint8_t nres_pin);

/*
 * Reset watchdog counter because RFM transceiver is alive.
 * Call whenever data was received (as a sign that transceiver is working properly).
 */
void rfm_watchdog_alive(void);

/*
 * Tell watchdog the (additional) time that passed till last call of this
 * function. The watchdog will automatically start a retry after timeout.
 */
void rfm_watchdog_count(uint16_t ms);

/*
 * If startup was by error (e.g. watchdog), send an error message once and return true.
 * Otherwise, do nothing and return false;
 */
bool send_startup_reason(uint8_t *mcusr_mirror);

#endif /* _UTIL_WATCHDOG_H */
