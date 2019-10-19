#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "defs.h"
#include "link_layer.h"

unsigned retriesCount = 0;
unsigned timeout_exit = 0;
int* port_fd;
int bytesWritten;
int bytesRead;
TERMIOS newtio;
int conn_good = 0;

int returnOnTimeout()
{
	if(timeout_exit)
	{
		printf("Connection timeout\n");
		return EXIT_TIMEOUT;
	}
	return OK;
}

int send_set_msg()
{
	uint_8 set_msg[FRAME_SU_LEN] = { FLAG, A_SENDER, C_SET, A_SENDER ^ C_SET, FLAG };
	return write_msg(*port_fd, set_msg, FRAME_SU_LEN, &bytesWritten);
}

int send_ua_msg()
{
	uint_8 ua_msg[FRAME_SU_LEN] = { FLAG, A_RECEIVER, C_UA, A_RECEIVER ^ C_UA, FLAG };
	return write_msg(*port_fd, ua_msg, FRAME_SU_LEN, &bytesWritten);
}

/**
 * @brief return true if no errors
 */
int check_bcc_errors(uint_8 a, uint_8 c, uint_8 bcc)
{
	uint_8 calc = a ^ c;
	return bcc == calc;
}

void timeoutHandler()
{
	if(retriesCount >= MAX_RETRIES)
	{
		timeout_exit = 1;
		alarm(0);
		return;
	}

	printf("Retrying... (%d)\n",retriesCount++);
	send_set_msg();
	alarm(TIMEOUT);
}

int llopen(int port, COM_MODE mode, int* fd, TERMIOS* oldtio)
{
	port_fd = fd;

	int err = open_port(port, port_fd);
	if(err != OK)
		return err;

	err = set_port_attr(*port_fd, oldtio, &newtio);
	if(err != OK)
		return err;

	if(mode == TRANSMITTER)
	{
		send_set_msg();

		// Disable timeout mechanism
		(void) signal(SIGALRM, timeoutHandler);
		alarm(TIMEOUT);

		// Wait for UA
		uint_8 msg[255];
		err = read_msg(*port_fd, msg, &bytesRead, 255, returnOnTimeout);

		// Exceeded timeout time, exit.
		if(err != OK)
			return err;

		// Disable timeout mechanism
		alarm(0);

		// Check UA for errors
		if(bytesRead != FRAME_SU_LEN || !check_bcc_errors(msg[1], msg[2], msg[3]))
		{
			#ifdef DEBUG_TTY_CALLS
			printf("Error in SET\n");
			#endif
			timeoutHandler();
		}
	}
	else if(mode == RECEIVER)
	{
		while(!conn_good)
		{
			// Wait for SET
			uint_8 msg[255];
			read_msg(*port_fd, msg, &bytesRead, 255, NULL);

			// Check SET for errors
			if(bytesRead != FRAME_SU_LEN || !check_bcc_errors(msg[1], msg[2], msg[3]))
			{
				#ifdef DEBUG_TTY_CALLS
				printf("Error in SET\n");
				#endif
				continue;
			}

			send_ua_msg();
			conn_good = 1;
		}
	}
	else
	{
		return BAD_ARGS;
	}


	return OK;
}
