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

/*
 * Initialize.
 */
void		sht11_init(void);

/*
 * External f .User.
 */

//call every x seconds to start measure (starts a sequence to measure and calculate temp and hum)
void        sht11_start_measure(void);

//poll to controle the measurement an get the values, finish returns 1 else 0 (calls sht11_measure())
uint8_t     sht11_measure_finish(void);

    //get temperature raw value (if measure is finish)
    uint16_t    sht11_get_tmp_raw(void);

    //get humidity raw value (if measure is finish)
    uint16_t    sht11_get_hum_raw(void);

    //get temperature value *100 (if measure is finish)
    int16_t     sht11_get_tmp(void);

    //get humidity value *100 (if measure is finish)
    int16_t     sht11_get_hum(void);

//returns and controles the measurement statemachine (called by sht11_measure_finish())
uint8_t     sht11_measure(void);



/*
 * Internal 
 */

/*
 * Start measurement (humidity or temperature).
 * Return "device found".
 * Afterwards poll sht11_ready.
 */
uint8_t		sht11_start_temp(void);
uint8_t		sht11_start_humid(void);

/*
 * Return 0 unless measurement completed.
 */
uint8_t		sht11_ready(void);

/*
 * Return result of measurement.
 * H: 100*%RH (0..10000)
 * T: 100*T
 * Return -32xxx on failure.
 */
int16_t     result(void);
int16_t		sht11_result_temp(void);
int16_t		sht11_result_humid(void);







#define		SHT11_UNAVAIL               -32768
#define		SHT11_CRC_FAIL              -32767
#define		sht11_valid(v)              ((v) > -32000)

#define     SHT11_STATE_READY           0
#define     SHT11_STATE_MEASURE_TMP     1
#define     SHT11_STATE_CALC_TMP        2
#define     SHT11_STATE_MEASURE_HUM     3
#define     SHT11_STATE_CALC_HUM        4

