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

int llfd = -1;
unsigned retries_count = 0;
unsigned timeout_exit = 0;
unsigned sequenceNumber = 0;
unsigned alarmFlag = 0;

int bytes_written;
int bytes_read;

unsigned bcc_error_count = 0;
unsigned bcc2_error_count = 0;
unsigned resend_count = 0;
unsigned missed_bcc2 = 0;
unsigned baudrate = 0;

uint8_t frame[MAX_FRAME_LEN];
size_t frame_len = MAX_FRAME_LEN;
uint8_t msg[MAX_FRAME_REPLY_LEN];

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
	if(bcc2_ret) *bcc2_ret = ret;
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

	for (size_t i = FRAME_POS_D; i <= old_len-FRAME_OFFSET_BCC2; i++)
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


	for (size_t i = 0; i < *len; i++)
	{
		frame_start[i+FRAME_POS_D] = temp[i];
	}
	frame_start[*len-1] = FLAG;

}

void print_frame_errors(unsigned byteTotal, unsigned byteSlice)
{
	printf("\nTotal bytes: %u | packet size: %u | Baud rate: %u\n", byteTotal, byteSlice, baudrate);
	printf("BCC     : %d | BCC2   : %d\n", bcc_error_count, bcc2_error_count);
	printf("Retries	: %d | Resend : %d | Misses : %d\n", retries_count, resend_count, missed_bcc2);
}


//////////////////////////////////////////////////////////////////////////////////////////////////

void timeout_handler()
{
	alarmFlag = 1;

	if(retries_count >= MAX_RETRIES)
	{
		timeout_exit = 1;
		alarm(0);
		return;
	}

	printf("Retrying... (%d)\n",++retries_count);
	write_msg(llfd, frame, frame_len, &bytes_written);
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

int frame_set_request()
{
	DEBUG_PRINT(("--------------------\n"));

	frame[0] = FLAG;
	frame[FRAME_POS_A] = A_SENDER;
	frame[FRAME_POS_C] = C_SET;
	frame[FRAME_POS_BCC] = (frame[FRAME_POS_A] ^ frame[FRAME_POS_C]);
	frame[4] = FLAG;
	frame_len = FRAME_SU_LEN;

	write_msg(llfd, frame, frame_len, &bytes_written);
	DEBUG_PRINT(("Sent SET\n"));

	// Setup timeout mechanism
	(void) signal(SIGALRM, timeout_handler);
	alarm(TIMEOUT);

	while ( 1 )
	{
		// Wait for UA
		int err = read_msg(llfd, msg, &bytes_read, MAX_FRAME_LEN, return_on_timeout);

		// Exceeded timeout time, exit.
		if(err == EXIT_TIMEOUT)
			return err;

		// Disable timeout mechanism
		alarm(0);

		// Check UA for errors
		uint8_t bcc;
		if((err = check_frame_address(msg, A_SENDER)) != OK)
			DEBUG_PRINT(("Error SET | ADDRESS\n"));
		else if((err = check_frame_control(msg, C_UA)) != OK)
			DEBUG_PRINT(("Error SET | CTRL\n"));
		else if((err = check_frame_bcc(msg, &bcc)) != OK)
			DEBUG_PRINT(("Error UA | BCC CALC: %02x | BCC RCV: %02x\n", bcc, msg[FRAME_POS_BCC]));

		if(err != OK)
		{
			timeout_handler();
			continue;
		}
		else
		{
			DEBUG_PRINT(("Got UA\n"));
			printf("Starting transfer...\n\n");
			return OK;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

void frame_set_reply()
{
	uint8_t err, bcc;
	if((err = check_frame_address(frame, A_SENDER)) != OK)
		DEBUG_PRINT(("Error SET | ADDRESS\n"));
	else if((err = check_frame_control(frame, C_SET)) != OK)
		DEBUG_PRINT(("Error SET | CTRL\n"));
	else if((err = check_frame_bcc(frame, &bcc)) != OK)
		DEBUG_PRINT(("Error UA | BCC CALC: %02x | BCC RCV: %02x\n", bcc, frame[FRAME_POS_BCC]));

	if(err != OK)
		return;

	DEBUG_PRINT(("Got SET\n"));

	frame_SU[FRAME_POS_A] = A_SENDER;
	frame_SU[FRAME_POS_C] = C_UA;
	frame_SU[FRAME_POS_BCC] = frame_SU[FRAME_POS_A] ^ frame_SU[FRAME_POS_C];

	write_msg(llfd, frame_SU, FRAME_SU_LEN, &bytes_written);
	DEBUG_PRINT(("Sent UA\n"));

	printf("Incoming data...\n\n");
}

int frame_i_reply(uint8_t* buffer)
{
	#ifdef ENABLE_FRAME_DELAY
		add_delay();
	#endif

	// #ifdef ENABLE_DEBUG
	// size_t pre_stuffing_size = frame_len;
	// #endif

	byte_destuffing(frame, &frame_len);

	// #ifdef ENABLE_DEBUG
	// DEBUG_PRINT(("DESTUFFING PRE: %lu | POS: %lu\n", pre_stuffing_size, frame_len));
	// for (size_t i = 0; i < frame_len; i++)
	// 	DEBUG_PRINT(("%02x", frame[i]));
	// DEBUG_PRINT(("\n"));
	// #endif


	uint8_t bcc;
	int err = check_frame_bcc(frame, &bcc);


	#ifdef ENABLE_HEAD_ERROR
	if(err == OK)
	{
		err = add_head_error(frame);
		if(err != OK)
			printf("OLD BCC: %02x | NEW BCC: %02x\n", bcc, frame[FRAME_POS_A] ^ frame[FRAME_POS_C]);
	}
	#endif


	if(err != OK)
	{
		bcc_error_count++;
		DEBUG_PRINT(("Error I | BCC CALC: %02x | BCC RCV: %02x\n", bcc, frame[FRAME_POS_BCC]));
		return BCC_ERROR;
	}
	err = check_frame_address(frame, A_SENDER);
	if(err != OK)
	{
		bcc_error_count++;
		DEBUG_PRINT(("Error | ADDRESS\n"));
		return ADR_ERROR;
	}

	DEBUG_PRINT(("Got I: %u | Have: %u\n", (frame[FRAME_POS_C] == C_I_1 ? 1 : 0), sequenceNumber));


	uint8_t bcc2;
	err = check_frame_bcc2(frame, frame_len, &bcc2);

	
	#ifdef ENABLE_DATA_ERROR
	if(err == OK)
	{
		err = add_data_error(frame, frame_len);
		if(err != OK)
		{
			uint8_t temp = BCC2_generator(&frame[FRAME_POS_D], frame_len - FRAME_I_LEN);
			printf("OLD BCC2: %02x | NEW BCC2: %02x\n", bcc2, temp);
			if(bcc2 == temp)
				missed_bcc2++;
		}
	}
	#endif

	if(err == BCC2_ERROR)
	{
		bcc2_error_count++;
		DEBUG_PRINT(("--------------------\n"));

		DEBUG_PRINT(("Error I | BCC2: %02x\n", bcc2));
		if(check_frame_control(frame, (sequenceNumber == 1 ? C_I_0 : C_I_1) == OK))
		{
			// SEND REJ
			frame_RR_REJ[FRAME_POS_A] = A_SENDER;
			frame_RR_REJ[FRAME_POS_C] = (frame[FRAME_POS_C] == C_I_0 ? C_REJ_0 : C_REJ_1);
			frame_RR_REJ[FRAME_POS_BCC] = frame_RR_REJ[FRAME_POS_A] ^ frame_RR_REJ[FRAME_POS_C];

			write_msg(llfd, frame_RR_REJ, FRAME_SU_LEN, &bytes_written);
			DEBUG_PRINT(("Sent REJ: %u | Have: %u\n", (frame_RR_REJ[FRAME_POS_C] == C_REJ_1 ? 1 : 0), sequenceNumber));
			return BCC2_ERROR;
		}

		else
		{
			frame_RR_REJ[FRAME_POS_A] = A_SENDER;
			frame_RR_REJ[FRAME_POS_C] = (frame[FRAME_POS_C] == C_I_0 ? C_RR_1 : C_RR_0);
			frame_RR_REJ[FRAME_POS_BCC] = frame_RR_REJ[FRAME_POS_A] ^ frame_RR_REJ[FRAME_POS_C];


			write_msg(llfd, frame_RR_REJ, FRAME_SU_LEN, &bytes_written);
			DEBUG_PRINT(("Sent RR: %u | Have: %u\n", (frame_RR_REJ[FRAME_POS_C] == C_RR_1 ? 1 : 0), sequenceNumber));
			return BCC2_ERROR;
		}
	}
	else if(err == OK)
	{
		frame_RR_REJ[FRAME_POS_A] = A_SENDER;
    	frame_RR_REJ[FRAME_POS_C] = (sequenceNumber == 1 ? C_RR_1 : C_RR_0);
    	frame_RR_REJ[FRAME_POS_BCC] = frame_RR_REJ[FRAME_POS_A] ^ frame_RR_REJ[FRAME_POS_C];

		if(check_frame_control(frame, (sequenceNumber == 1 ? C_I_0 : C_I_1) == OK))
		{
			#ifdef ENABLE_DATA_PRINT
			for (size_t i = FRAME_POS_D; i < frame_len - FRAME_OFFSET_BCC2; i++)
			{
				printf("%02x ",(frame[i]));
				if( (i - FRAME_POS_D+1) % 16 == 0)
					printf("\n");
			}
			printf("\n");
			#endif

			// Extract frame data
			memcpy(buffer, &frame[FRAME_POS_D], frame_len - FRAME_OFFSET_BCC2);

			sequenceNumber = !sequenceNumber;
			DEBUG_PRINT(("--------------------\n"));
			write_msg(llfd, frame_RR_REJ, FRAME_SU_LEN, &bytes_written);
			DEBUG_PRINT(("Sent RR: %u | Have: %u\n", (frame_RR_REJ[FRAME_POS_C] == C_RR_1 ? 1 : 0), sequenceNumber));
			return OK;
		}
		else
		{
			frame_RR_REJ[FRAME_POS_C] = (sequenceNumber == 1 ? C_RR_0 : C_RR_1);
			frame_RR_REJ[FRAME_POS_BCC] = frame_RR_REJ[FRAME_POS_A] ^ frame_RR_REJ[FRAME_POS_C];
			DEBUG_PRINT(("--------------------\n"));
			write_msg(llfd, frame_RR_REJ, FRAME_SU_LEN, &bytes_written);
			DEBUG_PRINT(("Sent RR: %u | Have: %u\n", (frame_RR_REJ[FRAME_POS_C] == C_RR_1 ? 1 : 0), sequenceNumber));
			return DUP_FRAME;
		}
	}
	else
		return CTR_ERROR;
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

	frame_SU[FRAME_POS_A] = A_RECEIVER;
	frame_SU[FRAME_POS_C] = C_DISC;
	frame_SU[FRAME_POS_BCC] = frame_SU[FRAME_POS_A] ^ frame_SU[FRAME_POS_C];

	write_msg(llfd, frame_SU, FRAME_SU_LEN, &bytes_written);
	DEBUG_PRINT(("Sent DISC\n"));

	while(1)
	{
		printf("bytes read: %d  | err: %d\n",bytes_read,err);
		timeout_exit = 1;
		err = read_msg(llfd, msg, &bytes_read, MAX_FRAME_REPLY_LEN, return_on_timeout);
		if(err == EXIT_TIMEOUT)
			return DISC_CONN;
		err = check_frame_bcc(msg, &bcc);
		if(err != OK)
		{
			DEBUG_PRINT(("Error DISC | BCC: %02x\n", bcc));
			return err;
		}


		// If TRANSMITTER DISC - repeat RECEIVER DISC
		if(check_frame_address(msg, A_SENDER) == OK && check_frame_control(msg, C_DISC) == OK)
		{
			write_msg(llfd, frame_SU, FRAME_SU_LEN, &bytes_written);
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
	}
	return DISC_CONN;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

int llopen(int port, COM_MODE mode, TERMIOS* oldtio, int baud, int pc_head, int pc_data, int delay)
{
	// Only try to open port once
	if( fcntl(llfd, F_GETFD) == -1 && errno == EBADF )
	{
		srand(time(NULL));

		int err = open_port(port, &llfd);
		if(err != OK)
			return err;

		TERMIOS newtio;
		err = set_port_attr(llfd, oldtio, &newtio, baud);
		if(err != OK)
			return err;

		baudrate = baud;

		printf("Connection open\n");
	}

	// 0 for TRANSMITTER | 1 for RECEIVER
	sequenceNumber = (mode == RECEIVER);

	if(mode == TRANSMITTER)
		return frame_set_request();
	else if(mode != RECEIVER)
		return BAD_ARGS;

	#ifdef ENABLE_TESTING
		init_rand(pc_head, pc_data, delay);
	#endif

	return OK;
}

int llclose(TERMIOS* oldtio, COM_MODE mode)
{
	DEBUG_PRINT(("--------------------\n"));

	if(mode == TRANSMITTER && timeout_exit == 0)
	{
		// prepare DISC frame
		frame[FRAME_POS_A] = A_SENDER;
		frame[FRAME_POS_C] = C_DISC;
		frame[FRAME_POS_BCC] = (frame[FRAME_POS_A] ^ frame[FRAME_POS_C]);
		frame[4] = FLAG;
		frame_len = FRAME_SU_LEN;

		// send DISC frame
		write_msg(llfd, frame, frame_len, &bytes_written);
		DEBUG_PRINT(("Sent DISC\n"));

		// Enable timeout mechanism
		(void) signal(SIGALRM, timeout_handler);
		alarm(TIMEOUT);

		// Wait for DISC
		int err;

		while ( 1 )
		{
			err = read_msg(llfd, msg, &bytes_read, MAX_FRAME_REPLY_LEN, return_on_timeout);

			// Exceeded timeout time, exit.
			if(err == EXIT_TIMEOUT)
				return err;

			// Disable timeout mechanism
			alarm(0);

			// Check frame's address
			err = check_frame_address(msg, A_RECEIVER);
			if(err != OK)
			{
				DEBUG_PRINT(("Error DISC | ADDRESS\n"));
				timeout_handler();
				continue;
			}
			// Check frame's control
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
			frame[FRAME_POS_A] = A_RECEIVER;
			frame[FRAME_POS_C] = C_UA;
			frame[FRAME_POS_BCC] = (frame[FRAME_POS_A] ^ frame[FRAME_POS_C]);
			frame[4] = FLAG;
			frame_len = FRAME_SU_LEN;

			// send UA frame
			write_msg(llfd, frame, frame_len, &bytes_written);
			DEBUG_PRINT(("Sent UA\n"));
			DEBUG_PRINT(("--------------------\n"));
			break;
		}
	}

	if(timeout_exit == 0)
	{
		printf("\nTransfer complete\n");
		DEBUG_PRINT(("--------------------\n"));
	}

	//cleanup
    restore_port_attr(llfd, oldtio);
	close_port(llfd);

    printf("Connection closed\n");

    return OK;
}

int llwrite(uint8_t* buf, size_t len)
{
	DEBUG_PRINT(("--------------------\n"));

	frame_len = FRAME_I_LEN + len;

	frame[0] = FLAG;
	frame[FRAME_POS_A] = A_SENDER;
	frame[FRAME_POS_C] = (sequenceNumber == 1 ? C_I_1 : C_I_0);
	frame[FRAME_POS_BCC] = (frame[FRAME_POS_A] ^ frame[FRAME_POS_C]);

	// Add data to frame
	for (size_t i = 0; i < len; i++)
		frame[i+4] = buf[i];

	// Calculate BCC2
	frame[frame_len - FRAME_OFFSET_BCC2] = BCC2_generator(buf, len);
	frame[frame_len - 1] = FLAG;

	#ifdef ENABLE_DATA_PRINT
	for (size_t i = FRAME_POS_D; i < frame_len-FRAME_OFFSET_BCC2; i++)
	{
		printf("%02x ",(frame[i]));
		if( (i - FRAME_POS_D+1) % 16 == 0)
			printf("\n");
	}
	printf("\n");
	#endif

	#ifdef ENABLE_DEBUG
	size_t pre_stuffing_size = frame_len;
	#endif

	byte_stuffing(frame, &frame_len);

	#ifdef ENABLE_DEBUG
	DEBUG_PRINT(("STUFFING PRE: %lu | POS: %lu\n", pre_stuffing_size, frame_len));
	for (size_t i = 0; i < frame_len; i++)
		DEBUG_PRINT(("%02x", frame[i]));
	DEBUG_PRINT(("\n"));
	#endif

	write_msg(llfd, frame, frame_len, &bytes_written);
	DEBUG_PRINT(("Sent I: %u | Have: %u\n", (frame[FRAME_POS_C] == C_I_1 ? 1 : 0), sequenceNumber));

	DEBUG_PRINT(("--------------------\n"));

	// Enable timeout mechanism
	alarm(TIMEOUT);

	while( 1 )
	{
		// Wait for UA
		int err = read_msg(llfd, msg, &bytes_read, MAX_FRAME_REPLY_LEN, return_on_timeout);

		// Exceeded timeout time, exit.
		if(err == EXIT_TIMEOUT)
			return err;

		uint8_t bcc;
		if((err = check_frame_address(msg, A_SENDER)) != OK)
			DEBUG_PRINT(("Error SET | ADDRESS\n"));
		else if((err = check_frame_bcc(msg, &bcc)) != OK)
			DEBUG_PRINT(("Error UA | BCC CALC: %02x | BCC RCV: %02x\n", bcc, msg[FRAME_POS_BCC]));

		// Errors in bcc or address; Ignore frame; resend with retry
		if(err != OK)
		{
			bcc_error_count++;
			continue;
		}

		// No errors + RR with expected sequence number; proceed to next frame
		else if(check_frame_control(msg, (sequenceNumber == 0 ? C_RR_1 : C_RR_0)) == OK)
		{
			// Update sequence number
			sequenceNumber = !sequenceNumber;
			DEBUG_PRINT(("Got RR: %u | Have: %u\n", (msg[FRAME_POS_C] == C_RR_1 ? 1 : 0), sequenceNumber));
			return OK;
		}
		// No errors + REJ with same sequence number; resend without retry
		else if(check_frame_control(msg, (sequenceNumber == 0 ? C_REJ_0 : C_REJ_1)) == OK)
		{
			// #ifdef ENABLE_BCC2_ERROR
			// rm_bcc2_error(frame, frame_len);
			// #endif

			DEBUG_PRINT(("Got REJ: %u | Have: %u\n", (msg[FRAME_POS_C] == C_REJ_1 ? 1 : 0), sequenceNumber));
			write_msg(llfd, frame, frame_len, &bytes_written);
			alarm(TIMEOUT);
			printf("Resend...\n");
			resend_count++;
		}
		// No errors BUT unexpected control field; shouldn't happen; resend with retry
		else
		{
			DEBUG_PRINT(("Error RR | CTRL\n"));
			continue;
		}
	}

	return OK;
}

int llread(uint8_t* buffer, size_t* len)
{
	DEBUG_PRINT(("--------------------\n"));

    // Wait for frame
	int err;
	if( (err = read_msg(llfd, frame, &bytes_read, MAX_FRAME_LEN, NULL)) != OK)
        return err;

	frame_len = bytes_read;


    switch (frame[FRAME_POS_C])
    {
    case C_SET:
        frame_set_reply();
        return SET_RET;
    case C_I_0:
    case C_I_1:
        err =frame_i_reply(buffer);
		*len = frame_len - FRAME_I_LEN;
		return err;
    case C_DISC:
		return frame_disc_reply();
    default:
        break;
    }

    return OK;
}
