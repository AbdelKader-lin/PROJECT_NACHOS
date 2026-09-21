#include "syscall.h"

void print(void *arg) {
    int i = *(int *)arg;
    SynchPutInt(i);
    SynchPutString("\n");
    if (i % 2) {
        SynchPutString("I am odd\n");
    } else {
        SynchPutString("I am even\n");
    }
    UserThreadExit();
}

int main() {
    static int a = 1;
    static int b = 2;
    static int c = 3;
    static int d = 4;
    UserThreadCreate(print, (void *)&a);
    UserThreadCreate(print, (void *)&b);
    UserThreadCreate(print, (void *)&c);
    UserThreadCreate(print, (void *)&d);
    UserThreadExit();
}
