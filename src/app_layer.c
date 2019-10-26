#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "app_layer.h"
#include "defs.h"


//uint8_t package[];

uint8_t* control_package_protocol(int start, long fileSize, uint8_t *fileName, int fileNameSize){

    uint8_t* package[13 + fileNameSize];

    
    if(start == 1)
        package[0] = (uint8_t)CP_START;

    else
        package[0] = CP_END;

   package[1] = (uint8_t)T_FILESIZE;
   package[2] = (uint8_t)L_LONG;
   package[3] = ((fileSize >> 56) & 0xFF);           //shift superior than variable's type size
   package[4] = ((fileSize >> 48) & 0xFF);
   package[5] = ((fileSize >> 40) & 0xFF);
   package[6] = ((fileSize >> 32) & 0xFF);
   package[7] = ((fileSize >> 24) & 0xFF);
   package[8] = ((fileSize >> 16) & 0xFF);
   package[9] = ((fileSize >> 8) & 0xFF);
   package[10] = (fileSize & 0xFF);
   package[11] = (uint8_t)T_FILENAME;
   package[12] = (uint8_t)fileNameSize;

    for(int i = 0; i < fileNameSize; i++){
        &package[13+i] = fileName[i];
    }

    return *package;
}

uint8_t* dataPackageProc(uint8_t* msg, unsigned long fileSize, __uint32_t* packetSize){
    uint8_t* package[] = (uint8_t)malloc(fileSize + 4);

    package[0] = CP_DATA;
    package[1] = num_msg % 256;                             // undeclared variable num_msg
    package[2] = (uint8_t)((int)packetSize / 256);          // super janky solution??
    package[3] =  (uint8_t)((int)packetSize % 256);
    memccpy(package + 4, msg, *packetSize);                 // missing arguments destiny and source

    *packetSize += 4;
    num_msg++;
    return *package;
}

int transfer_file(int port, char *path){

    //The file we want to send is opened in read only mode
    FILE* file = fopen(path, "r");
    if(file == NULL){
        printf("Failed to open file\n");
        return FOPEN_FAIL;
    }
    
    int port_fd;
	TERMIOS oldtio;
    //uint8_t* buffer;

    //The port is opened to begin transfering the file
    int err = llopen(port, 0, &port_fd, &oldtio);
    if(err != OK){
        printf("Failed to establish connection\n");
		return err;
    }

    //The port is closed
    err = llclose(&oldtio, 0);
	if(err != OK)
		return err;

    if(fclose(file) != 0){
        printf("Failed to close file\n");
        return FCLOSE_FAIL;
    }

    return OK;
}

int receive_file(int port){

    int port_fd;
	TERMIOS oldtio;
    //The port is opened to begin receiving the file
    int err = llopen(port, 1, &port_fd, &oldtio);
    if(err != OK){
        printf("Failed to establish connection\n");
		return err;
    }

    //The port is closed
    err = llclose(&oldtio, 1);
	if(err != OK)
		return err;

    return OK;
}