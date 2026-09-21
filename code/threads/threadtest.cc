// threadtest.cc
//      Simple test case for the threads assignment.
//
//      Create two threads, and have them context switch
//      back and forth between themselves by calling Thread::Yield,
//      to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"

//----------------------------------------------------------------------
// SimpleThread
//      Loop 5 times, yielding the CPU to another ready thread
//      each iteration.
//
//      "which" is simply a number identifying the thread, for debugging
//      purposes.
//----------------------------------------------------------------------

void SimpleThread(int which) {
    int num;

    for (num = 0; num < 5; num++) {
    //while (1) {
        printf("*** thread %d looped %d times\n", which, num);
        //currentThread->Yield();
    }
}

//----------------------------------------------------------------------
// ThreadTest
//      Set up a ping-pong between two threads, by forking a thread
//      to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

/*void ThreadTest() {
    DEBUG('t', "Entering SimpleTest\n");

    Thread *t0 = new Thread("forked thread T0");
    t0->Fork(SimpleThread, 0);

    Thread *t1 = new Thread("forked thread T1");
    t1->Fork(SimpleThread, 1);

    SimpleThread(0);
}*/

void ThreadTest() {
    DEBUG('t', "Entering SimpleTest\n");

    const int N = 500;

    for (int i = 0; i < N; i++) {
        char *name = new char[32];
        sprintf(name, "T%d", i);

        Thread *t = new Thread(name);
        t->Fork(SimpleThread, i);
        // NOTE: we intentionally don't delete name here; Thread stores the pointer.
    }

    // Option A: let main also run the workload
    SimpleThread(-1);

    // Option B: or just yield forever so workers run
    // while (true) currentThread->Yield();
}

