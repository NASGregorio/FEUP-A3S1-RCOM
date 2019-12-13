#include <arpa/inet.h>
#include <stdio.h>


#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ioctl.h>
#include <linux/sockios.h>


#include "ftp_info.h"
#include "socket_helper.h"
#include "defs.h"


// https://tools.ietf.org/html/rfc959


void print_msg(char* msg, uint8_t is_outgoing)
{
	if(is_outgoing == 0)
	{
		printf(">>>>>>\n");
		printf("%s\n", msg);
	}
	else
	{
		printf("<<<<<<\n");
		printf("%s\n\n", msg);
	}
}


int read_reply(int sock_fd, FILE* sock_file, char** reply, size_t* reply_len, size_t* reply_buf_size, char* code_str)
{
	char code[4];

	int err = read_msg(sock_fd, sock_file, reply, reply_len, reply_buf_size);
	if(err != OK)
		return err;

	strncpy(code, *reply, 3);
	print_msg(*reply, 1);

	if(strncmp(code, code_str, 3) != OK)
		return err;

	return OK;
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
	size_t reply_len;
	char request[256];
	char* line_endings = "\r\n";
	reply = malloc(256);
	size_t reply_buf_size = 256;

	err = read_reply(sock_fd, sock_file, &reply, &reply_len, &reply_buf_size, "220");
	if(err != OK)
		return err;


	

	printf("------------- AUTHENTIFICATION -------------\n");

	snprintf(request, 256, "USER %s%s", ftp_info.usr, line_endings);
	write(sock_fd, request, strlen(request));
	print_msg(request, 0);

	err = read_reply(sock_fd, sock_file, &reply, &reply_len, &reply_buf_size, "331");
	if(err != OK)
		return err;



	snprintf(request, 256, "PASS %s%s", ftp_info.pwd, line_endings);
	write(sock_fd, request, strlen(request));
	print_msg(request, 0);

	err = read_reply(sock_fd, sock_file, &reply, &reply_len, &reply_buf_size, "230");
	if(err != OK)
		return err;




	printf("------------- SET TYPE -------------\n");

	snprintf(request, 256, "TYPE I%s", line_endings);
	write(sock_fd, request, strlen(request));
	print_msg(request, 0);

	err = read_reply(sock_fd, sock_file, &reply, &reply_len, &reply_buf_size, "200");
	if(err != OK)
		return err;




	printf("------------- GET SIZE -------------\n");

	snprintf(request, 256, "SIZE %s%s", ftp_info.url_path, line_endings);
	write(sock_fd, request, strlen(request));
	print_msg(request, 0);

	err = read_reply(sock_fd, sock_file, &reply, &reply_len, &reply_buf_size, "213");
	if(err != OK)
		return err;

	size_t file_size = strtol(&reply[4], NULL, 10);




	printf("------------- CHANGE DIRECTORY -------------\n");

	snprintf(request, 256, "CWD /%s%s", ftp_info.path, line_endings);
	write(sock_fd, request, strlen(request));
	print_msg(request, 0);

	err = read_reply(sock_fd, sock_file, &reply, &reply_len, &reply_buf_size, "250");
	if(err != OK)
		return err;




	printf("------------- PASSIVE MODE -------------\n");

	snprintf(request, 256, "PASV%s", line_endings);
	write(sock_fd, request, strlen(request));
	print_msg(request, 0);

	err = read_reply(sock_fd, sock_file, &reply, &reply_len, &reply_buf_size, "227");
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
		return -1;

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

	err = read_reply(sock_fd, sock_file, &reply, &reply_len, &reply_buf_size, "150");
	if(err != OK)
		return err;

	if(strstr(reply, "226") == NULL)
		return -1;

	FILE* dl;
	int br;

	if ( (dl = fopen(ftp_info.filename, "wb")) == NULL )
	{
		perror("fopen:");
		return -1;
	}

	uint8_t* data = malloc(file_size);
	while ( (br = read(retr_fd, data, file_size)) )
	{
		if (br < 0)
		{
			printf("Download fail\n");
			return -1;
		}

		if ((br = fwrite(data, br, 1, dl)) < 0)
		{
			printf("Saving download fail\n");
			return -1;
		}
	}

	fclose(dl);
	free(reply);
	free(data);
	close(retr_fd);
	close(sock_fd);
	free_ftp_info(&ftp_info);

	return OK;
}
