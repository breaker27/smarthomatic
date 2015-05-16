/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2013..2015 Uwe Freese
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

#include "request_buffer.h"
#include "../src_common/uart.h"
#include "../src_common/util.h"
#include <string.h>

#include "../src_common/msggrp_generic.h"

// The request buffer holds several requests for different receivers.
request_t request_buffer[];

// The request queue holds the id of the device it holds the data for and a number of indices to the request buffer slots that are used for this device.
// The items request_queue[DEVICEID][0] holds the device id DEVICEID,
// items request_queue[DEVICEID][x] with x > 0 hold the indices to the request buffer.
uint16_t request_queue[REQUEST_QUEUE_RECEIVERS][REQUEST_QUEUE_PACKETS + 1];

void request_queue_init(void)
{
	uint8_t i, j;
	
	for (i = 0; i < REQUEST_BUFFER_SIZE; i++)
	{
		request_buffer[i].message_type = MESSAGETYPE_UNUSED;
	}

	for (i = 0; i < REQUEST_QUEUE_RECEIVERS; i++)
	{
		for (j = 0; j < REQUEST_QUEUE_PACKETS + 1; j++)
		{
			request_queue[i][j] = SLOT_UNUSED;
		}
	}
}

// Remember the request in the request queue and return if it was successful.
// If not, the request will not be queued and therefore not repeated if no acknowledge is received.
bool queue_request(uint16_t receiver_id, uint8_t message_type, uint8_t aes_key, uint8_t * data, uint8_t data_bytes)
{
	// Search free slot in request_buffer.
	uint8_t rb_slot = 0;
	
	while (request_buffer[rb_slot].message_type != MESSAGETYPE_UNUSED)
	{
		rb_slot++;
		
		if (rb_slot == REQUEST_BUFFER_SIZE)
		{
			return false; // ERROR: buffer is full!
		}
	}
	
	// Search free slot in request_queue for receiver_id.
	uint8_t rs_slot = 0;
	
	while ((request_queue[rs_slot][0] != SLOT_UNUSED) && (request_queue[rs_slot][0] != receiver_id))
	{
		rs_slot++;
		
		if (rs_slot == REQUEST_QUEUE_RECEIVERS)
		{
			return false; // ERROR: buffer is full!
		}
	}
	
	// set receiver_id in request_queue.
	request_queue[rs_slot][0] = receiver_id;
	
	// Search free slot in request_queue for request.
	uint8_t msg_slot = 1;
	
	while (request_queue[rs_slot][msg_slot] != SLOT_UNUSED)
	{
		msg_slot++;
		
		if (msg_slot == REQUEST_QUEUE_PACKETS + 1)
		{
			return false; // ERROR: buffer is full!
		}
	}
	
	// Set id of request_buffer index in request queue.
	request_queue[rs_slot][msg_slot] = rb_slot;
	
	if (data_bytes > REQUEST_DATA_BYTES_MAX)
	{
		data_bytes = REQUEST_DATA_BYTES_MAX;
	}
	
	// Set data in request_buffer.
	request_buffer[rb_slot].message_type = message_type;
	request_buffer[rb_slot].aes_key = aes_key;
	request_buffer[rb_slot].packet_counter = 0;
	memcpy(request_buffer[rb_slot].data, data, data_bytes);
	request_buffer[rb_slot].data_bytes = data_bytes;
	request_buffer[rb_slot].timeout = 1;
	request_buffer[rb_slot].retry_count = 0;
	
	return true; // success!
}

// Only for debugging...
void print_request_queue(void)
{
	uint8_t i, j;
	bool empty = true;
	
	for (i = 0; i < REQUEST_BUFFER_SIZE; i++)
	{
		
		if (request_buffer[i].message_type != MESSAGETYPE_UNUSED)
		{
			UART_PUTF("Request Buffer %u: ", i);
			UART_PUTF4("MessageType %u, PacketCounter %lu, Timeout %u, Retry %u, Data", request_buffer[i].message_type, request_buffer[i].packet_counter, request_buffer[i].timeout, request_buffer[i].retry_count);
			
			// TODO: only show bytes of real data length
			for (j = 0; j < request_buffer[i].data_bytes; j++)
			{
				UART_PUTF(" %02x", request_buffer[i].data[j]);
			}
			
			UART_PUTS("\r\n");
		}
	}

	for (i = 0; i < REQUEST_QUEUE_RECEIVERS; i++)
	{
		if (request_queue[i][0] != SLOT_UNUSED)
		{
			empty = false;
			
			UART_PUTF("Request Queue %u: ", i);
			UART_PUTF("ReceiverID %u, Buffer slots", request_queue[i][0]);
			
			for (j = 0; j < REQUEST_QUEUE_PACKETS; j++)
			{
				if (request_queue[i][j + 1] == SLOT_UNUSED)
				{
					UART_PUTS(" -");
				}
				else
				{
					UART_PUTF(" %u", request_queue[i][j + 1]);
				}
			}
			
			UART_PUTS("\r\n");
		}
	}

	if (empty)
	{
		UART_PUTS("Request Queue empty");
	}

	UART_PUTS("\r\n");
}

// Search for a request to repeat (with timeout reached) and write the data to the send buffer bufx.
// Return a pointer to the request if successful, 0 if no request to repeat was found.
// Automatically decrease timeout counters for all waiting requests in the queue,
// delete a request if it is repeated the last time and cleanup the queue and repeat_buffer accordingly.
// This function has to be called once a second, because the timeout values represent the amount of seconds.
//
// TODO (optimization): Change the behaviour so that a new packet can be sent out of the queue without a delay (currently, we have ~0.5s delay in average).
// So check the queue for "timeout 0" packets more often, but don't reduce the timeout in this case.
request_t * find_request_to_repeat(uint32_t packet_counter)
{
	uint8_t i;
	uint8_t slot;
	request_t * res = 0;

	for (i = 0; i < REQUEST_QUEUE_RECEIVERS; i++)
	{
		if (request_queue[i][0] != SLOT_UNUSED)
		{
			// count down timeout from first element per queue
			slot = request_queue[i][1];
			
			if (request_buffer[slot].timeout > 0)
			{
				request_buffer[slot].timeout--;
			}
			
			// set bufx to the request to retry, if timeout is reached
			if ((request_buffer[slot].timeout == 0) && (res == 0))
			{
				res = &request_buffer[slot];

				// Init header
				memset(&bufx[0], 0, sizeof(bufx));
				pkg_header_set_senderid(0); // FIXME: Use DeviceID instead?!
				pkg_header_set_packetcounter(packet_counter);

				// set message type
				pkg_header_set_messagetype(request_buffer[slot].message_type);

				// set header extension (incl. receiver_id) + data
				memcpy(bufx + 9, request_buffer[slot].data, request_buffer[slot].data_bytes); // header size = 9 bytes
				
				// remember packet counter
				request_buffer[slot].packet_counter = packet_counter;
				
				request_buffer[slot].retry_count++;
				
				if (request_buffer[slot].retry_count > REQUEST_RETRY_COUNT)
				{
					// delete request from queue
					request_buffer[slot].message_type = MESSAGETYPE_UNUSED;

					uint8_t x;
					
					for (x = 1; x < REQUEST_QUEUE_PACKETS; x++)
					{
						request_queue[i][x] = request_queue[i][x + 1];
					}
					
					request_queue[i][REQUEST_QUEUE_PACKETS] = SLOT_UNUSED;
					
					// delete request queue completely (if no requests are in the queue)
					if (request_queue[i][1] == SLOT_UNUSED)
					{
						request_queue[i][0] = SLOT_UNUSED;
					}
				}
				else
				{
					request_buffer[slot].timeout = (request_buffer[slot].retry_count - 1) * REQUEST_ADDITIONAL_TIMEOUT_S + REQUEST_INITIAL_TIMEOUT_S;
				}
			}
		}

		//UART_PUTS("\r\n");
	}
	
	return res;
}

// Assume a request as acknowledged and delete it from the request_buffer and request_queue.
void remove_request(uint16_t sender_id, uint16_t request_sender_id, uint32_t packet_counter)
{
	if (request_sender_id != 0) // if acknowledge is not meant for the base station (which has device id 0)
	{			
		UART_PUTS("Ignoring ack (request not from this device).\r\n");
	}
	else
	{
		uint8_t rq_slot;
		
		for (rq_slot = 0; rq_slot < REQUEST_QUEUE_RECEIVERS; rq_slot++)
		{
			if (request_queue[rq_slot][0] == sender_id)
			{
				// Because we use a fifo queue, the first buffered element has to be the one that is acknowledged.
				// We don't need to check the others.
				uint8_t rb_slot = request_queue[rq_slot][1];
				
				if (request_buffer[rb_slot].packet_counter == packet_counter)
				{
					uint8_t i;
					
					UART_PUTF("Removing request from request buffer slot %u.\r\n", rb_slot);
					
					// remove from request buffer
					request_buffer[rb_slot].message_type = MESSAGETYPE_UNUSED;
					
					// remove from request queue
					for (i = 1; i < REQUEST_QUEUE_PACKETS; i++)
					{
						request_queue[rq_slot][i] = request_queue[rq_slot][i + 1];
					}
					
					request_queue[rq_slot][REQUEST_QUEUE_PACKETS] = SLOT_UNUSED;
					
					// delete request queue entry if no more packets in this queue_request
					if (request_queue[rq_slot][1] == SLOT_UNUSED)
					{
						UART_PUTF("Request Queue %u is now empty.\r\n", rq_slot);
						request_queue[rq_slot][0] = SLOT_UNUSED;
					}
					
					print_request_queue();
				}
				else
				{
					UART_PUTS("Warning: SenderID from ack found in queue, but PacketCounter does not match.\r\n");
				}
				
				return;
			}
		}
		
		// After the last retry, a packet is immediately removed from the queue, and therefore not found if it is acknowledged.
		UART_PUTS("Warning: Acknowledged request not found in queue (could have been the last retry).\r\n");
	}
}
