#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "link_layer.h"
#include "defs.h"



// TODO
// application layer

// CHECKLIST
// check disc on transmitter
// multiple i frame bug - wrong sequence number maybe?

/* OK
	llopen
		frame_set_request
		frame_set_reply
	llread X

	llwrite
*/

//7e0300036101017d5e01017d5e01017d5d7a667e
//7e0300036101017d5e01017d5e01017d5d7a667e





time_t now; // Get the system time
size_t time_len;
uint8_t time_buf[64];

void string2ByteArray(char* input, uint8_t* output, size_t len)
{
    int loop;
    int i;
    
    loop = 0;
    i = 0;
    
    while(loop < len)
    {
        output[i++] = input[loop++];
    }
}


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
	{
		printf("Failed to establish connection\n");
		return err;
	}

	if(mode == RECEIVER)
	{
		while (TRUE)
		{
			if(llread(port_fd) == DISC_CONN)
				break;
		}
	}
	
	else if (mode == TRANSMITTER)
	{
		// write stuff
		
		//#ifdef ENABLE_STUFF
		uint8_t buf2[11] = {(uint8_t)'a',1,1,126,1,1,126,1,1,125,(uint8_t)'z'};
		uint8_t buf[3] = {(uint8_t)'a',126,(uint8_t)'z'};
		// #else
		// uint8_t buf[9] = {2,0,0,9,0,0,3,0,3};
		// #endif

		err = llwrite(buf2, sizeof(buf2));
		err = llwrite(buf, sizeof(buf));
		// if(err == EXIT_TIMEOUT)
		// 	continue;

		// now = time(0); // Get the system time
		// char* time = asctime(gmtime(&now));
		// time_len = strlen(time) - 1;

		// string2ByteArray(time, time_buf, time_len);
		// llwrite(time_buf, time_len);

	}

	err = llclose(&oldtio, mode);
	if(err != OK)
		return err;

    return OK;
}