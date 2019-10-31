#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include "link_layer.h"
#include "defs.h"


char* file_path;
char* file_name;
FILE* file;

long file_size;
long file_split_frames;
size_t file_current_frame;

uint8_t* buffer_sender;
uint8_t* buffer_rcvr;

size_t buffer_size;

int file_op_ret;

struct timeval st, et;
size_t whole_buffer_size;
int do_once = 0;


uint8_t calculate_size_length(long size)
{
	uint8_t res = 0;
	while (size > 0)
	{
		size >>= 8;
		res++;
	}
	return res;
}

void build_control_packet(uint8_t** packet, size_t* packet_len, char** filename, long filesize)
{
	// Get filename from path
	char* marker = strrchr(*filename, '/');
	if(marker != NULL)
		*filename = &(marker[1]);

	// Get filename length
	size_t fn_len = strlen(*filename);

	// Get length, in bytes, of hexadecimal file size representation
	uint8_t fs_len = calculate_size_length(filesize);
	
	// Allocate space for control packet
	*packet = (uint8_t*)malloc(1 + 2+fs_len + 2+fn_len);

	// Build control packet
	(*packet_len) = 0;
	(*packet)[(*packet_len)++] = TLV_START;
	(*packet)[(*packet_len)++] = TLV_FILESIZE;
	(*packet)[(*packet_len)++] = fs_len;

	// Add file size bytes
	for (size_t i = 0; i < fs_len; i++)
		(*packet)[(*packet_len)++] = (filesize >> 8*(fs_len-i-1));

	(*packet)[(*packet_len)++] = TLV_FILENAME;
	(*packet)[(*packet_len)++] = fn_len;

	// Add file name bytes
	for (size_t i = 0; i < fn_len; i++)
		(*packet)[(*packet_len)++] = (uint8_t)(*filename)[i];
}

void build_data_packet(uint8_t** packet, size_t* packet_len, size_t sequence_num, uint8_t* data, size_t data_len)
{
	// Allocate space for control packet
	*packet = (uint8_t*)malloc(1 + 3 + data_len);

	// Build data packet
	(*packet_len) = 0;
	(*packet)[(*packet_len)++] = TLV_DATA;
	(*packet)[(*packet_len)++] = sequence_num % 255;
	(*packet)[(*packet_len)++] = data_len / 256;
	(*packet)[(*packet_len)++] = data_len % 256;

	// Add data bytes
	for (size_t i = 0; i < data_len; i++)
		(*packet)[(*packet_len)++] = data[i];
}

void unpack_control(long* filesize, char** filename, uint8_t* packet)
{
	uint8_t marker = 2;

	uint8_t len = packet[marker++];
	(*filesize) = 0;
	for (size_t i = 0; i < len; i++)
		(*filesize) += (packet[marker++] << (8*(len-i-1)));

	marker++;

	len = packet[marker++];

	*filename = malloc(len+1);

	for (size_t i = 0; i < len; i++)
		(*filename)[i] = packet[marker++];
	(*filename)[len] = '\0';
}

int main(int argc, char *argv[])
{
	uint8_t* ctrl_packet = NULL;
	size_t ctrl_packet_len = 0;

	uint8_t* data_packet = NULL;
	size_t data_packet_len = 0;

    if (argc < 3)
	{
        printf("Usage: link_layer <port number 0|1|2> <mode T|R> <file path - required in mode T>\n");
        return BAD_ARGS;
    }

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
		if(argc < 5)
		{
        	printf("Usage: link_layer <port number 0|1|2> <mode T|R> <file path - T only> <packet size - T only>\n");
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

		buffer_size = strtol(argv[4],  NULL, 10);
		whole_buffer_size = buffer_size;
		buffer_sender = malloc(buffer_size);

		file_split_frames = file_size / buffer_size;
		if(file_size % buffer_size != 0)
			file_split_frames++;

	}

	int err;

	// read loop
	if(mode == RECEIVER)
	{
		// open connection
		err = llopen(portNumber, mode, &oldtio, strtol(argv[3], NULL, 10), strtol(argv[4], NULL, 10), strtol(argv[5], NULL, 10), strtol(argv[6], NULL, 10));
		if(err != OK)
		{
			printf("Failed to establish connection\n");
			return err;
		}


		buffer_rcvr = malloc(256);

		while( 1 )
		{
			err = llread(buffer_rcvr, &buffer_size);
			if(err == DISC_CONN)
				break;

			if(err == OK)
			{
				if(buffer_rcvr[0] == TLV_START)
				{
					gettimeofday(&st,NULL);
					printf("Got start control packet > %s | %luB\n", file_name, file_size);

					unpack_control(&file_size, &file_name, buffer_rcvr);

					if(file_size > 256)
						buffer_rcvr = realloc(buffer_rcvr, file_size);

					file = fopen(file_name, "w");

				}
				else if(buffer_rcvr[0] == TLV_DATA)
				{
					if(do_once == 0)
					{
						do_once = 1;
						whole_buffer_size = (size_t)(buffer_rcvr[2]*256+buffer_rcvr[3]);
					}

					file_op_ret = fwrite(&buffer_rcvr[4], buffer_rcvr[2]*256+buffer_rcvr[3], 1, file);
					if(file_op_ret != 1 && ferror(file))
					{
						printf("Read: %d\n", file_op_ret);
						printf("Error while reading file");
						continue;
					}
					printf("Got data packet > N-%03u | %luB\n", buffer_rcvr[1]+1, (size_t)(buffer_rcvr[2]*256+buffer_rcvr[3]));
				}
				else if(buffer_rcvr[0] == TLV_END)
				{
					gettimeofday(&et,NULL);
					printf("Got end control packet > %s | %luB\n", file_name, file_size);
				}
			}
		}
	}

	// write loop
	else if (mode == TRANSMITTER)
	{
		// open connection
		err = llopen(portNumber, mode, &oldtio, strtol(argv[5], NULL, 10), 0, 0, 0);
		if(err != OK)
		{
			printf("Failed to establish connection\n");
			return err;
		}


		// Send control packet
		build_control_packet(&ctrl_packet, &ctrl_packet_len, &file_path, file_size);
		printf("Sending start control packet > %s | %luB\n", file_path, file_size);
		err = llwrite(ctrl_packet, ctrl_packet_len);
		
		gettimeofday(&st,NULL);
		if(err != EXIT_TIMEOUT)
		{
			// write stuff
			file_current_frame = 0;
			while (file_current_frame < file_split_frames)
			{

				if( (file_size / buffer_size) == file_current_frame)
					buffer_size = file_size % buffer_size;
				else
					buffer_size = buffer_size;

				file_op_ret=fread(buffer_sender, buffer_size, 1, file);
				if(file_op_ret != 1 || feof(file) > 0)
				{
					if (ferror(file))
					{
						printf("Read: %d\n", file_op_ret);
						printf("Error while reading file");
						continue;
					}
				}

				build_data_packet(&data_packet, &data_packet_len, file_current_frame, buffer_sender, buffer_size);
				printf("Sending data packet > N-%03u | %luB | \n", data_packet[1]+1, buffer_size);

				err = llwrite(data_packet, data_packet_len);
				if(err == EXIT_TIMEOUT)
					break;

				file_current_frame++;

				// gettimeofday(&et,NULL);
				// int elapsed = ((et.tv_sec - st.tv_sec) * 1000000) + (et.tv_usec - st.tv_usec);
				// printf("dt: %d us\n",elapsed);
			}
		}

		if(err != EXIT_TIMEOUT)
		{
			gettimeofday(&et,NULL);

			// Send control packet
			ctrl_packet[0] = TLV_END;
			err = llwrite(ctrl_packet, ctrl_packet_len);
			printf("Sending end control packet > %s | %luB\n", file_path, file_size);
		}
	}

	fclose(file);

	if(buffer_sender)
		free(buffer_sender);

	if(buffer_rcvr)
		free(buffer_rcvr);

	if(ctrl_packet)
		free(ctrl_packet);

	if(file_name)
		free(file_name);

	print_frame_errors(file_size, whole_buffer_size);

	int elapsed = ((et.tv_sec - st.tv_sec) * 1000000) + (et.tv_usec - st.tv_usec);
	printf("Time interval first - last data packets: %d us | %f s\n",elapsed, elapsed * 0.000001 );

	// close connection
	err = llclose(&oldtio, mode);
	if(err != OK)
		return err;

    return OK;
}
