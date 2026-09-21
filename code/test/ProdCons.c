#include "syscall.h"
#define NbElem 20

static int buffer[NbElem];
int in = 0;
int out = 0;

// Semaphores
static int notEmpty;
static int notFull;
static int mutualExc;

void prod(int x) {
    P(notFull);
    P(mutualExc);

    buffer[in] = x;
    in = (in + 1) % NbElem;
    
    V(mutualExc);
    V(notEmpty);
}

int cons() {
    P(notEmpty);
    P(mutualExc);
    
    int x = buffer[out];
    out = (out + 1) % NbElem;
    
    V(mutualExc);
    V(notFull);

    return x;
}

void prod_worker(void *arg) { // Produce 5 elements
    SynchPutString("Producer thread started!\n");
    int x = *(int *)arg;
    for (int i = 0; i < 5; i++) {
        int elem = x + i + 1;
        prod(elem);
        SynchPutString("produced");
        SynchPutInt(elem);
        SynchPutString("\n");
    }
    UserThreadExit();
}

void cons_worker(void *arg) { // Consume 5 elements
    SynchPutString("Consumer thread started!\n");
    // int x = *(int *)arg;
    for (int i = 0; i < 5; i++) {
        int elem = cons();
        SynchPutString("Consumed");
        SynchPutInt(elem);
        SynchPutString("\n");
    }
    UserThreadExit();
}

int main() {
    notEmpty = CreateSem("notEmpty", 0);
    notFull = CreateSem("notFull", NbElem);
    mutualExc = CreateSem("mutualExc", 1);

    static int arg0 = 1;
    static int arg1 = 2;
    
    unsigned int t0 = UserThreadCreate(prod_worker, (void *)&arg0);
    SynchPutString("Producer thread nb: ");
    SynchPutInt(t0);
    SynchPutString("\n");
    
    unsigned int t1 = UserThreadCreate(cons_worker, (void *)&arg1);
    SynchPutString("Consumer thread nb: ");
    SynchPutInt(t1);
    SynchPutString("\n");

    UserThreadJoin(t0);
    UserThreadJoin(t1);

    SynchPutString("Test Finished\n");
    UserThreadExit();
}
