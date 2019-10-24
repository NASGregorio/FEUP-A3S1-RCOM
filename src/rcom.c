#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "link_layer.h"
#include "defs.h"


// application layer
// check sequence number / send REJ on bcc2 error

//TODO check disc on transmitter

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

	#ifdef DEBUG_MAIN_STUFFING
	uint8_t len = 11;
	uint8_t msg[] = {(uint8_t)'a',1,1,126,1,1,126,1,1,125,(uint8_t)'z'};
	uint8_t BCC2 = msg[0];
	for (size_t i = 1; i < len; i++)
	{
		BCC2 ^= msg[i];
	}

	size_t frame_len = FRAME_I_LEN + len;
	uint8_t buf[128] = {FLAG, A_SENDER, C_I_0, A_SENDER ^ C_I_0, (uint8_t)'a', 1,1,126,1,1,126,1,1,125, (uint8_t)'z', BCC2, FLAG};

	for (size_t i = 0; i < frame_len; i++)
		DEBUG_PRINT(("%02x ", buf[i]));
	DEBUG_PRINT(("\n"));

	byte_stuffing(buf, &frame_len);

	for (size_t i = 0; i < frame_len; i++)
		DEBUG_PRINT(("%02x ", buf[i]));
	DEBUG_PRINT(("\n"));

	byte_destuffing(buf, &frame_len);

	for (size_t i = 0; i < frame_len; i++)
		DEBUG_PRINT(("%02x ", buf[i]));
	DEBUG_PRINT(("\n"));
	#endif


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
		
		#ifdef ENABLE_STUFF
		//uint8_t buf[11] = {(uint8_t)'a',1,1,126,1,1,126,1,1,125,(uint8_t)'z'};
		uint8_t buf[3] = {(uint8_t)'a',126,(uint8_t)'z'};
		#else
		uint8_t buf[9] = {2,0,0,9,0,0,3,0,3};
		#endif

		llwrite(buf, sizeof(buf));
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