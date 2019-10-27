#ifndef __APP_LAYER_H__
#define __APP_LAYER_H__

#include "link_layer.h"

void string2ByteArray(char* input, uint8_t* output, size_t len);

int main(int argc, char const *argv[]);

int transferFile(int port, char* path);
int receiveFile(int port);

int fileSizeBytes(uint8_t* sizeBytes);

int control_package_protocol(int start, uint8_t *fileName, uint8_t fileNameSize, uint8_t sizeBytes, uint8_t* package);

int dataPackageProc(uint8_t* msg, __uint32_t* packetSize, uint8_t* package);

#endif