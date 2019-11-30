#include <arpa/inet.h>
#include <stdio.h>


#include <unistd.h>
#include <stdlib.h>
#include <string.h>


#include "ftp_info.h"
#include "socket_helper.h"
#include "defs.h"


// https://tools.ietf.org/html/rfc959


void print_msg(char* msg, size_t msg_len, uint8_t is_outgoing)
{
    if(is_outgoing == 0)
    {
        printf("  > %s", msg);
    }
    else
    {
        printf(" < ");
        printf("%.3s |%s | LEN: %ld\n", msg, &msg[3], msg_len);
    }
}


int main(int argc, char const *argv[])
{
    if(argc != 2)
    {
        printf("Usage:\ndownload ftp://[<user>:<password>@]<host>/<url-path>\n");
        return BAD_ARGS;
    }

    ftp_info_t ftp_info;
    int err = build_ftp_info(argv[1], &ftp_info);
	if(err != OK)
        return err;

    print_ftp_info(&ftp_info);

	int sock_fd = 0;
	FILE* sock_file;
	err = open_socket(&sock_fd, &sock_file);
	if(err != OK)
        return err;

	printf("------------- CONNECTION -------------\n");
	err = connect_socket(sock_fd, ftp_info.addr);
	if(err != OK)
        return err;

	char* reply;
	size_t msg_len;
    char code[4];
    char request[256];
    
	err = read_socket(sock_file, &reply, &msg_len);
	if(err != OK)
        return err;

    strncpy(code, reply, 3);
    print_msg(reply, msg_len, 1);
    free(reply);

    if(strncmp(code, "220", 3) != OK)
        return -1;

    printf("\n------------- AUTHENTIFICATION -------------\n");

    snprintf(request, 256, "USER %s\r\n", ftp_info.usr);
    write(sock_fd, request, strlen(request));
    print_msg(request, 0, 0);
    err = read_socket(sock_file, &reply, &msg_len);
	if(err != OK)
        return err;

    strncpy(code, reply, 3);
    print_msg(reply, msg_len, 1);
    free(reply);

    if(strncmp(code, "331", 3) != OK)
        return -1;

    snprintf(request, 256, "PASS %s\r\n", ftp_info.pwd);
    write(sock_fd, request, strlen(request));
    print_msg(request, 0, 0);
    err = read_socket(sock_file, &reply, &msg_len);
	if(err != OK)
        return err;

    strncpy(code, reply, 3);
    print_msg(reply, msg_len, 1);
    free(reply);

    if(strncmp(code, "230", 3) != OK)
        return -1;

    printf("\n------------- CHECK PATH -------------\n");

    memset(&request, 0, 256);
    snprintf(request, 256, "PASV\r\n");
    write(sock_fd, request, strlen(request));
    print_msg(request, 0, 0);
    err = read_socket(sock_file, &reply, &msg_len);
	if(err != OK)
        return err;

    strncpy(code, reply, 3);
    print_msg(reply, msg_len, 1);
    free(reply);

    if(strncmp(code, "230", 3) != OK)
        return -1;

    free_ftp_info(&ftp_info);

    return OK;
}
