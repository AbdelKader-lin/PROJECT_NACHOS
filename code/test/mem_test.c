#include "mem_init.h"
#include "syscall.h"

void print_ptr(void* p) {
    SynchPutInt((int) p);
    SynchPutChar('\n');
}

int main() {
    SynchPutString("mem_test: starting\n");

    void* base = mem_init(500);
    SynchPutString("mem_test: mem_init base = ");
    print_ptr(base);

    void* a = mem_alloc(100);
    SynchPutString("mem_test: alloc a = ");
    print_ptr(a);

    void* b = mem_alloc(200);
    SynchPutString("mem_test: alloc b = ");
    print_ptr(b);

    /* write some data */
    if (a) {
        char* ca = (char*) a;
        ca[0] = 'A';
        ca[99] = 'Z';
        SynchPutString("mem_test: a[0]=");
        SynchPutChar(ca[0]);
        SynchPutChar('\n');
    }

    if (b) {
        char* cb = (char*) b;
        cb[0] = 'B';
        cb[199] = 'Y';
        SynchPutString("mem_test: b[0]=");
        SynchPutChar(cb[0]);
        SynchPutChar('\n');
    }

    SynchPutString("mem_test: free a\n");
    mem_free(a);

    SynchPutString("mem_test: alloc c (should reuse freed space)\n");
    void* c = mem_alloc(80);
    SynchPutString("mem_test: alloc c = ");
    print_ptr(c);

    SynchPutString("mem_test: cleaning up\n");
    mem_free(b);
    mem_free(c);

    SynchPutString("mem_test: done\n");
    return 0;
}
