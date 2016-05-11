#include "../src/tlp.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define FAULT_VAL 0.5

tlp_t tlp1, tlp2;
static int rec_count;
static int faultCounter;

char tlp_timeout(tlp_message_t *message)
{
	return 1;
}

char tlp_send_1(uint8_t *buffer, uint8_t size)
{
	if(rand() < FAULT_VAL * RAND_MAX)
	{
		faultCounter++;
	}
	else
	{
		tlp_recieve(&tlp2,buffer,size);
	}
	return 1;
}

char tlp_send_2(uint8_t *buffer, uint8_t size)
{
	if(!(rand() < FAULT_VAL * RAND_MAX))
	{
		tlp_recieve(&tlp1,buffer,size);
	}
	return 1;
}

char tlp_recieve_1(uint8_t *buffer, uint8_t size)
{
	uint8_t data[5];
		int i;

		for(i = 0; i < size; i++)
		{
			data[i] = buffer[i];
		}

	return 1;
}

char tlp_recieve_2(uint8_t *buffer, uint8_t size)
{
	uint8_t data[5];
	int i;

	for(i = 0; i < size; i++)
	{
		data[i] = buffer[i];
	}
	rec_count++;
	return 1;
}

int main (void)
{
	int i;
	int u;
	time_t t;
	tlp1.timeoutCallback = &tlp_timeout;
	tlp2.timeoutCallback = &tlp_timeout;

	tlp_init(&tlp1,&tlp_recieve_1,&tlp_send_1);
	tlp_init(&tlp2,&tlp_recieve_2,&tlp_send_2);

	unsigned char message[3][5] = {{1,1,1,1,1},{12,2,2,2,2},{3,3,3,3,3}};

	srand((unsigned) time(&t));

	for(i = 0; i < 20000; i++)
	{
		if(!tlp_send(&tlp1,message[0],5))
		{
			tlp_flush(&tlp1);
		}
		for(u = 0; u < 100; u++)
		{
			tlp_tick(&tlp1);
			tlp_tick(&tlp2);
		}
	}

	while(1)
	{
		tlp_send(&tlp1,message[0],5);
	}
}
