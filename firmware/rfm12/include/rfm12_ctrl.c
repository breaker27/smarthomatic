/*
* This file is part of smarthomatic, http://www.smarthomatic.com.
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

/** \file rfm12_ctrl.c
 * \brief rfm12 library live control feature source
 * \author Soeren Heisrath
 * \author Hans-Gert Dahmen
 * \author Peter Fuhrmann
 * \version 0.9.0
 * \date 08.09.09
 *
 * This file implements all functions necessary for setting the baud rate and frquency.
 *
 * \note This file is included directly by rfm12.c, for performance reasons.
 * \todo Add more livectrl functions.
 */
 
/******************************************************
 *    THIS FILE IS BEING INCLUDED DIRECTLY		*
 *		(for performance reasons)				*
 ******************************************************/


#if RFM12_LIVECTRL
//! Set the data rate of the rf12.
/** The data rate has to be specified using the following macros:
* - RFM12_DATARATE_CALC_HIGH(x) for rates >= 2700 Baud
* - RFM12_DATARATE_CALC_LOW(x) for rates from 340 to < 2700 Baud
*
* Please refer to the rfm12 library configuration header for a demo macro usage. \n
* The data rate calculation macros can be found in  rfm12_hw.h.
* They are not included as a function for code-size reasons.
*/
void rfm12_set_rate (uint16_t in_datarate)
{
	rfm12_data(RFM12_CMD_DATARATE | DATARATE_VALUE );
}

//! Set the frequency of the rf12.
/** The frequency has to be specified using the RFM12_FREQUENCY_CALC_433(x) macro.
*
* Please refer to the rfm12 library configuration header for a demo macro usage. \n
* The frequency calculation macro can be found in  rfm12_hw.h.
* It is not included as a function for code-size reasons.
*/
void rfm12_set_frequency (uint16_t in_freq)
{
	rfm12_data(RFM12_CMD_FREQUENCY | in_freq );
}
#endif