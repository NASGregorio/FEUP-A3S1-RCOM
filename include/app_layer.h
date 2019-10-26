#ifndef __APP_LAYER_H__
#define __APP_LAYER_H__

#include "link_layer.h"

int transferFile(int port, char* path);
int receiveFile(int port);

uint8_t* controlPackageProc(int start, long fileSize, uint8_t *fileName, int fileNameSize);

uint8_t* dataPackageProc(uint8_t* msg, unsigned long fileSize, __uint32_t* packetSize);

#endif