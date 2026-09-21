#include "ftplite.h"
#include "system.h"
#include "stats.h"   // stats->totalTicks

FtpLite::FtpLite(TcpLite* tcp_) {
    tcp = tcp_;
}

bool FtpLite::SendFile(int farAddr, char* filename) {
    OpenFile* file = fileSystem->Open(filename);
    if (file == NULL) {
        return false;
    }
    FileMetadata metadata;
    metadata.filesize = file->Length();
    bcopy(filename, metadata.filename, FTP_FILENAME_SIZE);
    printf("Sending file: %s, size: %d bytes\n", metadata.filename, metadata.filesize);

    // Protocol:
    PacketHeader pktHdr;
    MailHeader mailHdr;
    TcpLiteHeader tcpHdr;
    pktHdr.to = farAddr;

    int startTicks = stats->totalTicks;
    tcpHdr.length = sizeof(FileMetadata);
    tcp->Send(pktHdr, mailHdr, tcpHdr, (char*)&metadata);
    
    char buffer[FTP_MAX_DATA_SIZE];
    bzero(buffer, FTP_MAX_DATA_SIZE);
    int bytesRead;
    
    tcp->Receive(&pktHdr, &mailHdr, &tcpHdr, buffer); // wait for server ready signal
    int FTPStatus = atoi(buffer); // ignore content
    if (FTPStatus != FTP_OK) {
        // printf("Server not ready to receive file\n");
        return false;
    } else {
        // printf("Server ready to receive file\n");
    }

    pktHdr.to = farAddr;

    while ((bytesRead = file->Read(buffer, FTP_MAX_DATA_SIZE)) > 0) {
        tcpHdr.length = bytesRead;
        tcp->Send(pktHdr, mailHdr, tcpHdr, buffer);
    }

    int eof = 0;
    tcpHdr.length = sizeof(int);
    tcp->Send(pktHdr, mailHdr, tcpHdr, (char*)&eof);

    bzero(buffer, FTP_MAX_DATA_SIZE);
    tcp->Receive(&pktHdr, &mailHdr, &tcpHdr, buffer); // wait for server ready signal
    int endTicks = stats->totalTicks;
    FTPStatus = atoi(buffer); // ignore content
    if (FTPStatus != FTP_DONE) {
        printf("File transfer failed\n");
        return false;
    } 
    int ticks = endTicks - startTicks;
    printf("Transfer: %d bytes in %d ticks\n", metadata.filesize, ticks);
    return true;
}

void FtpLite::Serve() {
    // Receive file name 
    char *filename = new char[FTP_FILENAME_SIZE];
    bzero(filename, FTP_FILENAME_SIZE);

    PacketHeader pktHdr;
    MailHeader mailHdr;
    TcpLiteHeader tcpHdr;
    char *buffer = new char[FTP_MAX_DATA_SIZE];

    tcp->Receive(&pktHdr, &mailHdr, &tcpHdr, buffer);
    
    FileMetadata* metadata = (FileMetadata*) buffer;
    bcopy(metadata->filename, filename, FTP_FILENAME_SIZE);
    bcopy(".ftp", filename + strlen(filename), 5); // append .ftp
    fileSystem->Create(filename, metadata->filesize);
    OpenFile* file = fileSystem->Open(filename);
    
    bzero(buffer, FTP_MAX_DATA_SIZE);

    //Prepare response header
    int from = pktHdr.from, to = pktHdr.to;
    pktHdr.from = to;
    pktHdr.to = from;
    if (file == nullptr) {
        tcpHdr.length = sprintf(buffer, "%d", FTP_ERROR);
        tcp->Send(pktHdr, mailHdr, tcpHdr, buffer);
        printf("Failed to create file: %s\n", filename);
        return;
    } else {
        tcpHdr.length = sprintf(buffer, "%d", FTP_OK);
        tcp->Send(pktHdr, mailHdr, tcpHdr, buffer);
    }
    while (true) {
        bzero(buffer, FTP_MAX_DATA_SIZE);
        int recevied = tcp->Receive(&pktHdr, &mailHdr, &tcpHdr, buffer);
        if (strlen(buffer) == 0) { // EOF
            break;
        }
        file->Write(buffer, recevied);
}

    // Send done signal
    bzero(buffer, FTP_MAX_DATA_SIZE);
    from = pktHdr.from, to = pktHdr.to;
    pktHdr.from = to;
    pktHdr.to = from;
    tcpHdr.length = sprintf(buffer, "%d", FTP_DONE);
    tcp->Send(pktHdr, mailHdr, tcpHdr, buffer); 

    printf("File reception completed: %s\n", filename);
}
