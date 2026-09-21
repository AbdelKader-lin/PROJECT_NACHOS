#include "thread.h"
#include "system.h"
#include "userthread.h"
#include <map>

static std::map<unsigned int, AddrSpace*> processTable;

Semaphore *threadSemaphore[MaxNumberThreads + 1];

struct UserThreadParams {
    int f;
    int arg;
};

static void StartUserThread_og(int arg);  // Forward declaration

static void StartUserThread(int arg){
    StartUserThread_og( arg ) ;
    do_UserThreadExit() ;
}
static void StartUserThread_og(int arg) {
    UserThreadParams *params = (UserThreadParams *)arg;
    int f   = params->f;
    int a  = params->arg;
    delete params;

    currentThread->space->InitRegisters();
    currentThread->space->RestoreState();
    
    machine->WriteRegister(PrevPCReg, f);
    machine->WriteRegister(PCReg, f);
    machine->WriteRegister(NextPCReg, f + 4);
    machine->WriteRegister(4, a);

    int position = currentThread->getPosition();
    if (currentThread->getThreadId() != 0) {
        int Stacktop = currentThread->space->GetNumPages() * PageSize - (position * UserStackSize) / (MaxNumberThreads + 1);
        Stacktop = (Stacktop / 4) * 4;
        machine->WriteRegister(StackReg, Stacktop);
    }
    interrupt->setStatus(UserMode);
    machine->Run();
    ASSERT(false);
}

static void ForkExecStart(int arg) {
    char *filename = (char *)arg;
    OpenFile *executable = fileSystem->Open(filename);
    if (executable == NULL) {
        printf("Unable to open file %s\n", filename);
        delete[] filename;
        return;
    }
    AddrSpace *space = new AddrSpace(executable);
    delete executable;

    currentThread->space = space;
    delete[] filename;
    space->InitRegisters();
    space->RestoreState();
    
    machine->Run();
    ASSERT(FALSE); // machine->Run never returns
}


int do_UserThreadCreate(int f, int arg) {
    Thread *newThread = new Thread("user thread");
    if (newThread == nullptr)
        return -1;

    UserThreadParams *params = new UserThreadParams;
    if (params == nullptr) {
        delete newThread;
        return -1;
    }
    params->f   = f;
    params->arg = arg;

    newThread->Fork(StartUserThread, (int)params);

    return newThread->getThreadId();
}

int do_UserThreadExit() {
    if (currentThread->getThreadId() != 0)
        currentThread->space->bitmap->Clear(currentThread->getPosition());
    
    bool isLast = currentThread->space->RemoveThread();
    
    if (threadSemaphore[currentThread->getThreadId()] == nullptr)
        threadSemaphore[currentThread->getThreadId()] = new Semaphore("Thread Join Semaphore", 0);

    threadSemaphore[currentThread->getThreadId()]->V();

    if (isLast) do_ProcessExit(0);
    else currentThread->Finish();

    ASSERT(false);
    return 0;
}

void do_WaitUserThreads() {
    currentThread->space->WaitForAllThreadsToExit();
}

void do_UserThreadJoin(unsigned int threadId) {
    if (threadId > MaxNumberThreads)
        return; // Invalid thread ID

    if (threadSemaphore[threadId] == nullptr)
        threadSemaphore[threadId] = new Semaphore("Thread Join Semaphore", 0);

    threadSemaphore[threadId]->P();
}

void do_ForkExec(char *filename) {
    char *threadName = new char[strlen(filename) + 8];
    strcpy(threadName, filename);
    strcat(threadName, "_thread");
    Thread *newThread = new Thread(threadName);
    if (newThread == NULL) {
        return;  
    }

    processCountLock->P();
    runningProcessCount++;
    processCountLock->V();
    newThread->Fork(ForkExecStart, (int)filename);
}

static void ExecStart(int arg) {
    AddrSpace *space = (AddrSpace *)arg;
    currentThread->space = space;

    space->InitRegisters();
    space->RestoreState();

    machine->Run();
    ASSERT(FALSE); // machine->Run never returns
}

int do_Exec(char *filename) {
    // Create a thread name from filename
    char *threadName = new char[strlen(filename) + 8];
    strcpy(threadName, filename);
    strcat(threadName, "_proc");
    Thread *newThread = new Thread(threadName);
    if (newThread == NULL) {
        delete[] threadName;
        delete[] filename;
        return -1;
    }

    OpenFile *executable = fileSystem->Open(filename);
    if (executable == NULL) {
        delete newThread;
        delete[] threadName;
        delete[] filename;
        return -1;
    }

    AddrSpace *space = new AddrSpace(executable);
    delete executable;
    delete[] filename;

    processCountLock->P();
    runningProcessCount++;
    // register the new address space so Join can find it
    processTable[space->GetAddressSpaceID()] = space;
    processCountLock->V();

    // Fork the new thread to start the new address space
    newThread->Fork(ExecStart, (int)space);

    return (int)space->GetAddressSpaceID();
}

int do_Join(int spaceId) {
    processCountLock->P();
    auto it = processTable.find((unsigned int)spaceId);
    if (it == processTable.end()) {
        processCountLock->V();
        return -1;
    }
    AddrSpace* space = it->second;
    processCountLock->V();

    int status = space->WaitForExit();

    // after the join returns, remove the entry
    processCountLock->P();
    processTable.erase((unsigned int)spaceId);
    processCountLock->V();

    return status;
}

int do_ProcessExit(int status) {
    processCountLock->P();
    runningProcessCount--;
    bool lastProcess = (runningProcessCount == 0);
    processCountLock->V();

    if (currentThread->space != NULL) {
        currentThread->space->SetExitStatus(status);
    }
    if (lastProcess) {
        printf("User program exited with status %d\n", status);
        interrupt->Halt();   // LAST process stops the machine
    } else {
        currentThread->Finish();  // Just terminate this process
    }
    return 0;
}
