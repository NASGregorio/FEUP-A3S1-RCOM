#include <arpa/inet.h>
#include <stdio.h>


#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ioctl.h>
#include <linux/sockios.h>
#include <linux/sockios.h>

#include <fcntl.h>
#include <sys/select.h>
#include <sys/stat.h>

#include "ftp_info.h"
#include "socket_helper.h"
#include "defs.h"


// https://tools.ietf.org/html/rfc959


void print_msg(char* msg, uint8_t is_outgoing)
{
	if(is_outgoing == 0)
	{
		printf(">>>>>>\n");
		printf("%s", msg);
	}
	else
	{
		printf("<<<<<<\n");
		printf("%s", msg);
	}
}


// int read_reply(int sock_fd, FILE* sock_file, char** reply, size_t* reply_len, size_t* reply_buf_size, char* code_str)
// {
// 	char code[4];

// 	int err = read_msg(sock_fd, sock_file, reply, reply_len, reply_buf_size);
// 	if(err != OK)
// 		return err;

// 	strncpy(code, *reply, 3);
// 	print_msg(*reply, 1);

// 	if(strncmp(code, code_str, 3) != OK)
// 		return 128;

// 	return OK;
// }


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
			printf("Download: %.1f%%\n", (1-((double)size/file_size))*100);
			timer = 0;
		}
		timer++;
	}
	printf("Download: 100.0%%\n");

	free(data);

	return OK;
}

int read_msg2(int sock_fd, char* code_str, char** out_msg) {

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

	return 200;
}

int read_msg_block(int sock_fd, char* code_str) {

	printf("<<<<<<\n");

	fd_set read_check;
	FD_ZERO(&read_check);
	FD_SET(sock_fd, &read_check);
	struct timeval timeout;
	//char code[4];

	int retries = 3;

	while( retries > 0 ) {
  		timeout.tv_sec = 0;
        timeout.tv_usec = 500 * 1000;

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

		if(read_msg2(sock_fd, code_str, NULL) != OK) {
			return 128;
		}
		retries = 3;
	}

	return OK;
}

int read_single_msg(int sock_fd, char* code_str, char** msg) {

	fd_set read_check;
	FD_ZERO(&read_check);
	FD_SET(sock_fd, &read_check);
	struct timeval timeout;

	timeout.tv_sec = 30;
	timeout.tv_usec = 0;

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
	return read_msg2(sock_fd, code_str, msg);
}

int read_two_step_msg(int sock_fd, char* code1_str, char* code2_str) {

	int err = read_single_msg(sock_fd, code1_str, NULL);
	if(err != OK)
		return err;

	sleep(1);

	err = read_single_msg(sock_fd, code2_str, NULL);
	return err;
}

int main(int argc, char const *argv[])
{
	if(argc != 2)
	{
		printf("Usage:\ndownload ftp://[<user>:<password>@]<host>/<url-path>\n");
		return BAD_ARGS;
	}

	// Parse url string
	ftp_info_t ftp_info;
	int err = build_ftp_info(argv[1], &ftp_info);
	if(err != OK)
		return err;

	// Print url information
	print_ftp_info(&ftp_info);


	// Prepare tcp socket
	int sock_fd = 0;
	FILE* sock_file;
	err = open_socket(&sock_fd, &sock_file);
	if(err != OK)
		return err;

	// Try to connect
	printf("------------- CONNECTION -------------\n");
	err = connect_socket(sock_fd, ftp_info.addr, 21);
	if(err != OK)
		return err;

	char* reply;
	//size_t reply_len;
	char request[256];
	char* line_endings = "\r\n";
	//size_t reply_buf_size = 256;

	err = read_msg_block(sock_fd, "220");
	if(err != OK)
		return err;


	printf("------------- AUTHENTIFICATION -------------\n");

	snprintf(request, 256, "USER %s%s", ftp_info.usr, line_endings);
	write(sock_fd, request, strlen(request));
	print_msg(request, 0);

	err = read_single_msg(sock_fd, "331", &reply);
	if(err != OK)
		return err;

	snprintf(request, 256, "PASS %s%s", ftp_info.pwd, line_endings);
	write(sock_fd, request, strlen(request));
	print_msg(request, 0);

	err = read_single_msg(sock_fd, "230", &reply);
	if(err != OK)
		return err;




	printf("------------- SET TYPE -------------\n");

	snprintf(request, 256, "TYPE I%s", line_endings);
	write(sock_fd, request, strlen(request));
	print_msg(request, 0);

	err = read_single_msg(sock_fd, "200", &reply);
	if(err != OK)
		return err;




	printf("------------- GET SIZE -------------\n");

	snprintf(request, 256, "SIZE %s%s", ftp_info.url_path, line_endings);
	write(sock_fd, request, strlen(request));
	print_msg(request, 0);

	int got_size = 0;
	size_t file_size = 0;

	err = read_single_msg(sock_fd, "213", &reply);
	if(err == OK) {
		file_size = strtol(&reply[4], NULL, 10);
		got_size = 1;
	}


	printf("------------- CHANGE DIRECTORY -------------\n");

	snprintf(request, 256, "CWD /%s%s", ftp_info.path, line_endings);
	write(sock_fd, request, strlen(request));
	print_msg(request, 0);

	err = read_msg_block(sock_fd, "250");
	if(err != OK)
		return err;




	printf("------------- PASSIVE MODE -------------\n");

	snprintf(request, 256, "PASV%s", line_endings);
	write(sock_fd, request, strlen(request));
	print_msg(request, 0);

	err = read_single_msg(sock_fd, "227", &reply);
	if(err != OK)
		return err;

	uint8_t ip1;
	uint8_t ip2;
	uint8_t ip3;
	uint8_t ip4;
	uint8_t port_h;
	uint8_t port_l;
	err = sscanf(reply, "%*[^(](%hhu,%hhu,%hhu,%hhu,%hhu,%hhu).", &ip1, &ip2, &ip3, &ip4, &port_h, &port_l);

	if(err != 6)
		return 1;

	uint16_t retr_port = port_h * 256 + port_l;
	char retr_ip[15];
	snprintf(retr_ip, 15, "%u.%u.%u.%u", ip1, ip2, ip3, ip4);

	printf("IP: %s\n", retr_ip);
	printf("PORT: %u\n", retr_port);

	int retr_fd = 0;
	FILE* retr_file;
	err = open_socket(&retr_fd, &retr_file);
	if(err != OK)
		return err;

	err = connect_socket(retr_fd, retr_ip, retr_port);
	if(err != OK)
		return err;

	printf("------------- DOWNLOADING -------------\n");

	snprintf(request, 256, "RETR %s%s", ftp_info.filename, line_endings);
	write(sock_fd, request, strlen(request));
	print_msg(request, 0);

	err = read_two_step_msg(sock_fd, "150", "260");
	if(err != OK)
		return err;

	FILE* dl;
	char trash[1024];

	if ( (dl = fopen(ftp_info.filename, "wb")) == NULL )
	{
		perror("fopen:");
		return 3;
	}

	if(got_size == 0) {

		err = sscanf(reply, "%[^(](%ld", trash, &file_size);
		printf("%ld\n", file_size);
	}

	read_file_w_size(retr_fd, dl, file_size);

	fclose(dl);
	free(reply);
	close(retr_fd);
	close(sock_fd);
	free_ftp_info(&ftp_info);

	return OK;
}
