#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "defs.h"
#include "link_layer.h"

int* llfd;
unsigned retries_count = 0;
unsigned timeout_exit = 0;
unsigned sequenceNumber = 0;

int bytes_written;
int bytes_read;

uint8_t frame[MAX_FRAME_LEN];
size_t frame_len = MAX_FRAME_LEN;

uint8_t frame_SU[FRAME_SU_LEN] = { FLAG, -1, -1, -1, FLAG };
uint8_t frame_RR_REJ[FRAME_SU_LEN] = { FLAG, -1, -1, -1, FLAG };


//////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t BCC2_generator(uint8_t* msg, size_t len)
{
	uint8_t BCC2 = msg[0];
	for (size_t i = 1; i < len; i++)
	{
		BCC2 ^= msg[i];
	}
	return BCC2;
}

int check_frame_bcc(uint8_t* frame, uint8_t* bcc_ret)
{
	uint8_t ret = frame[FRAME_POS_A] ^ frame[FRAME_POS_C];
	if(bcc_ret) *bcc_ret = ret;
	return (frame[FRAME_POS_BCC] == ret) ? OK : BCC_ERROR;
}

int check_frame_bcc2(uint8_t* frame, size_t len, uint8_t* bcc2_ret)
{
	uint8_t ret = BCC2_generator(&frame[FRAME_POS_D], len - FRAME_I_LEN);
	if(*bcc2_ret) *bcc2_ret = ret;
	return (frame[len - FRAME_OFFSET_BCC2] == ret) ? OK : BCC2_ERROR;
}

int check_frame_address(uint8_t* frame, uint8_t expected_addr)
{
	return (frame[FRAME_POS_A] == expected_addr) ? OK : ADR_ERROR;
}

int check_frame_control(uint8_t* frame, uint8_t expected_ctrl)
{
	return (frame[FRAME_POS_C] == expected_ctrl) ? OK : CTR_ERROR;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

void byte_stuffing(uint8_t* frame_start, size_t *len)
{
	size_t extra = 0;
	size_t old_len = *len;

	for (size_t i = FRAME_POS_D; i < old_len-FRAME_OFFSET_BCC2; i++)
		if ( frame_start[i] == FLAG || frame_start[i] == ESCAPE )
			extra++;

	*len += extra;
	uint8_t temp[*len];

	size_t i = FRAME_POS_D, j = 0;
	for (; i < old_len - 1; i++)
	{
		if ( frame_start[i] == FLAG || frame_start[i] == ESCAPE )
		{
			temp[j] = ESCAPE;
			temp[j+1] = frame_start[i] ^ BYTE_XOR;
			j+=2;
		}
		else
		{
			temp[j++] = frame_start[i];
		}
	}

	for (size_t i = 0; i < *len; i++)
	{
		frame_start[i+FRAME_POS_D] = temp[i];
	}
	frame_start[*len-1] = FLAG;

	// temp[*len - 1] = FLAG;
	// memcpy(&frame_start[FRAME_POS_D], temp, *len);
}

void byte_destuffing(uint8_t* frame_start, size_t* len)
{
	size_t extra = 0;
	size_t old_len = *len;

	for (size_t i = FRAME_POS_D; i < old_len-FRAME_OFFSET_BCC2; i++)
		if ( frame_start[i] == ESCAPE && ( (frame_start[i+1] == (FLAG ^ BYTE_XOR) ) || (frame_start[i+1] == (ESCAPE ^ BYTE_XOR) ) ))
			extra++;

	*len -= extra;
	uint8_t temp[*len];

	size_t i = FRAME_POS_D, j = 0;
	for (; i < old_len; i++)
		temp[j++] = (frame_start[i] == ESCAPE) ? (frame_start[(i++) + 1] ^ BYTE_XOR) : frame_start[i];

	//temp[*len - 1] = FLAG;

	for (size_t i = 0; i < *len; i++)
	{
		frame_start[i+FRAME_POS_D] = temp[i];
	}
	frame_start[*len-1] = FLAG;
	

	//memcpy(&frame_start[FRAME_POS_D], temp, *len);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void timeout_handler()
{
	if(retries_count >= MAX_RETRIES)
	{
		timeout_exit = 1;
		alarm(0);
		return;
	}

	// --DEBUG--
	#ifdef DEBUG_BCC_ERROR
	frame[3] = (frame[1] ^ frame[2]); //DEBUG:
	#endif
	#ifdef DEBUG_BCC2_ERROR
	frame[frame_len-2] -= 1;
	#endif
	// --DEBUG--

	printf("Retrying... (%d)\n",++retries_count);
	write_msg(*llfd, frame, frame_len, &bytes_written);
	alarm(TIMEOUT);
}

int return_on_timeout()
{
	if(timeout_exit)
	{
		printf("Connection timeout\n");
		return EXIT_TIMEOUT;
	}
	return OK;
}

int transmitter_set()
{
	DEBUG_PRINT(("--------------------\n"));

	frame[0] = FLAG;
	frame[1] = A_SENDER;
	frame[2] = C_SET;
	frame[3] = (frame[1] ^ frame[2]);
	frame[4] = FLAG;
	frame_len = FRAME_SU_LEN;

	write_msg(*llfd, frame, frame_len, &bytes_written);
	DEBUG_PRINT(("Sent SET\n"));

	// Enable timeout mechanism
	(void) signal(SIGALRM, timeout_handler);
	alarm(TIMEOUT);

	// Wait for UA
	uint8_t msg[255];
	int err = read_msg(*llfd, msg, &bytes_read, 255, return_on_timeout);

	// Exceeded timeout time, exit.
	if(err == EXIT_TIMEOUT)
		return err;

	// Disable timeout mechanism
	alarm(0);

	// Check UA for errors
	err = check_frame_address(msg, A_SENDER);
	if(err != OK)
	{
		DEBUG_PRINT(("Error UA | ADDRESS\n"));
		timeout_handler();
		return err;
	}
	err = check_frame_control(msg, C_UA);
	if(err != OK)
	{
		DEBUG_PRINT(("Error UA | CTRL\n"));
		timeout_handler();
		return err;
	}
	uint8_t bcc;
	err = check_frame_bcc(msg, &bcc);
	if(err != OK)
	{
		DEBUG_PRINT(("Error UA | BCC: %02x\n", bcc));
		timeout_handler();
		return err;
	}

	DEBUG_PRINT(("Got UA\n"));
	printf("Starting transfer...\n");

	return OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

void frame_set_reply()
{
	int err = check_frame_address(frame, A_SENDER);
	if(err != OK)
	{
		DEBUG_PRINT(("Error SET | ADDRESS\n"));
		return;
	}
	err = check_frame_control(frame, C_SET);
	if(err != OK)
	{
		DEBUG_PRINT(("Error SET | CTRL\n"));
		return;
	}
	uint8_t bcc;
	err = check_frame_bcc(frame, &bcc);
	if(err != OK)
	{
		DEBUG_PRINT(("Error SET | BCC: %02x\n", bcc));
		return;
	}

	DEBUG_PRINT(("Got SET\n"));

	frame_SU[1] = A_SENDER;
	frame_SU[2] = C_UA;
	frame_SU[3] = frame_SU[1] ^ frame_SU[2];

	write_msg(*llfd, frame_SU, FRAME_SU_LEN, &bytes_written);
	DEBUG_PRINT(("Sent UA\n"));

	printf("Incoming data...\n");
}

void frame_i_reply()
{
	// Check frame for errors
	DEBUG_PRINT(("Got I: %u\n", sequenceNumber));
	size_t msg_len = frame_len - FRAME_I_LEN;

	#ifdef ENABLE_STUFF
	DEBUG_PRINT(("DESTUFF PRE: %lu\n", frame_len));
	byte_destuffing(frame, &frame_len);
	for (size_t i = 0; i < frame_len; i++)
		DEBUG_PRINT(("%02x", frame[i]));
	DEBUG_PRINT(("\n"));
	DEBUG_PRINT(("DESTUFF POS: %lu\n", frame_len));
	#endif

	// frame_len = FRAME_I_LEN + msg_len;

	uint8_t bcc;
	int err = check_frame_bcc(frame, &bcc);
	if(err != OK)
	{
		DEBUG_PRINT(("Error I %u | BCC: %02x\n", sequenceNumber, bcc));
		return;
	}
	err = check_frame_address(frame, A_SENDER);
	if(err != OK)
	{
		DEBUG_PRINT(("Error I %u | ADDRESS\n", sequenceNumber));
		return;
	}

	uint8_t bcc2;
	err = check_frame_bcc2(frame, frame_len, &bcc2);
	DEBUG_PRINT(("BCC2 calc %02x\n", bcc2));
	DEBUG_PRINT(("BCC2 recv %02x\n", frame[frame_len - FRAME_OFFSET_BCC2]));
	if(err == BCC2_ERROR)
	{
		DEBUG_PRINT(("Error I: %u | BCC2: %02x\n", sequenceNumber, bcc2));
		if(check_frame_control(frame, (sequenceNumber == 1 ? C_I_0 : C_I_1) == OK))
		{
			// SEND REJ
			frame_RR_REJ[1] = A_SENDER;
			frame_RR_REJ[2] = (sequenceNumber == 1 ? C_REJ_0 : C_REJ_1);
			frame_RR_REJ[3] = frame_RR_REJ[1] ^ frame_RR_REJ[2];

			write_msg(*llfd, frame_RR_REJ, FRAME_SU_LEN, &bytes_written);
			DEBUG_PRINT(("Sent REJ: %u\n", sequenceNumber));
		}

		else
		{
			frame_RR_REJ[1] = A_SENDER;
			frame_RR_REJ[2] = (sequenceNumber = 1 ? C_RR_1 : C_RR_0);
			frame_RR_REJ[3] = frame_RR_REJ[1] ^ frame_RR_REJ[2];

			sequenceNumber = !sequenceNumber;

			write_msg(*llfd, frame_RR_REJ, FRAME_SU_LEN, &bytes_written);
			DEBUG_PRINT(("Sent RR: %u\n", sequenceNumber));
		}
	}
	else if(err == OK)
	{
		frame_RR_REJ[1] = A_SENDER;
    	frame_RR_REJ[2] = (sequenceNumber ? C_RR_1 : C_RR_0);
    	frame_RR_REJ[3] = frame_RR_REJ[1] ^ frame_RR_REJ[2];

		if(check_frame_control(frame, (sequenceNumber = 1 ? C_I_0 : C_I_1) == OK))
		{
			// TODO: Connection point to application layer
			// Print frame data
			for (size_t i = FRAME_POS_D; i < msg_len; i++)
				printf("%c",(char)(frame[i]));
			printf("\n");
		}
		sequenceNumber = !sequenceNumber;

		write_msg(*llfd, frame_RR_REJ, FRAME_SU_LEN, &bytes_written);
		DEBUG_PRINT(("Sent RR: %u\n", sequenceNumber));
	}
}

int frame_disc_reply()
{
	int err = check_frame_address(frame, A_SENDER);
	if(err != OK)
	{
		DEBUG_PRINT(("Error DISC | ADDRESS\n"));
		return err;
	}
	err = check_frame_control(frame, C_DISC);
	if(err != OK)
	{
		DEBUG_PRINT(("Error DISC | CTRL\n"));
		return err;
	}
	uint8_t bcc;
	err = check_frame_bcc(frame, &bcc);
	if(err != OK)
	{
		DEBUG_PRINT(("Error DISC | BCC: %02x\n", bcc));
		return err;
	}

	DEBUG_PRINT(("Got DISC\n"));

	frame_SU[1] = A_RECEIVER;
	frame_SU[2] = C_DISC;
	frame_SU[3] = frame_SU[1] ^ frame_SU[2];

	write_msg(*llfd, frame_SU, FRAME_SU_LEN, &bytes_written);
	DEBUG_PRINT(("Sent DISC\n"));


	while(1)
	{
		uint8_t msg[255];
		read_msg(*llfd, msg, &bytes_read, 255, NULL);

		err = check_frame_bcc(msg, &bcc);
		if(err != OK)
		{
			DEBUG_PRINT(("Error DISC | BCC: %02x\n", bcc));
			return err;
		}
		

		// If TRANSMITTER DISC - repeat RECEIVER DISC
		if(check_frame_address(msg, A_SENDER) == OK && check_frame_control(msg, C_DISC) == OK)
		{
			write_msg(*llfd, frame_SU, FRAME_SU_LEN, &bytes_written);
			DEBUG_PRINT(("Resent DISC\n"));
		}
		// If UA - disconnect
		else if(check_frame_address(msg, A_RECEIVER) == OK && check_frame_control(msg, C_UA) == OK)
		{
			DEBUG_PRINT(("Got UA\n"));
			return DISC_CONN;
		}
		// If anything else, loop back
		else
		{
			DEBUG_PRINT(("Waiting for TRANSMITTER disconnect acknowledgement...\n"));
		}
		

		// if(!check_frame_errors(msg, FRAME_SU_LEN, A_SENDER, C_DISC, 1, 0))
		// {
		// }
		// else if (!check_frame_errors(msg, FRAME_SU_LEN, A_RECEIVER, C_UA, 1, 0))
		// {
		// 	DEBUG_PRINT(("Got UA\n"));
		// 	return DISC_CONN;
		// }
		// else
		// {
		// 	DEBUG_PRINT(("Error UA\n"));
		// }

	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

int llopen(int port, COM_MODE mode, int* fd, TERMIOS* oldtio)
{
	if( fcntl(*fd, F_GETFD) == -1 && errno == EBADF )
	{
		srand(time(NULL));

		int err = open_port(port, fd);
		if(err != OK)
			return err;

		TERMIOS newtio;
		err = set_port_attr(*fd, oldtio, &newtio);
		if(err != OK)
			return err;

		llfd = fd;

		printf("Connection open\n");
	}

	// 0 for TRANSMITTER | 1 for RECEIVER
	sequenceNumber = (mode == RECEIVER);

	if(mode == TRANSMITTER)
		return transmitter_set();
	else if(mode != RECEIVER)
		return BAD_ARGS;



	return OK;
}

int llclose(TERMIOS* oldtio, COM_MODE mode)
{
	DEBUG_PRINT(("--------------------\n"));

	if(mode == TRANSMITTER)
	{
		// prepare DISC frame
		frame[1] = A_SENDER;
		frame[2] = C_DISC;
		frame[3] = (frame[1] ^ frame[2]);
		frame[4] = FLAG;
		frame_len = FRAME_SU_LEN;

		// send DISC frame
		write_msg(*llfd, frame, frame_len, &bytes_written);
		DEBUG_PRINT(("Sent DISC\n"));

		// Enable timeout mechanism
		(void) signal(SIGALRM, timeout_handler);
		alarm(TIMEOUT);

		// Wait for DISC
		uint8_t msg[255];
		int err;

		while ( 1 )
		{
			err = read_msg(*llfd, msg, &bytes_read, 255, return_on_timeout);

			// Exceeded timeout time, exit.
			if(err == EXIT_TIMEOUT)
				return err;

			// Disable timeout mechanism
			alarm(0);

			//TODO: Proper DISCONNECT		
			
			err = check_frame_address(msg, A_RECEIVER);
			if(err != OK)
			{
				DEBUG_PRINT(("Error DISC | ADDRESS\n"));
				timeout_handler();
				continue;
			}
			err = check_frame_control(msg, C_DISC);
			if(err != OK)
			{
				DEBUG_PRINT(("Error DISC | CTRL\n"));
				timeout_handler();
				continue;
			}
			uint8_t bcc;
			err = check_frame_bcc(msg, &bcc);
			if(err != OK)
			{
				DEBUG_PRINT(("Error DISC | BCC: %02x\n", bcc));
				timeout_handler();
				continue;
			}

			DEBUG_PRINT(("Got DISC\n"));


			// prepare UA frame
			frame[1] = A_RECEIVER;
			frame[2] = C_UA;
			frame[3] = (frame[1] ^ frame[2]);
			frame[4] = FLAG;
			frame_len = FRAME_SU_LEN;

			// send UA frame
			write_msg(*llfd, frame, frame_len, &bytes_written);
			DEBUG_PRINT(("Sent UA\n"));
			DEBUG_PRINT(("--------------------\n"));
			break;
		}
	}

	printf("Transfer complete\n");

	//cleanup
	DEBUG_PRINT(("--------------------\n"));
    restore_port_attr(*llfd, oldtio);
	close_port(*llfd);

    printf("Connection closed\n");

    return OK;
}

int llwrite(uint8_t* buf, size_t len)
{
	DEBUG_PRINT(("--------------------\n"));

	frame_len = FRAME_I_LEN + len;

	frame[0] = FLAG;
	frame[1] = A_SENDER;
	frame[2] = (sequenceNumber == 1 ? C_I_1 : C_I_0);
	frame[3] = (frame[1] ^ frame[2]);

	for (size_t i = 0; i < len; i++)
		frame[i+4] = buf[i];

	frame[frame_len - FRAME_OFFSET_BCC2] = BCC2_generator(buf, len);
	frame[frame_len - 1] = FLAG;


	#ifdef DEBUG_BCC_ERROR
	double r = (double)rand()/RAND_MAX;
	if(r < 0.3)
	{
		printf("DEBUG BCC ERROR\n");
		frame[3] = 0;
	}
	#endif


	#ifdef DEBUG_BCC2_ERROR
	double r2 = (double)rand()/RAND_MAX;
	if(r2 < 0.3)
	{
		printf("DEBUG ERROR BCC2 %02x\n", frame[frame_len - FRAME_OFFSET_BCC2]);
		frame[frame_len - FRAME_OFFSET_BCC2] += 1;
	}
	#endif


	#ifdef ENABLE_STUFF
	DEBUG_PRINT(("STUFF PRE: %lu\n", frame_len));
	byte_stuffing(frame, &frame_len);
	for (size_t i = 0; i < frame_len; i++)
		DEBUG_PRINT(("%02x", frame[i]));
	DEBUG_PRINT(("\n"));
	DEBUG_PRINT(("STUFF POS: %lu\n", frame_len));
	#endif

	// frame[frame_len - FRAME_OFFSET_BCC2] = bcc2;
	// frame[frame_len - 1] = FLAG;

	write_msg(*llfd, frame, frame_len, &bytes_written);
	DEBUG_PRINT(("Sent I: %u\n", sequenceNumber));


	#ifdef DEBUG_TTY_CALLS
	for (size_t i = 4; i < frame_len-2; i++)
		printf("%c",(char)(frame[i]));
	printf("\n");
	#endif

	// Enable timeout mechanism
	(void) signal(SIGALRM, timeout_handler);
	alarm(TIMEOUT);

	while(1)
	{
		// Wait for UA
		uint8_t msg[255];
		int err = read_msg(*llfd, msg, &bytes_read, 255, return_on_timeout);

		// Exceeded timeout time, exit.
		if(err == EXIT_TIMEOUT)
			return err;

		// Disable timeout mechanism
		alarm(0);

		uint8_t bcc;
		err = check_frame_bcc(msg, &bcc);
		if(err != OK)
		{
			DEBUG_PRINT(("Error RR | BCC: %02x\n", bcc));
			timeout_handler();
		}

		err = check_frame_address(msg, A_SENDER);
		if(err != OK)
		{
			DEBUG_PRINT(("Error RR | ADDRESS\n"));
			timeout_handler();
		}

		if(check_frame_control(msg, (sequenceNumber == 0 ? C_RR_1 : C_RR_0)) == OK)
		{
			// Update sequence number
			sequenceNumber = !sequenceNumber;
			DEBUG_PRINT(("Got RR: %u\n", sequenceNumber));
			return OK;
		}
		else if(check_frame_control(msg, (sequenceNumber == 0 ? C_REJ_0 : C_REJ_1)) == OK)
		{
			#ifdef DEBUG_BCC2_ERROR
			frame[frame_len-FRAME_OFFSET_BCC2] -= 1;
			#endif

			DEBUG_PRINT(("Got REJ: %u\n", sequenceNumber));
			write_msg(*llfd, frame, frame_len, &bytes_written);
			alarm(TIMEOUT);
		}
		else
		{
			DEBUG_PRINT(("Error RR | CTRL\n"));
			timeout_handler();
		}

		// // Check I for errors
		// if(!check_frame_errors(msg, FRAME_SU_LEN, A_SENDER, (sequenceNumber ? C_RR_0 : C_RR_1), 1, 0))
		// {
		// }
		// else if (!check_frame_errors(msg, FRAME_SU_LEN, A_SENDER, (sequenceNumber ? C_REJ_0 : C_REJ_1), 1, 0))
		// {
		// 	DEBUG_PRINT(("Got REJ: %u\n", sequenceNumber));
		// 	frame[3] = (frame[1] ^ frame[2]); //DEBUG:
		// 	write_msg(*llfd, frame, frame_len, &bytes_written);
		// 	alarm(TIMEOUT);
		// }
		// else
		// {
		// 	DEBUG_PRINT(("Error RR\n"));
		// 	timeout_handler();
		// }
	}

	return OK;
}

int llread()
{
	DEBUG_PRINT(("--------------------\n"));

    // Wait for frame
	if(read_msg(*llfd, frame, &bytes_read, 255, NULL) != OK)
        return -1;

	frame_len = bytes_read;

    uint8_t ctrl_field = frame[2];
	//DEBUG_PRINT(("C: %u\n",ctrl_field));


    switch (ctrl_field)
    {
    case C_SET:
        frame_set_reply();
        break;
    case C_I_0:
    case C_I_1:
        frame_i_reply();
        break;
    case C_DISC:
		return frame_disc_reply();
        break;
    default:
        break;
    }

    return OK;
}