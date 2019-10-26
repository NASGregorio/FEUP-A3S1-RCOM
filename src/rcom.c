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
// verificar llclose
// verificar/adicionar comentarios

/* OK
	llopen
		frame_set_request
		frame_set_reply
	llread
		frame_i_reply
	llwrite
	llclose (verificar)
		frame_disc_reply
*/


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

#define BUF_SIZE 64000

const char* file_path;
FILE* file;
long file_size;
long file_split_frames;
uint8_t buffer[BUF_SIZE];
size_t file_current_frame;
int file_op_ret;
int count;
size_t buffer_size;


int main(int argc, char const *argv[])
{
	// uint8_t buf[64] = {FLAG, A_SENDER, C_I_0, A_SENDER ^ C_I_0, 0x17, 0x6A, (0x17 ^ 0x6A), FLAG};
	// size_t len = 8; 

	// for (size_t i = 0; i < len; i++)
	// 	printf("%02x ", buf[i]);
	// printf("\n");
	// byte_stuffing(buf, &len);
	// for (size_t i = 0; i < len; i++)
	// 	printf("%02x ", buf[i]);
	// printf("\n");
	// byte_destuffing(buf, &len);
	// for (size_t i = 0; i < len; i++)
	// 	printf("%02x ", buf[i]);
	// printf("\n");
	
	
    if (argc < 3)
	{
        printf("Usage: link_layer <port number 0|1|2> <mode T|R> <file path - required in mode T>\n");
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


	// Check file path arg if transmitter
	if(mode == TRANSMITTER)
	{
		if(argc < 4)
		{
        	printf("Usage: link_layer <port number 0|1|2> <mode T|R> <file path - required in mode T>\n");
			return BAD_ARGS;
		}
		file_path = argv[3];

		file = fopen(file_path, "rb");
		if(file == NULL){
			perror("fopen");
			return FOPEN_FAIL;
		}

		fseek(file, 0L, SEEK_END);
		file_size = ftell(file);
		fseek(file, 0, SEEK_SET);
		printf("%ld \n", file_size);

		file_split_frames = file_size / BUF_SIZE;
		if(file_size % BUF_SIZE != 0)
			file_split_frames++;
	}
	else
	{
		file = fopen("test_transfer_new.gif", "w");
	}
	


	// open connection
	int err = llopen(portNumber, mode, &port_fd, &oldtio);
	if(err != OK)
	{
		printf("Failed to establish connection\n");
		return err;
	}

	// read loop
	if(mode == RECEIVER)
	{
		count = 0;
		while( 1 ) 
		{
			err = llread(buffer, &buffer_size);
			if(err == DISC_CONN)
				break;

			if(err == SET_RET)
				continue;

			file_op_ret = fwrite(buffer, buffer_size, 1, file);
			if(file_op_ret != 1 && ferror(file))
			{
				printf("Read: %d\n", file_op_ret);
				printf("Error while reading file");
				continue;
			}
			count++;
		}
	}
	
	// write loop
	else if (mode == TRANSMITTER)
	{
		// write stuff
		file_current_frame = 0;
		while (file_current_frame < file_split_frames)
		{

			if( (file_size / BUF_SIZE) == file_current_frame)
				buffer_size = file_size % BUF_SIZE;
			else
				buffer_size = BUF_SIZE;

			file_op_ret=fread(buffer, buffer_size, 1, file);
			if(file_op_ret != 1 || feof(file) > 0)
			{
				if (ferror(file))
				{
					printf("Read: %d\n", file_op_ret);
					printf("Error while reading file");
					continue;
				}
			}

			// for (size_t i = 0; i < 9; i++)
			// 	printf("%02x ", buffer[i]);
			// printf("\n");
			// for (size_t i = 0; i < 9; i++)
			// 	printf("%c", (char)buffer[i]);
			// printf("\n");

			err = llwrite(buffer, buffer_size);
			if(err == EXIT_TIMEOUT)
				break;

			file_current_frame++;
			

			// err = llwrite(buf2, sizeof(buf2));

		}
		
		
		// //#ifdef ENABLE_STUFF
		// uint8_t buf2[11] = {(uint8_t)'a',1,1,126,1,1,126,1,1,125,(uint8_t)'z'};
		// uint8_t buf[3] = {(uint8_t)'a',126,(uint8_t)'z'};
		// // #else
		// // uint8_t buf[9] = {2,0,0,9,0,0,3,0,3};
		// // #endif

		// err = llwrite(buf2, sizeof(buf2));
		// err = llwrite(buf, sizeof(buf));
		// if(err == EXIT_TIMEOUT)
		// 	continue;
	}


	fclose(file);

	// close connection
	err = llclose(&oldtio, mode);
	if(err != OK)
		return err;

    return OK;
}