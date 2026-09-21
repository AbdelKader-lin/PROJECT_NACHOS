#include "consoleopenfile.h"
#include "synchconsole.h"
#include "system.h"

ConsoleOpenFile::ConsoleOpenFile() : input(nullptr), output(nullptr) {
}

ConsoleOpenFile::ConsoleOpenFile(OpenFile* in, OpenFile* out) : input(in), output(out) {
}

ConsoleOpenFile::~ConsoleOpenFile() {
}

int ConsoleOpenFile::Read(char* buffer, int size) {
    if (input) {
        return input->Read(buffer, size);
    } 
    else {
        for (int i = 0; i < size; i++)
            buffer[i] = synchConsole->SynchGetChar();
        return size;
    }
}

int ConsoleOpenFile::Write(const char* buffer, int size) {
    if (output) {
        return output->Write(buffer, size);
    } else {
        for (int i = 0; i < size; i++)
            synchConsole->SynchPutChar(buffer[i]);
        return size;
    }
}