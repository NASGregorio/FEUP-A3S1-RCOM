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

int set_port_attr(int fd, TERMIOS* oldtio, TERMIOS* newtio)
{
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

int write_msg(int fd, uint_8 msg[], unsigned len, int* bw)
{
	if ( (*bw = write(fd, msg, len)) < 0)  //TODO: Erro handling
	{
		perror("write: ");
		return WRITE_FAIL;
	}

	#ifdef DEBUG_TTY_CALLS
	printf("Send %uB: ", *bw);
	for (unsigned i = 0; i < *bw; i++)
	    printf("%x", msg[i]);
	printf("\n");
	#endif

	return OK;
}

int read_msg(int fd, uint_8* msg, int* br, unsigned maxLength, int (*func)(void))
{
	*br = 0;

	int res;
	uint_8 buf[1];


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
		printf("%x", msg[i]);
	printf("\n");
	#endif

	return OK;
}
