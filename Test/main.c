#include "../src/tlp.h"

tlp_t tlp1, tlp2;

char tlp_timeout(tlp_message_t *message)
{
	return 1;
}

char tlp_send_1(uint8_t *buffer, uint8_t size)
{
	tlp_recieve(&tlp2,buffer,size);
	return 1;
}

char tlp_send_2(uint8_t *buffer, uint8_t size)
{
	tlp_recieve(&tlp1,buffer,size);
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

	return 1;
}

int main (void)
{
	tlp1.timeoutCallback = &tlp_timeout;
	tlp2.timeoutCallback = &tlp_timeout;

	tlp_init(&tlp1,&tlp_recieve_1,&tlp_send_1);
	tlp_init(&tlp2,&tlp_recieve_2,&tlp_send_2);

	unsigned char message[3][5] = {{1,1,1,1,1},{12,2,2,2,2},{3,3,3,3,3}};

	while(1)
	{
		tlp_send(&tlp1,message[0],5);
		tlp_tick(&tlp1);
		tlp_tick(&tlp2);
	}
}
