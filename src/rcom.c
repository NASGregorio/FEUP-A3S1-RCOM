#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "link_layer.h"
#include "defs.h"


// application layer
// byte stuffing

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

int byte_stuffing(size_t* new_len, uint8_t* msg, size_t len)
{
	size_t extra = 0;

	for (size_t i = 0; i < len; i++)
	{
		if ( msg[i] == FLAG || msg[i] == ESCAPE )
			extra++;
	}

	*new_len = len + extra;

	for (size_t i = 0; i < len; i++)
		printf("%02x", msg[i]);
	printf("\n");

	for (size_t i = *new_len; i > 0; i--)
	{
		if ( msg[i] == FLAG || msg[i] == ESCAPE )
		{
			msg[i] ^= BYTE_XOR;
			msg[i-1] ^= ESCAPE;
			i--;
		}
	}

	printf("\n");	
	printf("\n");	
	printf("\n");	
	printf("\n");	

	
	for (size_t i = 0; i < *new_len; i++)
		printf("%02x", msg[i]);
	printf("\n");	

	return OK;
}

int main(int argc, char const *argv[])
{
	uint8_t buf[128] = {1,1,126,1,1,126,1,1,125};
	//printf("%x\n",buf);

	size_t newlen = 0;
	byte_stuffing(&newlen, buf, 9);


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
		
		uint8_t buf[128] = {1,1,126,1,1,126,1,1,125};
		//printf("%x\n",buf);

		size_t newlen = 0;
		byte_stuffing(&newlen, buf, 128);
		//printf("%x\n",buf);
		
		//llwrite(buf, sizeof(buf));

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