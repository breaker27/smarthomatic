/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2013 Uwe Freese
*
* Original author of sht11_whitejack_v3 library:
*    Thorsten S. (whitejack), http://www.mikrocontroller.net/topic/145736
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

#include "config.h"
#include "sht11.h"

#define SHT11_CMD_TEMP	0x03
#define SHT11_CMD_HUMID	0x05
#define SHT11_CMD_WSTAT	0x06
#define SHT11_CMD_RSTAT	0x07
#define SHT11_CMD_RESET	0x1E

/////////////////////////////////////////////////////////////////////////////
// This version needs external pullups on SDA!
/////////////////////////////////////////////////////////////////////////////

static void	delay(void)	{ _delay_us(2); }

static void	scl_hi(void)    { setBits(PORT(SHT11_PORT), SHT11_SCL); }
static void	scl_lo(void)    { clrBits(PORT(SHT11_PORT), SHT11_SCL); }
static void	sda_hi(void)    { clrBits(DDR(SHT11_PORT), SHT11_SDA); }
static void	sda_lo(void)    { setBits(DDR(SHT11_PORT), SHT11_SDA); }
static void	scl_pulse(void) { scl_hi(); delay(); scl_lo(); }
static uint8_t	sda_val(void)   { return (PIN(SHT11_PORT) & SHT11_SDA) != 0; }

/////////////////////////////////////////////////////////////////////////////

static uint8_t crc_value;

static uint8_t  sht11_state;
static uint16_t sht11_hum_raw=0,
                sht11_tmp_raw=0;
static int16_t  sht11_hum=0,
                sht11_tmp=0;
             

static void
crc8(uint8_t b)
{
	uint8_t i;

    for (i = 0; i < 8; ++i) {
	if ((crc_value ^ b) & 0x80) {
	    crc_value <<= 1;
	    crc_value ^= 0x31;
	} else
	    crc_value <<= 1;
	b <<= 1;
    }
}

/////////////////////////////////////////////////////////////////////////////

static uint8_t
send(uint16_t b)
{
    uint8_t i;
    crc8(b);
	
    // data
    for (i = 0; i < 8; ++i)
	{
		if (b & 0x80)
			sda_hi();
		else
			sda_lo();
		b <<= 1;
		delay();
		scl_pulse();
    }

    // acknowledge
    sda_hi();
    delay();
    uint8_t ack = sda_val();
    scl_pulse();
    return ack;
}

static uint8_t
recv_data(void)
{
    uint8_t i;
    
	// data
    uint8_t b = 0;
    for (i = 0; i < 8; ++i)
	{
		// data is transmitted MSB first
		b <<= 1;
		if (sda_val())
			b |= 1;
		scl_pulse();
		delay();
    }

    // lo acknowledge
    sda_lo();
    delay();
    scl_pulse();
    sda_hi();
    delay();

    crc8(b);
    return b;
}

static uint8_t
recv_crc(void)
{
    uint8_t i;
	
	// data
    uint8_t b = 0;
    for (i = 0; i < 8; ++i)
	{
		// CRC is transmitted LSB first
		b >>= 1;
		if (sda_val())
			b |= 0x80;
		scl_pulse();
		delay();
    }

    // hi acknowledge
    sda_hi();
    delay();
    scl_pulse();
    delay();

    return b;
}

static void
start(void)
{
	uint8_t i;
	
    clrBits(PORT(SHT11_PORT), SHT11_SCL | SHT11_SDA);	// SCK output low, SDA input/high
    setBits(DDR(SHT11_PORT),  SHT11_SCL);
    clrBits(DDR(SHT11_PORT),  SHT11_SDA);
    delay();

    // reset communication
    for (i = 0; i < 10; ++i)
	{
		scl_pulse();
		delay();
    }

    // "start" sequence
    scl_hi(); delay();
    sda_lo(); delay();
    scl_lo(); delay();
    scl_hi(); delay();
    sda_hi(); delay();
    scl_lo(); delay();
}

/////////////////////////////////////////////////////////////////////////////
// Measurement sequence.



uint8_t
sht11_start_temp(void)
{
    crc_value = SHT11_LOWRES << 7; // bit-reversed
    start();
    return send(SHT11_CMD_TEMP) == 0;
}

uint8_t
sht11_start_humid(void)
{
    crc_value = SHT11_LOWRES << 7; // bit-reversed
    start();
    return send(SHT11_CMD_HUMID) == 0;
}

uint8_t
sht11_ready(void)
{
    return sda_val() == 0;
}

int16_t
result(void)
{
    if (!sht11_ready())
	return SHT11_UNAVAIL;
    int16_t v = recv_data() << 8; v |= recv_data();
    uint8_t crc = recv_crc();
    if (crc != crc_value)
	return SHT11_CRC_FAIL;
    return v;
}

int16_t
sht11_result_temp(void)
{
    int16_t v = result();
    if (sht11_valid(v))
    {
    sht11_tmp_raw=(uint16_t)v;
#if SHT11_LOWRES
	v = v * 4 - SHT11_TEMP_V_COMP;
#else
	v -= SHT11_TEMP_V_COMP;
#endif

    }
    return v;
}

int16_t
sht11_result_humid(void)
{
    int16_t v = result();
    if (sht11_valid(v)) 
    {
    sht11_hum_raw=(uint16_t)v;
    #if SHT11_LOWRES
    	// inspired by Werner Hoch, modified for low resolution mode
    	const int32_t C1 = (int32_t)(-4.0 * 100);
    	const int32_t C2 = (int32_t)(0.648 * 100 * (1L<<24));
    	const int32_t C3 = (int32_t)(-7.2e-4 * 100 * (1L<<24));
    	v = (int16_t)((((C3 * v + C2) >> 7) * v + (1L<<16)) >> 17) + C1;
        const int32_t T1 = (uint32_t)(0.01 * (1L<<30));
        const int32_t T2 = (uint32_t)(0.00128 * (1L<<30));
    #else
    	// inspired by Werner Hoch
    	const int32_t C1 = (int32_t)(-4.0 * 100);
    	const int32_t C2 = (int32_t)(0.0405 * 100 * (1L<<28));
    	const int32_t C3 = (int32_t)(-2.8e-6 * 100 * (1L<<30));
    	v = (int16_t)((((((C3 * v) >> 2) + C2) >> 11) * v + (1L<<16)) >> 17) + C1;
        const int32_t T1 = (uint32_t)(0.01 * (1L<<30));
        const int32_t T2 = (uint32_t)(0.00008 * (1L<<30));
    #endif
    // implemented by whitejack (temp compensation)
    v = (int16_t)( (((((int32_t)sht11_tmp-2500) * (int32_t)( (T1+T2*((int32_t)sht11_hum_raw)) >>13))>>17) + ((int32_t)v)) );
    }
    if(v>9999)v=9999;
    if(v<0001)v=0001;
    return v;


}
/////////////////////////////////////////////////////////////////////////////
// Initialize.

void
sht11_init(void)
{
    start();
    send(SHT11_CMD_RESET);
    _delay_ms(11);

    start();
    send(SHT11_CMD_WSTAT);
    send(SHT11_LOWRES);    //set resolution

    sht11_state=SHT11_STATE_MEASURE_TMP;
}

/////////////////////////////////////////////////////////////////////////////
// User.
void sht11_start_measure(void)
{
    if (sht11_state==SHT11_STATE_READY)
    {
        sht11_state=SHT11_STATE_MEASURE_TMP;
    }
}

uint8_t sht11_measure(void)
{
    switch (sht11_state)
    {
        case SHT11_STATE_MEASURE_TMP:
        {
            sht11_start_temp();
            sht11_state = SHT11_STATE_CALC_TMP;
        } break;
        case SHT11_STATE_CALC_TMP:
        {
            if(sht11_ready())
            {
                sht11_tmp = sht11_result_temp();
                sht11_state = SHT11_STATE_MEASURE_HUM;
            }
        } break;
        case SHT11_STATE_MEASURE_HUM:
        {
            sht11_start_humid();
            sht11_state = SHT11_STATE_CALC_HUM;
        } break;
        case SHT11_STATE_CALC_HUM:
        {
            if (sht11_ready())
            {
                sht11_hum = sht11_result_humid();
                sht11_state = SHT11_STATE_READY;
            }
        } break;
    }    
    return sht11_state;
}

uint8_t sht11_measure_finish(void)
{
    if (sht11_measure() == SHT11_STATE_READY)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

uint16_t sht11_get_tmp_raw(void)
{
    return sht11_tmp_raw;
}
uint16_t sht11_get_hum_raw(void)
{
    return sht11_hum_raw;
}
int16_t sht11_get_tmp(void)
{
    return sht11_tmp;
}
int16_t sht11_get_hum(void)
{
    return sht11_hum;
}
