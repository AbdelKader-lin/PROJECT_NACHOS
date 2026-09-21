#ifndef CONSOLE_OPEN_FILE_H
#define CONSOLE_OPEN_FILE_H

#include "openfile.h"

class ConsoleOpenFile : public OpenFile {
private:
    OpenFile* input;
    OpenFile* output;

public:
    ConsoleOpenFile();

    ConsoleOpenFile(OpenFile* in, OpenFile* out);

    ~ConsoleOpenFile();

    int Read(char* buffer, int size) override;
    int Write(const char* buffer, int size) override;
};

#endif // CONSOLEOPENFILE_H