/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2019 Uwe Freese
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

#include "../src_common/util.h"

#define SPS30_I2C_ADR	0x69

void sps30_start_measurement(void);
void sps30_stop_measurement(void);
bool sps30_read_data_ready(void);
bool sps30_read_measured_values(void);
float sps30_get_measured_value(uint8_t pos);
