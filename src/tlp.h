/*
 * tlp.h
 *
 *  Created on: 02. Mai 2016
 *      Author: Peter Kremsner
 */

#ifndef SRC_TLP_H_
#define SRC_TLP_H_

#include <inttypes.h>
#include "../libfifo/c/stc_fifo.h"

#define TLP_MESSAGE_SIZE 31
#define WINDOW_SIZE 8
#define RESEND_COUNTER 8
#define ACK_COUNTER RESEND_COUNTER - 2 //This must be smallern than the RESEND_COUNTER
#define TIMEOUT 10 //Number of retries to deliver a message until the timeoutCallback gets executed

typedef struct
{
	uint8_t data[TLP_MESSAGE_SIZE + 2];
	uint8_t size;
	uint8_t resendCounter;
	uint8_t timeoutCounter;
}tlp_message_t;

typedef char (*tlp_frameRecieved)(tlp_message_t *message);
typedef char (*tlp_frameSend)(uint8_t *buffer, uint8_t size);
typedef char (*tlp_timeOutCallback)(tlp_message_t* message);

typedef struct
{
	struct
	{
		tlp_message_t buffer[WINDOW_SIZE];
		stc_fifo_t fifo;
	}transmit_buffer;
	tlp_frameRecieved callback;
	tlp_frameSend framesendFunction;
	tlp_timeOutCallback timeoutCallback;
	uint8_t data_to_acknowledge;
	uint8_t last_recieved_sequence;
	uint8_t last_transmitted_sequence;
	uint8_t ack_counter;
}tlp_t;

#endif /* SRC_TLP_H_ */
