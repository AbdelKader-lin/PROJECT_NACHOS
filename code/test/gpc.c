#include "syscall.h"

int main() {
    int c = SynchGetChar();
    if (c == -1) {
        SynchPutChar('\n');
        return 0;
    }
    SynchPutChar((char)c);
    SynchPutChar('\n');
}