#ifndef __MAN_MADE_ERROR_H__
#define __MAN_MADE_ERROR_H__

typedef unsigned char uint8;

void init_rand(int ph, int pd, int delay);

int add_head_error(uint8* frame);

int add_data_error(uint8* frame, size_t frame_len);

void add_delay();

#endif /* __MAN_MADE_ERROR_H__*/
