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

int transferFile(int port, char *path) {

    //The file we want to send is opened in read only mode
    FILE* file = fopen(path, "r");
    if(file == NULL) {
        printf("Failed to open file");
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
        printf("Failed to close file");
        return FCLOSE_FAIL;
    }

    return OK;
}

int receiveFile(int port){

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