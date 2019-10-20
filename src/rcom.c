#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "link_layer.h"
#include "defs.h"

int main(int argc, char const *argv[])
{
    if (argc < 3)
	{
        printf("Usage: link_layer <port number 0|1|2> <mode T|R>\n");
        return BAD_ARGS;
    }

    int port_fd;
	TERMIOS oldtio;

	int portNumber = strtol(argv[1],  NULL, 10);

	COM_MODE mode;
	if(strcmp(argv[2], "T") == 0)
		mode = TRANSMITTER;
	else if(strcmp(argv[2], "R") == 0)
		mode = RECEIVER;
	else
		return BAD_ARGS;

	int err = llopen(portNumber, mode, &port_fd, &oldtio);
	if(err != OK)
		printf("Failed to establish connection\n");

	if(mode == RECEIVER)
	{
		while (TRUE)
		{
			llread(port_fd);
		}
	}
	else if (mode == TRANSMITTER)
	{
		// write stuff
		uint8_t buf[] = {1,1,0,6,1,9,9,1};
		llwrite(buf, sizeof(buf));

		// call llclose
	}

	err = llclose(&oldtio);
	if(err != OK)
		return err;

    return OK;
}