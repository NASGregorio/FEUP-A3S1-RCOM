#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "defs.h"
#include "link_layer.h"
#include "tty_layer.h"

///////////////////////////////////////////////////////////////////
const char *bit_rep[16] = {
    [ 0] = "0000", [ 1] = "0001", [ 2] = "0010", [ 3] = "0011",
    [ 4] = "0100", [ 5] = "0101", [ 6] = "0110", [ 7] = "0111",
    [ 8] = "1000", [ 9] = "1001", [10] = "1010", [11] = "1011",
    [12] = "1100", [13] = "1101", [14] = "1110", [15] = "1111",
};

void print_byte(int d, uint_8 byte)
{
    printf("%d: %s%s\n", d, bit_rep[byte >> 4], bit_rep[byte & 0x0F]);
}
///////////////////////////////////////////////////////////////////

unsigned retriesCount = 0;
unsigned timeout_exit = 0;
int* port_fd;
int bytesWritten;
int bytesRead;
TERMIOS oldtio,newtio;

void exitOnTimeout()
{
	if(timeout_exit)
	{
		restore_port_attr(*port_fd, &oldtio);
		close_port(*port_fd);
		exit(-1);
	}
}

void timeoutHandler()
{
	if(retriesCount >= MAX_RETRIES)
	{
		timeout_exit = 1;
		alarm(0);
		return;
	}


	uint_8 set_msg[FRAME_SU_LEN] = { FLAG, A_SENDER, C_SET, A_SENDER ^ C_SET, FLAG };
	write_msg(*port_fd, set_msg, FRAME_SU_LEN, &bytesWritten);

	retriesCount++;
	alarm(TIMEOUT);
	printf("Retrying... (%d)\n",retriesCount);
}

int llopen(int port, COM_MODE mode, int* fd)
{
	port_fd = fd;

	int err = open_port(port, port_fd);
	if(err != 0)
		return err;


	printf("B\n");

	err = set_port_attr(*port_fd, &oldtio, &newtio);
	if(err != 0)
		return err;

	uint_8 bcc;

	if(mode == TRANSMITTER)
	{
		// SEND SET
		uint_8 set_msg[FRAME_SU_LEN] = { FLAG, A_SENDER, C_SET, A_SENDER ^ C_SET, FLAG };
		write_msg(*port_fd, set_msg, FRAME_SU_LEN, &bytesWritten);


		// ENABLE TIMEOUT MECHANISM
		(void) signal(SIGALRM, timeoutHandler);
		alarm(TIMEOUT);

		// WAIT FOR UA
		uint_8 msg[255];
		read_msg(*port_fd, msg, &bytesRead, 255, exitOnTimeout);

		// DISABLE TIMEOUT MECHANISM
		alarm(0);

		printf("Got %d bytes:\n", bytesRead);
		for (unsigned i = 0; i < bytesRead; i++)
	    	printf(" %u: %x\n", i, msg[i]);

		bcc = msg[1] ^ msg[2];
		if(bcc != msg[3])
			printf("Error found in msg!\n");
	}
	else if(mode == RECEIVER)
	{
		// WAIT FOR SET
		uint_8 msg[255];
		read_msg(*port_fd, msg, &bytesRead, 255, NULL);

		printf("Got %d bytes:\n", bytesRead);
		for (unsigned i = 0; i < bytesRead; i++)
	    	printf(" %u: %x\n", i, msg[i]);

		bcc = msg[1] ^ msg[2];
		if(bcc != msg[3])
			printf("Error found in msg!\n");

		// SEND UA
		uint_8 ua_msg[FRAME_SU_LEN] = { FLAG, A_RECEIVER, C_UA, A_RECEIVER ^ C_UA, FLAG };
		write_msg(*port_fd, ua_msg, FRAME_SU_LEN, &bytesWritten);
	}
	else
	{
		//TODO: error handling
	}


	return OK;
}
