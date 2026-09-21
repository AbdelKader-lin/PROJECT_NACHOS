#ifndef FTPLITE_H
#define FTPLITE_H

#include "tcplite.h"

#define FTP_MAX_DATA_SIZE 512
#define FTP_FILENAME_SIZE 20

enum {
    FTP_OK = 1,
    FTP_ERROR = 2,
    FTP_DONE = 3
};


typedef struct FileMetadata {
    char filename[FTP_FILENAME_SIZE];
    int filesize;
} FileMetadata;

class FtpLite {
public:
    FtpLite(TcpLite* tcp);

    // client
    bool SendFile(int farAddr, char* filename);

    // server
    void Serve();

private:
    TcpLite* tcp;
};

#endif
