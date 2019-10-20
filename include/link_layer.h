#ifndef __LINK_LAYER_H__
#define __LINK_LAYER_H__

#include "tty_layer.h"

typedef enum com_mode
{
	TRANSMITTER,
	RECEIVER
} COM_MODE;

int llopen(int port, COM_MODE mode, int* fd, TERMIOS* oldtio);
int llclose(TERMIOS* oldtio, COM_MODE mode);

int llwrite(uint8_t* buf, int len);
int llread();

extern int return_on_timeout();

#endif /*__LINK_LAYER_H__*/
