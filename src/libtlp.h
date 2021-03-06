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
#define WINDOW_SIZE 7
#define RESEND_COUNTER 5
#define ACK_COUNTER RESEND_COUNTER - 2 //This must be smallern than the RESEND_COUNTER
#define TIMEOUT 10 //Number of retries to deliver a message until the timeoutCallback gets executed

#define IMMEDIATE_ACK 0

typedef struct
{
	uint8_t data[TLP_MESSAGE_SIZE + 3];
	uint8_t size;
	uint8_t resendCounter;
	uint8_t timeoutCounter;
}tlp_message_t;

typedef char (*tlp_frameRecieved)(uint8_t *buffer, uint8_t size);
typedef char (*tlp_frameSend)(uint8_t *buffer, uint8_t size);
typedef char (*tlp_timeOutCallback)(tlp_message_t* message);

typedef struct
{
	struct
	{
		tlp_message_t buffer[WINDOW_SIZE + 1];
		stc_fifo_t fifo;
	}transmit_buffer;
	tlp_frameRecieved callback;
	tlp_frameSend framesendFunction;
	tlp_timeOutCallback timeoutCallback;
	uint8_t last_recieved_sequence;
	uint8_t last_transmitted_sequence;
	uint8_t ack_counter;
	struct
	{
		unsigned int send_ack :1;
	}status;
}tlp_t;

typedef union
{
	struct
	{
		unsigned int Ack_Ack :1;
	}bits;
	unsigned char flags;
}flags_t;

void tlp_init(tlp_t *tlp, tlp_frameRecieved recieveCallbackFunction, tlp_frameSend framesendFunction);
uint8_t tlp_recieve(tlp_t *tlp, uint8_t *data, uint8_t size);
uint8_t tlp_send(tlp_t *tlp, uint8_t *data, uint8_t size);
void tlp_tick(tlp_t *tlp);
void tlp_flush(tlp_t *tlp);

#endif /* SRC_TLP_H_ */
