#ifndef __SOCKET_HELPER__
#define __SOCKET_HELPER__

int open_socket(int* sock_fd, FILE** sock_file);

int connect_socket(int sock_fd, char* addr, uint16_t port);

int read_file_w_size(int retr_fd, FILE* dl, size_t file_size);

int read_msg_block(int sock_fd, char* code_str);

int read_single_msg(int sock_fd, char* code_str, char** msg, size_t tsec, size_t tusec);

int write_msg(int sock_fd, char* cmd, ...);

#endif /*__SOCKET_HELPER__*/
