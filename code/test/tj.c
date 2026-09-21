#include "syscall.h"

void worker(void *arg) {
    int threadId = *(int *)arg;
    SynchPutString("Worker thread running\n");
    SynchPutInt(threadId);
    SynchPutString("\n");
    UserThreadExit();
}

int main() {
    static int dummy_1 = 100; // to avoid warning about no args
    static int dummy_2 = 200; // to avoid warning about no args
    SynchPutString("Main: creating worker\n");
    unsigned int threadId1 = UserThreadCreate(worker, (void *)&dummy_1);
    unsigned int threadId2 = UserThreadCreate(worker, (void *)&dummy_2);
    SynchPutString("Main: created worker threads with IDs: ");
    SynchPutInt(threadId1);
    SynchPutString(", ");
    SynchPutInt(threadId2);
    SynchPutString("\n");
    UserThreadJoin(threadId1);
    UserThreadJoin(threadId2);

    /* should never reach here */
    return 0;
}
