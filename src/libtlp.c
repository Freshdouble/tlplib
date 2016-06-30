/*
 * tlp.c
 *
 *  Created on: 02. Mai 2016
 *      Author: Peter Kremsner
 */

#include "libtlp.h"
#include <stdlib.h>
#include <limits.h>

void tlp_init(tlp_t *tlp, tlp_frameRecieved recieveCallbackFunction,
              tlp_frameSend framesendFunction)
{
    tlp->last_transmitted_sequence = 0;
    tlp->last_recieved_sequence = 0;
    tlp->ack_counter = ACK_COUNTER;
    tlp->callback = recieveCallbackFunction;
    tlp->framesendFunction = framesendFunction;
    tlp->status.send_ack = 1;
    fifo_init(&tlp->transmit_buffer.fifo, tlp->transmit_buffer.buffer,
              WINDOW_SIZE + 1, sizeof(tlp_message_t));
}

void tlp_send_ack_ack(tlp_t* tlp)
{
    uint8_t message[3];
    flags_t flag;
    flag.bits.Ack_Ack = 1;
    if (tlp->last_transmitted_sequence)
    {
        if (tlp->last_transmitted_sequence)
            message[0] = tlp->last_transmitted_sequence; //Pick a valid sequence number
        else
            message[0] = 1;
        message[1] = tlp->last_transmitted_sequence;
        message[2] = flag.flags; //flags
        tlp->framesendFunction(message, 3);
    }
}

void tlp_recv_ack(uint8_t number, tlp_t *tlp)
{
    int i;
    tlp_message_t* message;
    if (number != 0) //If ACK is 0 it isn't valid ACK Field
    {
        for (i = 0; i < WINDOW_SIZE; i++)
        {
            message = fifo_get_nth_Object(i, &tlp->transmit_buffer.fifo);
            if (!message)
                break;

            if (message->data[0] == number)
            {
                fifo_delete_n_Objects(i + 1, &tlp->transmit_buffer.fifo);
                break;
            }
        }

        if(fifo_empty(&tlp->transmit_buffer.fifo))
        {
            tlp_send_ack_ack(tlp);
        }
    }
}

void tlp_send_ack(tlp_t *tlp)
{
    uint8_t message[3];
    if (tlp->last_recieved_sequence && tlp->status.send_ack)
    {
        if (tlp->last_transmitted_sequence)
            message[0] = tlp->last_transmitted_sequence; //Pick a valid sequence number
        else
            message[0] = 1;
        message[1] = tlp->last_recieved_sequence;
        message[2] = 0x00; //flags
        if (tlp->framesendFunction(message, 3)) // Dont save this message, just transmit it
        {
            tlp->ack_counter = ACK_COUNTER;
        }
    }
    else
    {
        tlp->ack_counter = ACK_COUNTER;
    }
}

uint8_t tlp_recieve(tlp_t *tlp, uint8_t *data, uint8_t size)
{
    if (size < 2)
        return 0;

    uint32_t upper;
    uint32_t lower;
    uint32_t current = data[0] + UCHAR_MAX;
    flags_t flags;
    flags.flags = data[2];

    if (tlp->last_recieved_sequence == 255)
    {
        upper = 1 + UCHAR_MAX;
        lower = UCHAR_MAX - (WINDOW_SIZE - 1);
    }
    else
    {
        upper = tlp->last_recieved_sequence + 1 + UCHAR_MAX;
        if (tlp->last_recieved_sequence + WINDOW_SIZE <= UCHAR_MAX)
            lower = tlp->last_recieved_sequence + UCHAR_MAX - WINDOW_SIZE;
        else
            lower = tlp->last_recieved_sequence + UCHAR_MAX - (WINDOW_SIZE - 1);
    }

    if (upper >= current && lower <= current)
    {
        if (flags.bits.Ack_Ack && data[0] == tlp->last_recieved_sequence)
        {
            if (tlp->last_recieved_sequence == data[1])
            {
                tlp->status.send_ack = 0;
            }
        }
        tlp_recv_ack(data[1], tlp);
        if (size != 3 && current == upper) // If size == 3 recieved a simple ack Message
        {
            tlp->last_recieved_sequence = data[0]; //Update recieve Order and Ack Byte
            tlp->status.send_ack = 1;

#if IMMEDIATE_ACK != 0
            tlp_send_ack(tlp);
#endif

            return tlp->callback(&data[3], size - 3);
        }
        return 1;
    }
    return 0;
}

uint8_t tlp_send(tlp_t *tlp, uint8_t *data, uint8_t size)
{
    tlp_message_t message;
    uint8_t increment_sequence;
    int i;

    if (size > TLP_MESSAGE_SIZE)
        return 0;

    increment_sequence = tlp->last_transmitted_sequence + 1;
    if (increment_sequence == 0)
        increment_sequence = 1;

    message.data[0] = increment_sequence;
    message.data[1] = tlp->last_recieved_sequence;
    message.data[2] = 0x00; //Flags

    for (i = 0; i < size; i++)
    {
        message.data[i + 3] = data[i];
    }

    message.size = size + 3;
    message.resendCounter = RESEND_COUNTER;
    message.timeoutCounter = TIMEOUT;

    if (fifo_write_object(&message, &tlp->transmit_buffer.fifo))
    {
        if (tlp->framesendFunction(message.data, message.size))
        {
            tlp->ack_counter = ACK_COUNTER;
            tlp->last_transmitted_sequence = increment_sequence;
            return size;
        }
    }

    return 0;
}

void tlp_tick(tlp_t *tlp)
{
    uint16_t fifo_data;
    tlp_message_t* message;

    if (tlp->ack_counter == 0)
    {
        tlp_send_ack(tlp);
    }
    else
    {
        tlp->ack_counter--;
    }

    fifo_data = fifo_datasize(&tlp->transmit_buffer.fifo);

    while (fifo_data > 0)
    {
        message = fifo_get_nth_Object(fifo_data - 1,
                                      &tlp->transmit_buffer.fifo);
        if(message)
        {
            if (message->resendCounter == 0)
            {
                if (tlp->framesendFunction(message->data, message->size))
                {
                    message->resendCounter = RESEND_COUNTER;
                }
            }
            else
            {
                message->resendCounter--;
            }

            if (message->timeoutCounter == 0)
            {
                if (tlp->timeoutCallback)
                    tlp->timeoutCallback(message);
                message->timeoutCounter = TIMEOUT;
            }
            else
            {
                message->timeoutCounter--;
            }
        }
        fifo_data--;
    }
}

void tlp_flush(tlp_t *tlp)
{
    int i;
    uint16_t fifo_data;
    tlp_message_t* message;

    fifo_data = fifo_datasize(&tlp->transmit_buffer.fifo);
    if (fifo_data)
    {
        message = fifo_get_nth_Object(fifo_data - 1,
                                      &tlp->transmit_buffer.fifo);
        tlp->last_transmitted_sequence = message->data[0]; //Reset last_transmitted_sequence to the smallest sequencenumber in the buffer
        fifo_clear(&tlp->transmit_buffer.fifo);
    }
}

