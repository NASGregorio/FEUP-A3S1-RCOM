#ifndef __TTY_LAYER_H__
#define __TTY_LAYER_H__

#include <termios.h>

typedef struct termios TERMIOS;
typedef unsigned char uint8_t;

int open_port(int port, int* fd);
int close_port(int fd);

int set_port_attr(int fd, TERMIOS* oldtio, TERMIOS* newtio);
int restore_port_attr(int fd, TERMIOS* oldtio);

int write_msg(int fd, uint8_t msg[], unsigned len, int* bw);
int read_msg(int fd, uint8_t* msg, int* br, unsigned maxLength, int (*func)(void));

#endif /*__TTY_LAYER_H__*/
