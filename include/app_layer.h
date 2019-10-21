#ifndef __APP_LAYER_H__
#define __APP_LAYER_H__

#include "link_layer.h"

int transferFile(int port, char* path);
int receiveFile(int port);

#endif