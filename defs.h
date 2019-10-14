#ifndef __DEFS_H__
#define __DEFS_H__

#define BAUDRATE B38400
#define FRAME_SU_LEN 5
#define FRAME_I_LEN 8
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

#define FLAG 0x7E
#define A_SENDER 0x03
#define A_RECEIVER 0x01
#define C_SET 0x03
#define C_DISC 0x0B
#define C_UA 0x07
#define C_RR_0 0x05
#define C_REJ_0 0x01
#define C_RR_1 (C_RR_0 | 0x80)
#define C_REJ_1 (C_REJ_0 | 0x80)

#define FALSE 0
#define TRUE 1

#endif /*__DEFS_H__*/
