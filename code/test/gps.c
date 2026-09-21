#include "syscall.h"

int main() {
    const char str1[] = "Enter a string: \n";
    SynchPutString(str1);
    char c[300];
    SynchGetString(c, 300);
    SynchPutString(c);
    SynchPutChar('\n');
}