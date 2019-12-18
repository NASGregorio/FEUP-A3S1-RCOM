#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <time.h>


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
	size_t timeout_sec = 30;
	size_t timeout_usec = 0;
	char request[256];
	char* line_endings = "\r\n";

	err = read_msg_block(sock_fd, "220");
	if(err != OK)
		return err;


	printf("------------- AUTHENTIFICATION -------------\n");

	snprintf(request, 256, "USER %s%s", ftp_info.usr, line_endings);
	write(sock_fd, request, strlen(request));
	print_msg(request, 0);

	err = read_single_msg(sock_fd, "331", NULL, timeout_sec, timeout_usec);
	if(err != OK)
		return err;

	snprintf(request, 256, "PASS %s%s", ftp_info.pwd, line_endings);
	write(sock_fd, request, strlen(request));
	print_msg(request, 0);

	err = read_single_msg(sock_fd, "230", NULL, timeout_sec, timeout_usec);
	if(err != OK)
		return err;




	printf("------------- SET TYPE -------------\n");

	snprintf(request, 256, "TYPE I%s", line_endings);
	write(sock_fd, request, strlen(request));
	print_msg(request, 0);

	err = read_single_msg(sock_fd, "200", NULL, timeout_sec, timeout_usec);
	if(err != OK)
		return err;




	printf("------------- GET SIZE -------------\n");

	snprintf(request, 256, "SIZE %s%s", ftp_info.url_path, line_endings);
	write(sock_fd, request, strlen(request));
	print_msg(request, 0);

	int got_size = 0;
	size_t file_size = 0;

	err = read_single_msg(sock_fd, "213", &reply, timeout_sec, timeout_usec);
	if(err == OK) {
		file_size = strtol(&reply[4], NULL, 10);
		got_size = 1;
		free(reply);
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

	err = read_single_msg(sock_fd, "227", &reply, timeout_sec, timeout_usec);
	if(err != OK)
		return err;

	uint8_t ip1;
	uint8_t ip2;
	uint8_t ip3;
	uint8_t ip4;
	uint8_t port_h;
	uint8_t port_l;
	err = sscanf(reply, "%*[^(](%hhu,%hhu,%hhu,%hhu,%hhu,%hhu).", &ip1, &ip2, &ip3, &ip4, &port_h, &port_l);

	free(reply);

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

	err = read_two_step_msg(sock_fd, "150", "226", timeout_sec, timeout_usec);
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

  	struct timespec start, end;

	clock_gettime(CLOCK_MONOTONIC, &start);

	read_file_w_size(retr_fd, dl, file_size);

	clock_gettime(CLOCK_MONOTONIC, &end);


	fseek(dl, 0L, SEEK_END);
	long received_size = ftell(dl);


	fclose(dl);
	free(reply);
	close(retr_fd);
	close(sock_fd);
	free_ftp_info(&ftp_info);

	long seconds = end.tv_sec - start.tv_sec; 
    long ns = end.tv_nsec - start.tv_nsec; 
    
    if (start.tv_nsec > end.tv_nsec) { // clock underflow 
		--seconds; 
		ns += 1000000000; 
    } 

	fflush(stdout);
	printf("Download: 100.0%% [====================]\n");
	printf("Expected size: %ld\n", file_size);
	printf("Received size: %ld\n", received_size);
    printf("Total time: %f\n", (double)seconds + (double)ns/(double)1000000000); 

	return OK;
}
