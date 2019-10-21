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

int llwrite(uint8_t* buf, size_t len);
int llread();

int byte_stuffing(size_t* new_len, uint8_t* msg, size_t len, size_t* extra);
int byte_destuffing(size_t* new_len, uint8_t* msg, size_t len, size_t* extra);

#endif /*__LINK_LAYER_H__*/
