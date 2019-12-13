#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
//#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/sockios.h>

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

    // (*msg)[*msg_len-2] = '\0';
    // (*msg_len)-=2;

    return OK;
}

int read_msg(int sock_fd, FILE* sock_file, char** msg, size_t* msg_len, size_t* msg_buf_size)
{
    size_t pending = 0;
    usleep(200000);
    ioctl(sock_fd, SIOCINQ, &pending);


    printf("msg_buf_size - %ld\n", *msg_buf_size);

		//*msg = malloc(pending + 64);



		if(pending > *msg_buf_size)
		{
			size_t next_power_2 = *msg_buf_size;

			while(pending > next_power_2)
			{
				next_power_2 *= 2;
			}
			*msg_buf_size = next_power_2;

			*msg = realloc(*msg, *msg_buf_size);
		}

		memset(*msg, 0, *msg_buf_size);

		fprintf(stderr, "msg_buf_size: %ld\n", *msg_buf_size);


    char* temp;
		size_t len = 0;
		int err = 0;

    // int err = read_socket(sock_file, &temp, &len);
    // if(err != OK)
    //     return err;
		//
    // printf("msg    :%s\n", *msg);
		// printf("msg_len: %ld\n", *msg_len);
   	// printf("pending: %ld\n", pending);


    //pending -= (*msg_len);

    while ( pending > 0 )
    {
				fprintf(stderr, "pendingB: %ld\n", pending);
        err = read_socket(sock_file, &temp, &len);
        if(err != OK)
            return err;

        //printf("%s\n", temp);
        //printf("%ld\n", len);

        pending -= len;

        //printf("pending - %ld\n", pending);

        (*msg_len) += len;
        //*msg = realloc(*msg, *msg_len);
        strncat(*msg, temp, len);

        free(temp);
    }

		//fprintf(stderr, "pendingA: %ld\n", pending);

		*msg_len = strlen(*msg);

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


    // printf("\n\n");
    // for (size_t i = 0; i < 2048; i++)
    // {
    //     printf("%02x ");
    //     if(i != 0 && i % 16 == 0)
    //         printf("\n");
    // }
    // printf("\n\n");



}
