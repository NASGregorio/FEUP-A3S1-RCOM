#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <unistd.h>

// #include "defs.h"
#include "link_layer.h"
#include "defs.h"
// #include "tty_layer.h"

int main(int argc, char const *argv[])
{
    if (argc < 3)
	{
        printf("Usage: link_layer <port number 0|1|2> <mode T|R>\n");
        return BAD_ARGS;
    }

    int port_fd;

	int portNumber = strtol(argv[1],  NULL, 10);

	COM_MODE mode;
	if(strcmp(argv[2], "T") == 0)
		mode = TRANSMITTER;
	else if(strcmp(argv[2], "R") == 0)
		mode = RECEIVER;
	else
		return BAD_ARGS;

    printf("A\n");


	int err = llopen(portNumber, mode, &port_fd);
	if(err != OK)
		return err;
        
    printf("Z\n");

    return OK;
}
