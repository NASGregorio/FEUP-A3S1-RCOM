#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "tty_layer.h"
#include "defs.h"

int get_baud(int baud)
{
    switch (baud) {
    case 110:
        return B110;
	case 300:
        return B300;
	case 600:
        return B600;
    case 1200:
        return B1200;
	case 2400:
        return B2400;
    case 4800:
        return B4800;
    case 9600:
        return B9600;
    case 19200:
        return B19200;
    case 38400:
        return B38400;
    case 57600:
        return B57600;
    case 115200:
        return B115200;
    case 230400:
        return B230400;
    case 460800:
        return B460800;
    case 500000:
        return B500000;
    case 576000:
        return B576000;
    case 921600:
        return B921600;
    case 1000000:
        return B1000000;
    case 1152000:
        return B1152000;
    case 1500000:
        return B1500000;
    case 2000000:
        return B2000000;
    case 2500000:
        return B2500000;
    case 3000000:
        return B3000000;
    case 3500000:
        return B3500000;
    case 4000000:
        return B4000000;
    default: 
        return -1;
    }
}

int open_port(int port, int* fd)
{
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
		printf("Invalid port number: %d", port);
		return BAD_ARGS;
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

int set_port_attr(int fd, TERMIOS* oldtio, TERMIOS* newtio, int baud)
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
    bzero(newtio, sizeof(*newtio));

	int a = B38400;
	int b = get_baud(baud);
	printf("%d | %d \n", a, b);

	// set new settings
    newtio->c_cflag = b | CS8 | CLOCAL | CREAD;
    newtio->c_iflag = IGNPAR;
    newtio->c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio->c_lflag = 0;

    newtio->c_cc[VTIME]    = 5;   /* inter-character timer unused */
    newtio->c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */



  /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) proximo(s) caracter(es)
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

	#ifdef ENABLE_DEBUG
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

	#ifdef ENABLE_DEBUG
	printf("Read %uB: ", *br);
	for (unsigned i = 0; i < *br; i++)
		printf("%02x", msg[i]);
	printf("\n");
	#endif

	return OK;
}
