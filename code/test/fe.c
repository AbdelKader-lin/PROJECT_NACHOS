#include "syscall.h"

int main() {
    ForkExec("u0");
    ForkExec("u1");
    return 0;
}