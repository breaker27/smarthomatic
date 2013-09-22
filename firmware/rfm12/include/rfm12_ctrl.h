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
 
/** \file rfm12_ctrl.h
 * \brief rfm12 library live control feature header
 * \author Soeren Heisrath
 * \author Hans-Gert Dahmen
 * \author Peter Fuhrmann
 * \version 0.9.0
 * \date 08.09.09
 *
 * This header declares all functions necessary for setting the baud rate and frquency.
 *
 * \note It is usually not required to explicitly include this header,
 * as this is already done by rfm12.h.
 * \todo Add more livectrl functions.
 */
 
#ifndef _RFM12_CTRL_H
#define _RFM12_CTRL_H

/************************
* function protoypes	
*/
 
#if RFM12_LIVECTRL
//see rfm12_ctrl.c for more documentation
void rfm12_set_rate (uint16_t in_datarate);
void rfm12_set_frequency (uint16_t in_freq);
#endif /* RFM12_LIVECTRL */

#endif /* _RFM12_EXTRA_H */