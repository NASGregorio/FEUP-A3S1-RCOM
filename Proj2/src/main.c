#include <arpa/inet.h>
#include <stdio.h>

#include "ftp_info.h"
#include "defs.h"


int main(int argc, char const *argv[])
{
    if(argc != 2)
    {
        printf("Usage:\ndownload ftp://[<user>:<password>@]<host>/<url-path>\n");
        return BAD_ARGS;
    }

    ftp_info_t ftp_info;
    int err = build_ftp_info(argv[1], &ftp_info);
	if(err != OK)
        return err;

    print_ftp_info(&ftp_info);

    free_ftp_info(&ftp_info);

    return OK;
}
