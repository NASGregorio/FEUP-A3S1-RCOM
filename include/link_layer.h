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

void byte_stuffing(uint8_t* frame_start, size_t *len);
void byte_destuffing(uint8_t* frame_start, size_t *len);

#endif /*__LINK_LAYER_H__*/
