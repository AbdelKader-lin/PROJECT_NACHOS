#include "syscall.h"

static void Worker(void *arg) {
    int tid = *(int *)arg;
    int fd = Open("shared.txt"); // We try to open the file
    
    if (fd < 0) { 
        SynchPutString("Worker: open failed (expected for 2nd thread)\n"); 
        UserThreadExit(); 
        return; 
    }

    if (tid == 0) {
        Write("AAA", 3, fd);
    }
    else{
        Write("BBB", 3, fd);
    }

    Close(fd);
    SynchPutString("Worker: done\n");
    UserThreadExit();
}

int main() {
    SynchPutString("Concurrent FS test\n");

    CreateDirectory("d1"); // mkdir d1
    ChangeDirectory("d1"); // cd d1
    
    // Open multiple files
    Create("f1.txt");
    Create("f2.txt");

    int fd1 = Open("f1.txt");
    int fd2 = Open("f2.txt");
    
    if (fd1 < 0 || fd2 < 0) { 
        SynchPutString("Part II failed\n"); 
        return 1; 
    }
    
    Write("X", 1, fd1);
    Write("Y", 1, fd2);
    Close(fd1);
    Close(fd2);
    SynchPutString("Writing in file : OK\n");

    // concurrent
    Create("shared.txt"); // touch shared.txt
    SynchPutString("Starting 2 threads\n");
    
    static int arg0 = 0;
    static int arg1 = 1;
    unsigned int t0 = UserThreadCreate(Worker , (void *)&arg0);
    unsigned int t1 = UserThreadCreate(Worker , (void *)&arg1);

    UserThreadJoin(t0);
    UserThreadJoin(t1);

    SynchPutString("TESTS: OK\n");
    return 0;
}