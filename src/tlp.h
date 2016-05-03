/*
 * tlp.h
 *
 *  Created on: 02. Mai 2016
 *      Author: Peter Kremsner
 */

#ifndef SRC_TLP_H_
#define SRC_TLP_H_

#include <inttypes.h>

#define TLP_MESSAGE_SIZE 31
#define WINDOW_SIZE 8
#define RESEND_COUNTER 8

typedef struct
{
	uint8_t data[TLP_MESSAGE_SIZE + 2];
	uint8_t size;
	uint8_t sequenceNumber;
	uint8_t resendCounter;
}tlp_message_t;

typedef char (*tlp_frameRecieved)(tlp_message_t *message);
typedef char (*tlp_frameSend)(uint8_t *buffer, uint8_t size);

typedef struct
{
	struct
	{
		tlp_message_t buffer[WINDOW_SIZE];
	}transmit_buffer;
	tlp_frameRecieved callback;
	tlp_frameSend framesendFunction;
	uint8_t data_to_acknowledge;
	uint8_t last_recieved_sequence;
	uint8_t last_transmitted_sequence;
}tlp_t;

#endif /* SRC_TLP_H_ */
