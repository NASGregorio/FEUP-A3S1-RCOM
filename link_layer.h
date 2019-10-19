#ifndef __LINK_LAYER_H__
#define __LINK_LAYER_H__

#include "tty_layer.h"

typedef enum com_mode
{
	TRANSMITTER,
	RECEIVER
} COM_MODE;

int llopen(int port, COM_MODE mode, int* port_fd, TERMIOS* oldtio);
int llclose(int port_fd, TERMIOS* oldtio);

int llwrite(int port, int* port_fd);
int llread(int port, int* port_fd);

#endif /*__LINK_LAYER_H__*/
