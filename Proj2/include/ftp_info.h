#ifndef __FTP_INFO__
#define __FTP_INFO__

typedef struct sockaddr_in sockaddr_in_t;

typedef enum {
    FTP_ANON,
    FTP_USER
} ftp_type_t;

typedef struct {
    char* usr;
    char* pwd;
    char* location;
    char* host;
    char* addr;
    char* url_path;
    char* path;
    char* filename;
    ftp_type_t type;
} ftp_info_t;


int build_ftp_info(const char* full_url, ftp_info_t* ftp_info);

void print_ftp_info(ftp_info_t* ftp_info);

void free_ftp_info(ftp_info_t* ftp_info);

#endif /*__FTP_INFO__*/
