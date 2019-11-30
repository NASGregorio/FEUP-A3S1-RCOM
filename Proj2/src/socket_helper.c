#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>

#include "defs.h"
#include "socket_helper.h"

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

int connect_socket(int sock_fd, char* addr)
{
    struct sockaddr_in server_addr;

    /*server address handling*/
    memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(addr);	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(21);               /*server TCP port must be network byte ordered */

    if(connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect()");
        return CONNECT_SOCKET_ERROR;
    }
    return OK;
}

int read_socket(FILE* sock_file, char** msg, size_t* msg_len)
{
    size_t buf_size = 256;

    char buf[buf_size];
    size_t buf_len;

    *msg = malloc(buf_size);
    fgets(*msg, buf_size, sock_file);
    *msg_len = strlen(*msg);

    while ((*msg)[*msg_len - 2] != '\r' && (*msg)[*msg_len - 1] != '\n')
    {
        fgets(buf, buf_size+1, sock_file);
        buf_len = strlen(buf);

        *msg_len += buf_len;
        *msg = realloc(*msg, *msg_len);
        strcat(*msg, buf);
    }

    (*msg)[*msg_len-2] = '\0';
    (*msg_len)-=2;

    return OK;
}

int write_socket()
{
    return OK;

}

int close_socket()
{
    return OK;

}