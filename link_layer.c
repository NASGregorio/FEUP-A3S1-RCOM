
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define BAUDRATE B38400
#define FRAME_SU_LEN 5
#define TTYS0 "/dev/ttyS0"
#define TTYS1 "/dev/ttyS1"
#define TTYS2 "/dev/ttyS2"

#define TIMEOUT 3
#define MAX_RETRIES 3

#define OK 0
#define BAD_ARGS 1
#define OPEN_PORT_FAIL 2
#define SET_ATTR_FAIL 3
#define CLOSE_PORT_FAIL 4
#define EXIT_TIMEOUT 5

#define FALSE 0
#define TRUE 1

typedef __uint8_t uint_8;

typedef struct termios TERMIOS;

typedef enum com_mode
{
	TRANSMITTER,
	RECEIVER
} COM_MODE;

typedef enum frame_codes
{
	FLAG = 0x7E,
	A_SENDER = 0x03,
	A_RECEIVER = 0x01,
	C_SET = 0x03,
	C_DISC = 0x0B,
	C_UA = 0x07,
	C_RR_0 = 0x05,
	C_REJ_0 = 0x01,
	C_RR_1 = C_RR_0 | 0x80,
	C_REJ_1 = C_REJ_0 | 0x80
} FRAME;

const char *bit_rep[16] = {
    [ 0] = "0000", [ 1] = "0001", [ 2] = "0010", [ 3] = "0011",
    [ 4] = "0100", [ 5] = "0101", [ 6] = "0110", [ 7] = "0111",
    [ 8] = "1000", [ 9] = "1001", [10] = "1010", [11] = "1011",
    [12] = "1100", [13] = "1101", [14] = "1110", [15] = "1111",
};

void print_byte(int d, uint_8 byte)
{
    printf("%d: %s%s\n", d, bit_rep[byte >> 4], bit_rep[byte & 0x0F]);
}

unsigned retriesCount = 0;
unsigned timeout_exit = 0;
int port_fd;
int bytesWritten;
int bytesRead;
TERMIOS oldtio,newtio;


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

	*fd = open(portName, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (*fd <0)
	{
		perror("open");
		return OPEN_PORT_FAIL;
	}

	//fdopen com r+

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
    bzero(newtio, sizeof(*newtio));

	// set new settings
    newtio->c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio->c_iflag = IGNPAR;
    newtio->c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio->c_lflag = 0;

    newtio->c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio->c_cc[VMIN]     = 5;   /* blocking read until 5 chars received */



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
    printf("Sending %u bytes:\n", len);
	for (unsigned i = 0; i < len; i++)
	    printf(" %u: %x\n", i, msg[i]);
	
	*bw = write(fd, msg, len); //TODO: Erro handling

    printf("%d bytes written\n", *bw);

	return OK;
}

void exitOnTimeout()
{
	if(timeout_exit)
	{
		restore_port_attr(port_fd, &oldtio);
		close_port(port_fd);
		exit(-1);
	}
}

int read_msg(int fd, uint_8* msg, int* br, unsigned maxLength, void (*func)(void))
{
	*br = 0;

	int res;
	uint_8 buf[1];


	int STOP = 0;
	while (STOP == FALSE)
	{
		if(func) func();

		res = read(fd,buf,1);

		if(res == -1)
			continue;


		msg[*br] = buf[0];

		if( (*br > 1 && msg[*br - 1] != FLAG && msg[*br] == FLAG) || *br == maxLength )
		{
			STOP = TRUE;
		}
		
		*br += res;
    }

	return OK;
}

void timeoutHandler()
{
	if(retriesCount >= MAX_RETRIES)
	{
		timeout_exit = 1;
		return;
	}

	printf("Retrying...\n");

	uint_8 set_msg[FRAME_SU_LEN] = { FLAG, A_SENDER, C_SET, A_SENDER ^ C_SET, FLAG };
	write_msg(port_fd, set_msg, FRAME_SU_LEN, &bytesWritten);

	alarm(TIMEOUT);
	retriesCount++;
}

int llopen(int port, COM_MODE mode, int* port_fd)
{
	int err = open_port(port, port_fd);
	if(err != 0)
		return err;

	err = set_port_attr(*port_fd, &oldtio, &newtio);
	if(err != 0)
		return err;

	// int bytesWritten;
	// int bytesRead;
	uint_8 bcc;

	if(mode == TRANSMITTER)
	{
		// SEND SET
		uint_8 set_msg[FRAME_SU_LEN] = { FLAG, A_SENDER, C_SET, A_SENDER ^ C_SET, FLAG };
		write_msg(*port_fd, set_msg, FRAME_SU_LEN, &bytesWritten);


		// ENABLE TIMEOUT MECHANISM
		(void) signal(SIGALRM, timeoutHandler);
		alarm(TIMEOUT);

		// WAIT FOR UA
		uint_8 msg[255];
		read_msg(*port_fd, msg, &bytesRead, 255, exitOnTimeout);

		// DISABLE TIMEOUT MECHANISM
		alarm(0);

		printf("Got %d bytes:\n", bytesRead);
		for (unsigned i = 0; i < bytesRead; i++)
	    	printf(" %u: %x\n", i, msg[i]);

		bcc = msg[1] ^ msg[2];
		if(bcc != msg[3])
			printf("Error found in msg!\n");
	}
	else if(mode == RECEIVER)
	{
		// WAIT FOR SET
		uint_8 msg[255];
		read_msg(*port_fd, msg, &bytesRead, 255, NULL);

		printf("Got %d bytes:\n", bytesRead);
		for (unsigned i = 0; i < bytesRead; i++)
	    	printf(" %u: %x\n", i, msg[i]);

		bcc = msg[1] ^ msg[2];
		if(bcc != msg[3])
			printf("Error found in msg!\n");

		// SEND UA
		uint_8 ua_msg[FRAME_SU_LEN] = { FLAG, A_RECEIVER, C_UA, A_RECEIVER ^ C_UA, FLAG };
		write_msg(*port_fd, ua_msg, FRAME_SU_LEN, &bytesWritten);
	}
	else
	{
		//TODO: error handling
	}


	return OK;
}


int main(int argc, char const *argv[])
{
    if (argc < 3)
	{
      printf("Usage: link_layer <port number 0|1|2> <mode T|R>\n");
      return BAD_ARGS;
    }

	int portNumber = strtol(argv[1],  NULL, 10);

	COM_MODE mode;
	if(strcmp(argv[2], "T") == 0)
		mode = TRANSMITTER;
	else if(strcmp(argv[2], "R") == 0)
		mode = RECEIVER;
	else
		return BAD_ARGS;

	int err = llopen(portNumber, mode, &port_fd);
	if(err != OK)
		return err;

    return OK;
}
