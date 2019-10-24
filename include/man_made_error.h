#ifndef __MAN_MADE_ERROR_H__
#define __MAN_MADE_ERROR_H__

typedef unsigned char uint8;

void add_bcc_error(uint8* frame);

void add_bcc2_error(uint8* frame, size_t frame_len);

void rm_bcc_error(uint8* frame);

void rm_bcc2_error(uint8* frame, size_t frame_len);

#endif /* __MAN_MADE_ERROR_H__*/


/*
	#ifdef DEBUG_MAIN_STUFFING
	uint8_t len = 11;
	uint8_t msg[] = {(uint8_t)'a',1,1,126,1,1,126,1,1,125,(uint8_t)'z'};
	uint8_t BCC2 = msg[0];
	for (size_t i = 1; i < len; i++)
	{
		BCC2 ^= msg[i];
	}

	size_t frame_len = FRAME_I_LEN + len;
	uint8_t buf[128] = {FLAG, A_SENDER, C_I_0, A_SENDER ^ C_I_0, (uint8_t)'a', 1,1,126,1,1,126,1,1,125, (uint8_t)'z', BCC2, FLAG};

	for (size_t i = 0; i < frame_len; i++)
		DEBUG_PRINT(("%02x ", buf[i]));
	DEBUG_PRINT(("\n"));

	byte_stuffing(buf, &frame_len);

	for (size_t i = 0; i < frame_len; i++)
		DEBUG_PRINT(("%02x ", buf[i]));
	DEBUG_PRINT(("\n"));

	byte_destuffing(buf, &frame_len);

	for (size_t i = 0; i < frame_len; i++)
		DEBUG_PRINT(("%02x ", buf[i]));
	DEBUG_PRINT(("\n"));
	#endif
*/