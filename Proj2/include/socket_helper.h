#ifndef __SOCKET_HELPER__
#define __SOCKET_HELPER__

int open_socket(int* sock_fd, FILE** sock_file);

int connect_socket(int sock_fd, char* addr);

int read_socket(FILE* sock_file, char** msg, size_t* msg_len);

#endif /*__SOCKET_HELPER__*/
