#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "tty_layer.h"
#include "defs.h"

int open_port(int port, int* fd)
{
	if(port < 0 || port > 2)
	{
		printf("Invalid port number: %d", port);
      	return BAD_ARGS;
	}

	char* portName;

	switch (port)
	{
	case 0:
		portName = TTYS0;
		break;
	case 1:
		portName = TTYS1;
		break;
	case 2:
		portName = TTYS2;
		break;
	default:
		break;
	}

	//Opening as R/W and not as controlling tty to not get killed if linenoise sends CTRL-C.
	*fd = open(portName, O_RDWR | O_NOCTTY);
    if (*fd <0)
	{
		perror("open");
		return OPEN_PORT_FAIL;
	}

	return OK;
}

int check_fd(int fd)
{
	// Check if fd is valid
    if( fcntl(fd, F_GETFD) == -1 && errno == EBADF )
    {
        perror("fcntl");
        return INVALID_FD;
    }
	return OK;
}

int set_port_attr(int fd, TERMIOS* oldtio, TERMIOS* newtio)
{
	if(check_fd(fd) != OK)
		return INVALID_FD;

	// save current port settings
	if ( tcgetattr(fd,oldtio) == -1)
	{
      perror("tcgetattr");
      return SET_ATTR_FAIL;
    }

	// zero out new settings struct
    bzero(newtio, sizeof(*newtio)); //TODO: memcpy instead of bzero

	// set new settings
    newtio->c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio->c_iflag = IGNPAR;
    newtio->c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio->c_lflag = 0;

    newtio->c_cc[VTIME]    = 5;   /* inter-character timer unused */
    newtio->c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */



  /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) pr�ximo(s) caracter(es)
  */


    //flushes both data received but not read and data written but not transmitted.
    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd,TCSANOW,newtio) == -1)
	{
      perror("tcsetattr");
      return SET_ATTR_FAIL;
    }

	return OK;
}

int restore_port_attr(int fd, TERMIOS* oldtio)
{
	if(check_fd(fd) != OK)
		return INVALID_FD;

	if (tcsetattr(fd,TCSANOW,oldtio) == -1)
	{
		perror("tcsetattr");
		return SET_ATTR_FAIL;
	}

	return OK;
}

int close_port(int fd)
{
	if(close(fd) != 0)
	{
		perror("close");
		return CLOSE_PORT_FAIL;
	}

	return OK;
}

int write_msg(int fd, uint8_t msg[], unsigned len, int* bw)
{
	if(check_fd(fd) != OK)
		return INVALID_FD;


	if ( (*bw = write(fd, msg, len)) < 0)
	{
		perror("write: ");
		return WRITE_FAIL;
	}

	#ifdef DEBUG_TTY_CALLS
	printf("Send %uB: ", *bw);
	for (unsigned i = 0; i < *bw; i++)
	    printf("%02x", msg[i]);
	printf("\n");
	#endif

	return OK;
}

int read_msg(int fd, uint8_t* msg, int* br, unsigned maxLength, int (*func)(void))
{
	if(check_fd(fd) != OK)
		return INVALID_FD;

	*br = 0;

	int res;
	uint8_t buf[1];


	int STOP = 0;
	while (STOP == FALSE)
	{
		if(func && (func() == EXIT_TIMEOUT))
			return EXIT_TIMEOUT;

		res = read(fd,buf,1);
		if (res == -1)
		{
			perror("read: ");
			return READ_FAIL;
		}

		if(res == 0)
			continue;

		if(*br == 0 && buf[0] != FLAG)
			continue;

		if(*br == 1 && msg[*br - 1] == FLAG && buf[0] == FLAG)
			continue;

		msg[*br] = buf[0];

		if( (*br > 1 && msg[*br - 1] != FLAG && msg[*br] == FLAG) || *br == maxLength )
		{
			STOP = TRUE;
		}

		*br += res;
    }

	#ifdef DEBUG_TTY_CALLS
	printf("Read %uB: ", *br);
	for (unsigned i = 0; i < *br; i++)
		printf("%02x", msg[i]);
	printf("\n");
	#endif

	return OK;
}
