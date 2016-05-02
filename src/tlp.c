/*
 * tlp.c
 *
 *  Created on: 02. Mai 2016
 *      Author: Peter Kremsner
 */


#include "tlp.h"

void tlp_init(tlp_t *tlp, tlp_frameRecieved recieveCallbackFunction)
{
	tlp->data_to_acknowledge = 0;
	tlp->last_transmitted_sequence = 0;
	tlp->callback = recieveCallbackFunction;
}

void tlp_ack(uint8_t number)
{
	if(number != 0)
	{

	}
}

uint8_t tlp_recieve(tlp_t *tlp, uint8_t *data, uint8_t size)
{
	uint8_t i;
	tlp_message_t message;

	if(size < 2)
		return 0;

	tlp->data_to_acknowledge = data[0];
	tlp_ack(data[1]);

	for(i = 2; i < size; i++)
	{
		message.data[i] = data[i];
	}
	message.size = size - 2;

	return tlp->callback(&message);
}

uint8_t tlp_send(tlp_t *tlp, uint8_t *data, uint8_t size)
{
	tlp_message_t message;
	int i;

	if(size > TLP_MESSAGE_SIZE)
		return 0;

	message.data[0] = tlp->last_transmitted_sequence = tlp->last_transmitted_sequence + 1;
	message.data[1] = tlp->data_to_acknowledge;

	for(i = 0; i < size; i++)
	{
		message.data[i + 2] = data[i];
	}

	message.size = size + 2;

	return 0;
}
