#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "link_layer.h"
#include "defs.h"

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

		// uint8_t buf2[] = {2,0,0,9,0,0,3,0,3};
		// llwrite(buf2, sizeof(buf2));

		// time_t now = time(0); // Get the system time
		// char* time = asctime(gmtime(&now));
		// printf("%s\n", time);


		// uint8_t time_buf[sizeof(time)];
		// string2ByteArray(time, time_buf);

		//uint8_t buf3[] = {2,0,0,9,0,0,3,0,3};
		//llwrite(buf, sizeof(buf));

		/*
			Sun Oct 20 10:04:28 2019

			7e03000353756e204f637420107e
		*/

		// for (size_t i = 0; i < sizeof(time_buf); i++)
		// 	printf("%c",(char)time_buf[i]);
		// printf("\n");
		

		// call llclose
	}

	err = llclose(&oldtio);
	if(err != OK)
		return err;

    return OK;
}