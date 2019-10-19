#include "defs.h"
#include "link_layer.h"

int llclose(int port_fd, TERMIOS* oldtio)
{
    restore_port_attr(port_fd, oldtio);
	close_port(port_fd);

    return OK;
}
