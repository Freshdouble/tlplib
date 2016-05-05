/*
 * tlp.c
 *
 *  Created on: 02. Mai 2016
 *      Author: Peter Kremsner
 */


#include "tlp.h"

void tlp_init(tlp_t *tlp, tlp_frameRecieved recieveCallbackFunction, tlp_frameSend framesendFunction)
{
	tlp->data_to_acknowledge = 0;
	tlp->last_transmitted_sequence = 0;
	tlp->last_recieved_sequence = 0;
	tlp->ack_counter = ACK_COUNTER;
	tlp->callback = recieveCallbackFunction;
	tlp->framesendFunction = framesendFunction;
	fifo_init(&tlp->transmit_buffer.fifo,tlp->transmit_buffer.buffer,
			WINDOW_SIZE,sizeof(tlp_message_t));
}

void tlp_recv_ack(uint8_t number, tlp_t *tlp)
{
	int i;
	tlp_message_t* message;
	if(number != 0) //If ACK is 0 it isn't valid ACK Field
	{
		for(i = 0; i < WINDOW_SIZE; i++)
		{
			message = fifo_get_nth_Object(i,&tlp->transmit_buffer.fifo);
			if(message->data[0] == number)
			{
				fifo_delete_n_Objects(i,&tlp->transmit_buffer.fifo);
				if(fifo_empty(&tlp->transmit_buffer.fifo))
					tlp->data_to_acknowledge = 0;
				break;
			}
		}
	}
}

uint8_t tlp_recieve(tlp_t *tlp, uint8_t *data, uint8_t size)
{
	uint8_t i;
	tlp_message_t message;

	if(size < 2)
		return 0;

	if((tlp->last_recieved_sequence + 1) > data[0] || (tlp->last_recieved_sequence - WINDOW_SIZE) > data[0]) //Check if frame is in the right order
		return 0;

	tlp_recv_ack(data[1], tlp);
	if(size != 2) // If size == 2 recieved a simple ack Message
	{
		tlp->last_recieved_sequence = data[0]; //Update recieve Order

		if(data[1] != 0) //If ACK is 0 it isn't valid ACK Field
		{
			if(data[1] < tlp->last_recieved_sequence)
				//Acknowledge with last recieved sequence number
				tlp->data_to_acknowledge = tlp->last_recieved_sequence;
			else
				//Acknowledge with current sequence number
				tlp->data_to_acknowledge = data[0];    //Ack byte
		}

		for(i = 2; i < size; i++)
		{
			message.data[i] = data[i];
		}
		message.size = size - 2;

		return tlp->callback(&message);
	}
	return 1;
}

uint8_t tlp_send(tlp_t *tlp, uint8_t *data, uint8_t size)
{
	tlp_message_t message;
	uint8_t increment_sequence;
	int i;

	if(size > TLP_MESSAGE_SIZE)
		return 0;

	increment_sequence = tlp->last_transmitted_sequence + 1;
	if(increment_sequence == 0)
		increment_sequence = 1;

	message.data[0] = increment_sequence;
	message.data[1] = tlp->data_to_acknowledge;

	for(i = 0; i < size; i++)
	{
		message.data[i + 2] = data[i];
	}

	message.size = size + 2;
	message.resendCounter = RESEND_COUNTER;
	message.timeoutCounter = TIMEOUT;

	if(fifo_write_object(&message,&tlp->transmit_buffer.fifo))
	{
		if(tlp->framesendFunction(message.data,message.size))
		{
			tlp->ack_counter = ACK_COUNTER;
			tlp->last_transmitted_sequence = increment_sequence;
			return size;
		}
	}

	return 0;
}

void tlp_send_ack(tlp_t *tlp)
{
	uint8_t message[2];
	if(tlp->data_to_acknowledge)
	{
		message[0] = tlp->last_transmitted_sequence; //Pick a valid sequence number
		message[1] = tlp->data_to_acknowledge;
		if(tlp->framesendFunction(message,2)) // Dont save this message, just transmit it
		{
			tlp->ack_counter = ACK_COUNTER;
		}
	}
	else
	{
		tlp->ack_counter = ACK_COUNTER;
	}
}

void tlp_tick(tlp_t *tlp)
{
	uint16_t fifo_data;
	tlp_message_t* message;

	if(tlp->ack_counter == 0)
	{
		tlp_send_ack(tlp);
	}
	else
	{
		tlp->ack_counter--;
	}

	fifo_data = fifo_datasize(&tlp->transmit_buffer.fifo);

	while(fifo_data > 0)
	{
		message = fifo_get_nth_Object(fifo_data - 1,&tlp->transmit_buffer.fifo);
		if(message->resendCounter == 0)
		{
			if(tlp->framesendFunction(message->data,message->size))
			{
				message->resendCounter = RESEND_COUNTER;
			}
			else
			{
				if(message->timeoutCounter == 0)
				{
					if(tlp->timeoutCallback(message))
						tlp->timeoutCallback(message);
					message->timeoutCounter = TIMEOUT;
				}
				else
				{
					message->timeoutCounter--;
				}
			}
		}
		else
		{
			message->resendCounter--;
		}
		fifo_data--;
	}
}

