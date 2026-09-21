#include "syscall.h"
#define THIS "xxx"
#define THAT "yyy"
const int N = 2; // Choose it large enough!

void puts(char *s) {
	char *p;
	for (p = s; *p != '\0'; p++) {
        SynchPutChar(*p);
    }
}

void f(void *s) {
	for (int i = 0; i < N; i++) {
        puts((char *)s);
    }
    UserThreadExit();
}

int main() {
	int threadId = UserThreadCreate(f, (void *) THIS);
	f((void*) THAT);
    UserThreadJoin(threadId);
}