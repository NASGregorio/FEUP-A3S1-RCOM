#ifndef __TTY_LAYER_H__
#define __TTY_LAYER_H__

#include <termios.h>

typedef struct termios TERMIOS;
typedef __uint8_t uint_8;

int open_port(int port, int* fd);
int close_port(int fd);

int set_port_attr(int fd, TERMIOS* oldtio, TERMIOS* newtio);
int restore_port_attr(int fd, TERMIOS* oldtio);

int write_msg(int fd, uint_8 msg[], unsigned len, int* bw);
int read_msg(int fd, uint_8* msg, int* br, unsigned maxLength, void (*func)(void));

#endif /*__TTY_LAYER_H__*/
