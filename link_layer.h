#ifndef __LINK_LAYER_H__
#define __LINK_LAYER_H__

typedef enum com_mode
{
	TRANSMITTER,
	RECEIVER
} COM_MODE;

int llopen(int port, COM_MODE mode, int* port_fd);
int llwrite(int port, int* port_fd);
int llread(int port, int* port_fd);
int llclose(int port, int* port_fd);

#endif /*__LINK_LAYER_H__*/
