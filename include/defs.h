#ifndef __DEFS_H__
#define __DEFS_H__

#define BAUDRATE B38400
#define FRAME_SU_LEN 5
#define FRAME_I_LEN 8
#define TTYS0 "/dev/ttyS0"
#define TTYS1 "/dev/ttyS1"
#define TTYS2 "/dev/ttyS2"

#define BCC_CHECK(a, c, bcc) ((bcc) == ((a) ^ (c)))

//#define ENABLE_STUFF

#define DEBUG_TTY_CALLS
#ifdef DEBUG_TTY_CALLS
    #define DEBUG_PRINT(x) printf x
#else
    #define DEBUG_PRINT(x) do {} while (0)
#endif

#define TIMEOUT 3
#define MAX_RETRIES 3

#define MAX_FRAME_LEN 512

#define OK 0
#define BAD_ARGS 1
#define OPEN_PORT_FAIL 2
#define SET_ATTR_FAIL 3
#define CLOSE_PORT_FAIL 4
#define EXIT_TIMEOUT 5
#define INVALID_FD 6
#define WRITE_FAIL 7
#define READ_FAIL 8
#define BCC_ERROR 9
#define BCC2_ERROR 10
#define DISC_CONN 11
#define DUP_FRAME 12

#define BYTE_XOR 0x20
#define ESCAPE 0x7d
#define FLAG 0x7E
#define A_SENDER 0x03
#define A_RECEIVER 0x01
#define C_SET 0x03
#define C_DISC 0x0B
#define C_UA 0x07
#define C_RR_0 0x05
#define C_REJ_0 0x01
#define C_I_0 0x00
#define C_RR_1 (C_RR_0 | 0x80)
#define C_REJ_1 (C_REJ_0 | 0x80)
#define C_I_1 (C_I_0 | 0x40)

#define FALSE 0
#define TRUE 1

#endif /*__DEFS_H__*/



/*
///////////////////////////////////////////////////////////////////
const char *bit_rep[16] = {
    [ 0] = "0000", [ 1] = "0001", [ 2] = "0010", [ 3] = "0011",
    [ 4] = "0100", [ 5] = "0101", [ 6] = "0110", [ 7] = "0111",
    [ 8] = "1000", [ 9] = "1001", [10] = "1010", [11] = "1011",
    [12] = "1100", [13] = "1101", [14] = "1110", [15] = "1111",
};

void print_byte(int d, uint8_t byte)
{
    printf("%d: %s%s\n", d, bit_rep[byte >> 4], bit_rep[byte & 0x0F]);
}
///////////////////////////////////////////////////////////////////
*/