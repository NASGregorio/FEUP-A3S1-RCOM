#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "man_made_error.h"
#include "defs.h"

int perc_head = 0;
int perc_data = 0;
int tprop_delay = 0;

void init_rand(int ph, int pd, int delay)
{
	perc_head = ph;
	perc_data = pd;
	tprop_delay = delay;

	printf("%d | %d\n", perc_head, perc_data);

	srand(time(NULL));
}

int add_head_error(uint8* frame)
{

	double r = (double)rand()/RAND_MAX;
	if(r < perc_head * 0.01)
	{
		int p = 1+(rand()%3);
		printf("ADD HEAD ERROR: Field %d from %02x", p, frame[p]);
		frame[p] = rand()%255;
		printf(" to %02x | ", frame[p]);
		return BCC_ERROR;
	}
	return OK;
}

int add_data_error(uint8* frame, size_t frame_len) 
{
	double r = (double)rand()/RAND_MAX;
	if(r < perc_data * 0.01)
	{
		int p = FRAME_POS_D+(rand()%(frame_len-FRAME_POS_D-1));
		printf("ADD DATA ERROR: Field %d from %02x", p, frame[p]);
		frame[p] = rand()%255;
		printf(" to %02x | ", frame[p]);
		return BCC2_ERROR;
	}
	return OK;
}

void add_delay()
{
	if(tprop_delay > 0 && tprop_delay < 1000000)
		usleep(tprop_delay);
}
