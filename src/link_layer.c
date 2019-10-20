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

uint8_t frame[255];
size_t frame_len = 255;

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

int check_frame_errors(uint8_t* msg, size_t len, uint8_t expected_addr, uint8_t expected_ctrl, int check_bcc, int check_bcc2)
{
	// if(msg[1] != expected_addr)
	// 	printf("ADDRESS\n");
	// if(msg[2] != expected_ctrl)
	// 	printf("CONTROL\n");
	// if((check_bcc && msg[3] != (msg[1] ^ msg[2])))
	// 	printf("BCC\n");
	// if((check_bcc2 && msg[len - 2] != BCC2_generator(&frame[4], len - FRAME_SU_LEN - 1)))
	// 	printf("BCC2\n");
		
	return
		(	msg[1] != expected_addr ||	// Wrong address field
			msg[2] != expected_ctrl ||  // Wrong control field
			(check_bcc && msg[3] != (msg[1] ^ msg[2])) ||
			(check_bcc2 && msg[len - 2] != BCC2_generator(&frame[4], len - FRAME_SU_LEN - 1))
		);
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

//////////////////////////////////////////////////////////////////////////////////////////////////////

void frame_set_reply()
{
	if(check_frame_errors(frame, FRAME_SU_LEN, A_SENDER, C_SET, 1, 0))
	{
		DEBUG_PRINT(("Error SET\n"));
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
	if(check_frame_errors(frame, frame_len, A_SENDER, (sequenceNumber ? C_I_0 : C_I_1), 1, 1))
	{
		DEBUG_PRINT(("Error I: %u\n", sequenceNumber));

		// SEND REJ
		frame_RR_REJ[1] = A_SENDER;
        frame_RR_REJ[2] = (sequenceNumber ? C_REJ_1 : C_REJ_0);
    	frame_RR_REJ[3] = frame_RR_REJ[1] ^ frame_RR_REJ[2];

		write_msg(*llfd, frame_RR_REJ, FRAME_SU_LEN, &bytes_written);
		DEBUG_PRINT(("Sent REJ: %u\n", sequenceNumber));
	}
	else
	{
		DEBUG_PRINT(("Got I: %u\n", sequenceNumber));
		frame_RR_REJ[1] = A_SENDER;
    	frame_RR_REJ[2] = (sequenceNumber ? C_RR_1 : C_RR_0);
    	frame_RR_REJ[3] = frame_RR_REJ[1] ^ frame_RR_REJ[2];

		sequenceNumber = !sequenceNumber;

		// Print frame data
		for (size_t i = 4; i < frame_len-2; i++)
			printf("%c",(char)(frame[i]));
		printf("\n");

		write_msg(*llfd, frame_RR_REJ, FRAME_SU_LEN, &bytes_written);
		DEBUG_PRINT(("Sent RR: %u\n", sequenceNumber));
	}
}

int frame_disc_reply()
{
	if(check_frame_errors(frame, FRAME_SU_LEN, A_SENDER, C_DISC, 1, 0))
	{
		DEBUG_PRINT(("Error DISC\n"));
		return BCC_ERROR;
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

		// If TRANSMITTER DISC - repeat RECEIVER DISC 
		if(!check_frame_errors(msg, FRAME_SU_LEN, A_SENDER, C_DISC, 1, 0))
		{
			write_msg(*llfd, frame_SU, FRAME_SU_LEN, &bytes_written);
			DEBUG_PRINT(("Sent DISC\n"));
		}
		// If UA - disconnect
		else if (!check_frame_errors(msg, FRAME_SU_LEN, A_RECEIVER, C_UA, 1, 0))
		{
			DEBUG_PRINT(("Got UA\n"));
			return DISC_CONN;
		}
		// If anything else - exit
		else
		{
			DEBUG_PRINT(("Error UA\n"));
		}
		
	}
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
	if(check_frame_errors(msg, FRAME_SU_LEN, A_SENDER, C_UA, 1, 0))
	{
		DEBUG_PRINT(("Error SET\n"));
		timeout_handler();
	}

	DEBUG_PRINT(("Got UA\n"));
	printf("Starting transfer...\n");

	return OK;
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
		int err = read_msg(*llfd, msg, &bytes_read, 255, return_on_timeout);

		// Exceeded timeout time, exit.
		if(err == EXIT_TIMEOUT)
			return err;

		// Disable timeout mechanism
		alarm(0);

		// Check DISC for errors
		if(check_frame_errors(msg, FRAME_SU_LEN, A_RECEIVER, C_DISC, 1, 0))
		{
			DEBUG_PRINT(("Error DISC\n"));
			timeout_handler();
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
	}

	printf("Transfer complete\n");

	//cleanup
	DEBUG_PRINT(("--------------------\n"));
    restore_port_attr(*llfd, oldtio);
	close_port(*llfd);

    printf("Connection closed\n");

    return OK;
}

int llwrite(uint8_t* buf, int len)
{
	DEBUG_PRINT(("--------------------\n"));

	frame[0] = FLAG;
	frame[1] = A_SENDER;
	frame[2] = (sequenceNumber ? C_I_1 : C_I_0);
	frame[3] = (frame[1] ^ frame[2]);

	double r = (double)rand()/RAND_MAX;
	if(r < 0.3)
		frame[3] = 0;

	for (size_t i = 0; i < len; i++)
		frame[i+4] = buf[i];

	frame[len+4] = BCC2_generator(buf, len);
	frame[len+5] = FLAG;
	frame_len = FRAME_SU_LEN + 1 + len;

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

		// Check I for errors
		if(!check_frame_errors(msg, FRAME_SU_LEN, A_SENDER, (sequenceNumber ? C_RR_0 : C_RR_1), 1, 0))
		{
			// Update sequence number
			sequenceNumber = !sequenceNumber;
			DEBUG_PRINT(("Got RR: %u\n", sequenceNumber));
			return OK;
		}
		else if (!check_frame_errors(msg, FRAME_SU_LEN, A_SENDER, (sequenceNumber ? C_REJ_0 : C_REJ_1), 1, 0))
		{
			DEBUG_PRINT(("Got REJ: %u\n", sequenceNumber));
			frame[3] = (frame[1] ^ frame[2]);
			write_msg(*llfd, frame, frame_len, &bytes_written);
			alarm(TIMEOUT);
		}
		else
		{
			DEBUG_PRINT(("Error RR\n"));
			timeout_handler();
		}
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