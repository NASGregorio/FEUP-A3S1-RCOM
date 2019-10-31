#ifndef __TTY_LAYER_H__
#define __TTY_LAYER_H__

#include <termios.h>
#include "man_made_error.h"

///// --DEBUG-- /////

//#define ENABLE_DATA_PRINT
//#define ENABLE_DEBUG
#define ENABLE_TESTING


#ifdef ENABLE_TESTING
	#define ENABLE_HEAD_ERROR
	#define ENABLE_DATA_ERROR
	#define ENABLE_FRAME_DELAY
#endif

#ifdef ENABLE_DEBUG
    #define DEBUG_PRINT(x) printf x
#else
    #define DEBUG_PRINT(x) do {} while (0)
#endif
///// --DEBUG-- /////

typedef struct termios TERMIOS;
typedef unsigned char uint8_t;

int open_port(int port, int* fd);
int close_port(int fd);

int set_port_attr(int fd, TERMIOS* oldtio, TERMIOS* newtio, int baud);
int restore_port_attr(int fd, TERMIOS* oldtio);

int write_msg(int fd, uint8_t msg[], unsigned len, int* bw);
int read_msg(int fd, uint8_t* msg, int* br, unsigned maxLength, int (*func)(void));

#endif /*__TTY_LAYER_H__*/
