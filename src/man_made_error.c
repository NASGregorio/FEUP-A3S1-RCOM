#include <stdio.h>
#include <stdlib.h>

#include "man_made_error.h"
#include "defs.h"

/*
* Error in bcc:
* Occurs with a 10% chance and a maximum of 2? times per frame/file??
*/

/*
* Error in bcc2:
* Occurs with a 10% chance and a maximum of 2? times per frame
*/

#define BCC_PERC (0.9)
#define BCC2_PERC (0.9)

int bcc_has_error = 0;
int bcc2_has_error = 0;

void add_bcc_error(uint8* frame){

	if(bcc_has_error)
		return;

	double r = (double)rand()/RAND_MAX;
	if(r < BCC_PERC)
	{
		printf("ADD BCC ERROR: %02x", frame[FRAME_POS_BCC]);
		frame[FRAME_POS_BCC] += 1;
		printf(" to %02x\n", frame[FRAME_POS_BCC]);
		bcc_has_error = 1;
	}

}

void add_bcc2_error(uint8* frame, size_t frame_len){

	if(bcc2_has_error)
		return;

	double r2 = (double)rand()/RAND_MAX;
	if(r2 < BCC2_PERC)
	{
		printf("ADD BCC2 ERROR: %02x", frame[frame_len - FRAME_OFFSET_BCC2]);
		frame[frame_len - FRAME_OFFSET_BCC2] += 1;
		printf(" to %02x\n", frame[frame_len - FRAME_OFFSET_BCC2]);
		bcc2_has_error = 1;
	}

}

void rm_bcc_error(uint8* frame){
	if(bcc_has_error){
		// if statement in order to only debug certain frames
		printf("REMOVE BCC ERROR: %02x", frame[FRAME_POS_BCC]);
		frame[FRAME_POS_BCC] = (frame[FRAME_POS_A] ^ frame[FRAME_POS_C]);
		printf(" to %02x\n", frame[FRAME_POS_BCC]);
		bcc_has_error = 0;
	}
}

void rm_bcc2_error(uint8*frame, size_t frame_len){
	if(bcc2_has_error){
		// if statement in order to only debug certain frames
		printf("REMOVE BCC2 ERROR: %02x", frame[frame_len - FRAME_OFFSET_BCC2]);
		frame[frame_len-FRAME_OFFSET_BCC2] -= 1;
		printf(" to %02x\n", frame[frame_len - FRAME_OFFSET_BCC2]);
		bcc2_has_error = 0;
	}
}
