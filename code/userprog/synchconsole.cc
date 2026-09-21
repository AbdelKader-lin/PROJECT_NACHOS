#ifdef CHANGED

#include <string.h>

#include "copyright.h"
#include "system.h"
#include "synchconsole.h"
#include "synch.h"

static Semaphore *readAvail;
static Semaphore *writeDone;
static Semaphore *putStringMutex;

static void ReadAvail(int arg) { readAvail->V(); }
static void WriteDone(int arg) { writeDone->V(); }

SynchConsole::SynchConsole(char *readFile, char *writeFile) {
    readAvail = new Semaphore("read avail", 0);
    writeDone = new Semaphore("write done", 0);
    putStringMutex = new Semaphore("put string mutex", 1);

    console = new Console(readFile, writeFile, ReadAvail, WriteDone, 0);
}

SynchConsole::~SynchConsole() {
    delete console;
    delete writeDone;
    delete readAvail;
}

void SynchConsole::SynchPutChar(const char ch) {
    putStringMutex->P();
    console->PutChar(ch);
    writeDone->P();
    putStringMutex->V();
}

int SynchConsole::SynchGetChar() {
    readAvail->P();
    char ch = console->GetChar();
    return (int)ch;
}

void SynchConsole::SynchPutString(const char s[]) {
    putStringMutex->P();
    for (int i = 0; i < MAX_STRING_SIZE && s[i] != '\0'; i++) {
        console->PutChar(s[i]);
        writeDone->P();
    }
    putStringMutex->V();
}

void SynchConsole::SynchGetString(char *s, int n) {
    for (int i = 0; i < n - 1; i++) {
        int ch = SynchGetChar();
        if (ch == -1 || ch == '\n') {
            s[i] = '\0';
            return;
        }
        s[i] = (char)ch;
    }
    s[n - 1] = '\0';
}
#endif // CHANGED