#ifndef __TTY_LAYER_H__
#define __TTY_LAYER_H__

#include <termios.h>

///// --DEBUG-- /////
//#define ENABLE_DATA_PRINT
//#define ENABLE_DEBUG
#ifdef ENABLE_DEBUG
	#include "man_made_error.h"
    #define DEBUG_PRINT(x) printf x
	//#define ENABLE_BBC_ERROR
	//#define ENABLE_BCC2_ERROR
#else
    #define DEBUG_PRINT(x) do {} while (0)
#endif
///// --DEBUG-- /////

typedef struct termios TERMIOS;
typedef unsigned char uint8_t;

int open_port(int port, int* fd);
int close_port(int fd);

int set_port_attr(int fd, TERMIOS* oldtio, TERMIOS* newtio);
int restore_port_attr(int fd, TERMIOS* oldtio);

int write_msg(int fd, uint8_t msg[], unsigned len, int* bw);
int read_msg(int fd, uint8_t* msg, int* br, unsigned maxLength, int (*func)(void));

#endif /*__TTY_LAYER_H__*/
