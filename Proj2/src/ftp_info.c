#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netdb.h>
#include <arpa/inet.h>

#include "ftp_info.h"
#include "defs.h"


const char* valid_login = "abcdefghijklmnopqrstuvxywzABCDEFGHIJKLMNOPQRSTUVXYWZ0123456789@.";
const char* valid_url = "abcdefghijklmnopqrstuvxywzABCDEFGHIJKLMNOPQRSTUVXYWZ0123456789-_./";

typedef struct addrinfo addrinfo_t;
typedef struct sockaddr_in sockaddr_in_t;

#define EXPECTED_URL_COUNT 4

int host_to_ipv4(const char* hostname, ftp_info_t* ftp_info)
{

    addrinfo_t hints;
    addrinfo_t* results;

    memset(&hints, 0, sizeof(addrinfo_t));
    hints.ai_family = AF_INET;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = 0; /* Datagram socket */
    hints.ai_flags = AI_NUMERICSERV | AI_PASSIVE;    /* For wildcard IP address */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    // Translate host to ip address
    int err = getaddrinfo(hostname, "21", &hints, &results);
    if (err != OK) {
        //fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
        printf("Failed to resolve host name\n");
        return GETADDRINFO_ERROR;
    }

    // Copy address to ftp_info_t
    sockaddr_in_t* addr = (sockaddr_in_t*)results->ai_addr;
    char* addr_str = inet_ntoa((struct in_addr)addr->sin_addr);
    size_t len = strlen(addr_str);
	ftp_info->addr = (char*) malloc(len);
	memcpy(ftp_info->addr, addr_str, len);

    // Try to translate address to host (in case user inserted ip address directly)
    char hbuf[256], sbuf[8];
    err = getnameinfo((struct sockaddr*) addr, 16, hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICSERV);
    if (err == OK) {
        len = strlen(hbuf);
        ftp_info->host = (char*) malloc(len);
        memcpy(ftp_info->host, hbuf, len);
    }
    else
    {
        ftp_info->host = (char*) malloc(4);
        memcpy(ftp_info->host, "N/A", 4);
    }

    // Free memory used by getaddrinfo
    freeaddrinfo(results);


	//struct hostent *h;
    // if ((h=gethostbyname(hostname)) == NULL) {  
    //     herror("gethostbyname");
    //     exit(1);
    // }
    // printf("Host name  : %s\n", h->h_name);
    // printf("IP Address : %s\n",inet_ntoa(*((struct in_addr *)h->h_addr)));

    return OK;
}

int validade_string(char* str, const char* valid, char* field_name)
{
    if (str[strspn(str, valid)] == '\0')
        return OK;

    printf("INVALID %s -> %s\n", field_name, str);
    return INVALID_CHAR;
}

int build_ftp_info(const char* full_url, ftp_info_t* ftp_info)
{

    char usr[256] = {0};
    char pwd[256] = {0};
    char location[256] = {0};
    char url_path[256] = {0};
    char prompt[300] = {0};

    // Try User and password
    int i = sscanf(full_url, "ftp://%255[^:]:%255[^@]@%255[^/]%255s", usr, pwd, location, url_path);
    if (i != 4)
    {
        // Try User only
        memset(usr, 0, 256);
        memset(pwd, 0, 256);
        memset(location, 0, 256);
        memset(url_path, 0, 256);
        i = sscanf(full_url, "ftp://%255[^@]@%255[^/]%255s", usr, location, url_path);
        if(i != 3)
        {
            // Try Anonymous mode
            memset(usr, 0, 256);
            memset(location, 0, 256);
            memset(url_path, 0, 256);
            i = sscanf(full_url, "ftp://%255[^/]%255s", location, url_path);
            if (i != 2)
            {
                printf("Invalid URL format. Try:\n");
                printf("download ftp://<user>:<password>@<host>/<url-path>\n");
                printf("download ftp://<user>@<host>/<url-path>\n");
                printf("download ftp://<host>/<url-path>\n");
                return INVALID_URL_FORMAT;
            }
            else
            {
                strncpy(usr, "anonymous", 9);
                strncpy(pwd, "anonymous", 9);
            }
        }
        else
        {
            sprintf(prompt, "Type password for user %s: ", usr);
            char* pass = getpass(prompt);
            if(pass == NULL)
            {
                printf("Invalid password\n");
                return INVALID_URL_FORMAT; 
            }
            strncpy(pwd, pass, strlen(pass));
        }
    }
    

	// Invalid characters
    if (validade_string(usr, valid_login, "USER") != OK ||
        validade_string(pwd, valid_login, "PASSWORD") != OK ||
        validade_string(location, valid_url, "HOST/ADDR") != OK ||
        validade_string(url_path, valid_url, "PATH") != OK)
	{ return INVALID_CHAR; }

	size_t len = strlen(usr);
	ftp_info->usr = (char*) malloc(len);
	memcpy(ftp_info->usr, usr, len);

	len = strlen(pwd);
	ftp_info->pwd = (char*) malloc(len);
	memcpy(ftp_info->pwd, pwd, len);

	len = strlen(location);
	ftp_info->location = (char*) malloc(len);
	memcpy(ftp_info->location, location, len);

	len = strlen(url_path);
	ftp_info->url_path = (char*) malloc(len);
	memcpy(ftp_info->url_path, url_path, len);

	char* filename = strrchr(url_path, '/');
	if(filename == NULL) {
		printf("Format validation failed?\n");
		return INVALID_URL_FORMAT;
	}

	size_t filename_len = strlen(filename);
	ftp_info->path = (char*) malloc(len - filename_len);
	memcpy(ftp_info->path, url_path, len - filename_len); //sprintf(ftp_info->path, "./%s", url_path)

	ftp_info->filename = (char*) malloc(filename_len);
	memcpy(ftp_info->filename, &url_path[len - filename_len + 1], filename_len);

    ftp_info->type = ( strncmp(ftp_info->usr, "anonymous", 10) == 0 ) ? FTP_ANON :  FTP_USER;

    return host_to_ipv4(ftp_info->location, ftp_info);
}

void print_ftp_info(ftp_info_t* ftp_info)
{
    if(ftp_info == NULL)
        return;

    printf("\n--------------------------------------------------------------------------------\n");
    printf("|TYPE     : %.68s\n", (ftp_info->type == 0 ? "ANOMYMOUS" : "LOGIN"));
	printf("|USR      : %.68s\n", ftp_info->usr);
	printf("|PWD      : %.68s\n", ftp_info->pwd);
	printf("|URL      : %.68s\n", ftp_info->url_path);
	printf("|PATH     : %.68s\n", ftp_info->path);
	printf("|FILENAME : %.68s\n", ftp_info->filename);
	printf("|LOCATION : %.68s\n", ftp_info->location);
	printf("|HOST     : %.68s\n", ftp_info->host);
	printf("|ADDR     : %.68s\n", ftp_info->addr);
    printf("--------------------------------------------------------------------------------\n\n");
}

void free_ftp_info(ftp_info_t* ftp_info)
{
    if(ftp_info == NULL)
        return;

    if(ftp_info->usr)
        free(ftp_info->usr);
    if(ftp_info->pwd)
        free(ftp_info->pwd);
    if(ftp_info->location)
        free(ftp_info->location);
    if(ftp_info->host)
        free(ftp_info->host);
    if(ftp_info->addr)
        free(ftp_info->addr);
    if(ftp_info->url_path)
        free(ftp_info->url_path);
    if(ftp_info->path)
        free(ftp_info->path);
    if(ftp_info->filename)
        free(ftp_info->filename);
}