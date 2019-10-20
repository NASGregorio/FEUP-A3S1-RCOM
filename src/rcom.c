#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "link_layer.h"
#include "defs.h"


time_t now; // Get the system time
size_t time_len;
uint8_t time_buf[64];

void string2ByteArray(char* input, uint8_t* output)
{
    int loop;
    int i;
    
    loop = 0;
    i = 0;
    
    while(input[loop] != '\0')
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
			llread(port_fd);
		}
	}
	
	else if (mode == TRANSMITTER)
	{
		// write stuff
		uint8_t buf[] = {1,1,0,6,1,9,9,1};
		llwrite(buf, sizeof(buf));

		//sleep(1);

		now = time(0); // Get the system time
		char* time = asctime(gmtime(&now));
		time_len = strlen(time);
		printf("%s: %ld\n", time, time_len);

		string2ByteArray(time, time_buf);
		llwrite(time_buf, time_len);

		// call llclose
	}

	err = llclose(&oldtio);
	if(err != OK)
		return err;

    return OK;
}