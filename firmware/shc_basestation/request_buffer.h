/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2013 Uwe Freese
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

#ifndef REQUEST_BUFFER_H
#define REQUEST_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

// The request buffer is used to remember requests that were sent until an acknowledge
// for the request is received. After a timeout, the request is then repeated.
// The request buffer can queue only 6 byte-messages currently (there are no longer
// requests defined yet).

#define REQUEST_BUFFER_SIZE 8          // How many request can be queued in total?
#define REQUEST_QUEUE_RECEIVERS 4      // For how many receivery should messages be queued at maximum?
#define REQUEST_QUEUE_PACKETS 4        // How many request should be queued per receiver at maximum?
#define REQUEST_RETRY_COUNT 5          // number of retries when no acknowledge is received for a request yet
#define REQUEST_INITIAL_TIMEOUT_S 5    // The initial timeout in seconds. Please note that a receiver needs some time to receive,
                                       // decode, react, encode and send an acknowledge. So don't make the timeout too short!
#define REQUEST_ADDITIONAL_TIMEOUT_S 2 // Additional timeout per retry.
#define RS_UNUSED 255

typedef struct {
	uint8_t command_id; // set to RS_UNUSED to show that this buffer is unused
	uint8_t aes_key;
	uint32_t packet_counter;
	uint8_t data[5]; 

	uint8_t timeout;
	uint8_t retry_count;
} request_t; 

// The buffer holds the requests independently of the receiver ID.
extern request_t request_buffer[REQUEST_BUFFER_SIZE];

// The request queue is a lookup table which shows requests are queued for which receivers.
// This is to support many requests for few receivers also as few requests for many receivers
// with a limited size of the request_buffer.
extern uint8_t request_queue[REQUEST_QUEUE_RECEIVERS][REQUEST_QUEUE_PACKETS + 1];

void request_queue_init(void);
void print_request_queue(void);
bool queue_request(uint8_t receiver_id, uint8_t command_id, uint8_t aes_key, uint8_t * data);
bool set_repeat_request(uint32_t packet_counter);
void remove_request(uint8_t sender_id, uint8_t request_sender_id, uint32_t packet_counter);

#endif