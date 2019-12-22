#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>

#include <arpa/inet.h>

#include "socket_helper.h"
#include "defs.h"

char request[256];

int open_socket(int* sock_fd, FILE** sock_file)
{
	if ((*sock_fd = socket(AF_INET,SOCK_STREAM,0)) < 0)
	{
		perror("socket()");
		return OPEN_SOCKET_ERROR;
	}

	*sock_file = fdopen(*sock_fd, "r");

	return OK;
}

int connect_socket(int sock_fd, char* addr, uint16_t port)
{
	struct sockaddr_in server_addr;

	/*server address handling*/
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(addr);	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(port);               /*server TCP port must be network byte ordered */

	if(connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("connect()");
		return CONNECT_SOCKET_ERROR;
	}
	return OK;
}

int read_msg(int sock_fd, char* code_str, char** out_msg)
{

	char msg[256] = "";
	uint8_t len = 0;

	uint8_t res = 0;
	char buf[1] = "";

	char code[4];


	while ( 1 )
	{

		res = read(sock_fd,buf,1);
		if (res == -1)
		{
			perror("read: ");
			return 200;
		}

		msg[len++] = buf[0];

		if( (msg[len - 2] == '\r' && msg[len - 1] == '\n') || len == 256 )
		{

			strncpy(code, msg, 3);
			printf("%s", msg);

			if(strncmp(code, code_str, 3) != OK)
			{
				printf("Return code mismatch\n");
				return 128;
			}
			else
			{
				if(out_msg != NULL)
				{
					*out_msg = malloc(strlen(msg));
					strncpy(*out_msg, msg, strlen(msg));
				}
				return OK;
			}
		}
    }
}

int read_file_w_size(int retr_fd, FILE* dl, size_t file_size)
{
	size_t block_size = 1024;
	uint8_t* data = malloc(block_size);
	size_t size = file_size;
	size_t print_timer = 1000;
	size_t timer = 0;
	int br = 0;
	while ( size > 0 )
	{
		br = read(retr_fd, data, block_size);
		size -= br;

		if (br < 0)
		{
			printf("Download fail\n");
			return 4;
		}

		if (fwrite(data, br, 1, dl) < 0)
		{
			printf("Saving download fail\n");
			return 5;
		}

		if(timer >= print_timer) {
			double j = (1-((double)size/file_size))*100;
			int k = (int)j/5;

			printf("Download: %.1f%% [%.*s>%.*s]\r", j, k, "====================", (20-k), "                    ");
			fflush(stdout);
			timer = 0;
		}
		timer++;
	}

	free(data);

	return OK;
}

int read_msg_block(int sock_fd, char* code_str)
{

	printf("<<<<<<\n");

	fd_set read_check;
	FD_ZERO(&read_check);
	FD_SET(sock_fd, &read_check);
	struct timeval timeout;
	//char code[4];

	int retries = 3;

	while( retries > 0 ) {
  		timeout.tv_sec = 0;
        timeout.tv_usec = 100 * 1000;

		int n = select(sock_fd + 1, &read_check, NULL, NULL, &timeout);
		if (n == -1) {
			retries--;
			continue;
		} 
		else if (n == 0) {
			retries--;
			continue;
		}
		if (!FD_ISSET(sock_fd, &read_check)) {
			retries--;
			continue;
		}

		if(read_msg(sock_fd, code_str, NULL) != OK) {
			return 128;
		}
		retries = 3;
	}

	return OK;
}

int read_single_msg(int sock_fd, char* code_str, char** msg, size_t tsec, size_t tusec)
{

	fd_set read_check;
	FD_ZERO(&read_check);
	FD_SET(sock_fd, &read_check);
	struct timeval timeout;

	timeout.tv_sec = tsec;
	timeout.tv_usec = tusec;

	int n = select(sock_fd + 1, &read_check, NULL, NULL, &timeout);
	if (n == -1) {
		printf("Read Timeout\n");
		return -1;
	} 
	else if (n == 0) {
		printf("Read Timeout\n");
		return -1;
	}
	if (!FD_ISSET(sock_fd, &read_check)) {
		printf("Read Timeout\n");
		return -1;
	}

	printf("<<<<<<\n");
	return read_msg(sock_fd, code_str, msg);
}

int write_msg(int sock_fd, char* cmd, ...)
{
	va_list va;
	va_start (va, cmd);
	vsnprintf(request, 256, cmd, va);
	va_end(va);

	write(sock_fd, request, strlen(request));
	printf(">>>>>>\n");
	printf("%s", request);

	return OK;
}

int close_socket()
{
	return OK;


	// printf("\n\n");
	// for (size_t i = 0; i < 2048; i++)
	// {
	//     printf("%02x ");
	//     if(i != 0 && i % 16 == 0)
	//         printf("\n");
	// }
	// printf("\n\n");



}
