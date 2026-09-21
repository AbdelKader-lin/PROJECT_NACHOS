// exception.cc
//      Entry point into the Nachos kernel from user programs.
//      There are two kinds of things that can cause control to
//      transfer back to here from user code:
//
//      syscall -- The user code explicitly requests to call a procedure
//      in the Nachos kernel.  Right now, the only function we support is
//      "Halt".
//
//      exceptions -- The user code does something that the CPU can't handle.
//      For instance, accessing memory that doesn't exist, arithmetic errors,
//      etc.
//
//      Interrupts (which can also cause control to transfer from user
//      code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "syscall.h"
#include "system.h"
#include "userthread.h"


extern int do_UserThreadCreate(int f, int arg);
extern int do_UserThreadExit();
extern void do_WaitUserThreads();
extern void do_UserThreadJoin(unsigned int threadId);
extern void do_ForkExec(char *name);
extern int do_Exec(char *name);
extern int do_Join(int id);
extern int do_ProcessExit(int status);

#include "directory.h"

#define NbMaxSem 20
static Semaphore* SemTable[NbMaxSem] = {NULL};

extern int OpenFileId;

//----------------------------------------------------------------------
// UpdatePC : Increments the Program Counter register in order to resume
// the user program immediately after the "syscall" instruction.
//----------------------------------------------------------------------
static void UpdatePC() {
    int pc = machine->ReadRegister(PCReg);
    machine->WriteRegister(PrevPCReg, pc);
    pc = machine->ReadRegister(NextPCReg);
    machine->WriteRegister(PCReg, pc);
    pc += 4;
    machine->WriteRegister(NextPCReg, pc);
}

void copyStringFromMachine(int from, char *to, unsigned size) {
    int value;
    if (size == 0) return;
    for (unsigned i = 0; i < size - 1; i++) {
        if (!machine->ReadMem(from + i, 1, &value)) {
            to[i] = '\0';
            return;
        }
        to[i] = (char)value;
        if (to[i] == '\0') return;
    }

    to[size - 1] = '\0';
}

void copyStringToMachine(int to, const char *from, unsigned size) {
    if (size == 0) return;
    unsigned i;
    for (i = 0; i < size - 1; i++) {
        if (!machine->WriteMem(to + i, 1, (int)from[i])) {
            break;
        }
        if (from[i] == '\0') return;
    }
    machine->WriteMem(to + i, 1, (int)'\0');
}

//----------------------------------------------------------------------
// ExceptionHandler
//      Entry point into the Nachos kernel.  Called when a user program
//      is executing, and either does a syscall, or generates an addressing
//      or arithmetic exception.
//
//      For system calls, the following is the calling convention:
//
//      system call code -- r2
//              arg1 -- r4
//              arg2 -- r5
//              arg3 -- r6
//              arg4 -- r7
//
//      The result of the system call, if any, must be put back into r2.
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//      "which" is the kind of exception.  The list of possible exceptions
//      are in machine.h.
//----------------------------------------------------------------------

void ExceptionHandler(ExceptionType which) {
    int type = machine->ReadRegister(2);

    if (which == SyscallException) {
        switch (type) {
            case SC_Halt: {
                DEBUG('a', "Shutdown, initiated by user program.\n");
                if (!currentThread->space->TestAndSetHalt()) {
                    do_WaitUserThreads();
                    interrupt->Halt();
                } else {
                    do_UserThreadExit();
                }
                break;
            }

            case SC_Exit: {
                int status = machine->ReadRegister(4);
                do_ProcessExit(status);
                break;
            }

            case SC_Fork: {
                int f_addr = machine->ReadRegister(4);
                // Reuse user thread creation path with no argument
                do_UserThreadCreate(f_addr, 0);
                break;
            }

            case SC_Join: {
                int id = machine->ReadRegister(4);
                int status = do_Join(id);
                machine->WriteRegister(2, status);
                break;
            }
            
            case SC_Exec: {
                int nameAddr = machine->ReadRegister(4);
                char *filename = new char[MAX_STRING_SIZE + 1];
                int i = 0;
                char ch;
                while (i < MAX_STRING_SIZE) {
                    if (!machine->ReadMem(nameAddr + i, 1, (int *)&ch)) {
                        printf("Error reading memory at address %d\n", nameAddr + i);
                        break;
                    }
                    filename[i] = ch;
                    if (ch == '\0') {
                        break;
                    }
                    i++;
                }
                filename[i] = '\0';
                int spaceId = do_Exec(filename);
                machine->WriteRegister(2, spaceId);
                break;
            }

            case SC_Sbrk: {
                unsigned n = machine->ReadRegister(4);
                int addr = currentThread->space->Sbrk(n);
                machine->WriteRegister(2, addr);
                break;
            }

            case SC_SynchPutChar: {
                char c = (char)machine->ReadRegister(4);
                synchConsole->SynchPutChar(c);
                break;
            }

            case SC_SynchGetChar: {
                int c = synchConsole->SynchGetChar();
                machine->WriteRegister(2, c);
                break;
            }

            case SC_SynchPutString: {
                int addr = machine->ReadRegister(4);
                char buffer[MAX_STRING_SIZE + 1];
                int i = 0;
                char ch;
                // Read the string from user memory
                while (i < MAX_STRING_SIZE) {
                    if (!machine->ReadMem(addr + i, 1, (int *)&ch)) {
                        printf("Error reading memory at address %d\n", addr + i);
                        break;
                    }
                    buffer[i] = ch;
                    if (ch == '\0') {
                        break;
                    }
                    i++;
                }
                buffer[i] = '\0';
                synchConsole->SynchPutString(buffer);
                break;
            }
        
            case SC_SynchGetString: {
                int addr = machine->ReadRegister(4);
                int n = machine->ReadRegister(5);
                char buffer[MAX_STRING_SIZE + 1];
                synchConsole->SynchGetString(buffer, n);
                n = n < MAX_STRING_SIZE ? n : MAX_STRING_SIZE; 
                // Write the string back to user memory
                for (int i = 0; i < n; i++) {
                    if (!machine->WriteMem(addr + i, 1, (int)buffer[i])) {
                        printf("Error writing memory at address %d\n", addr + i);
                        break;
                    }
                    if (buffer[i] == '\0') {
                        break;
                    }
                }
                break;
            }

            case SC_SynchPutInt: {
                int n = machine->ReadRegister(4);
                // Convert integer to string and print
                char buffer[MAX_INT_SIZE]; // Enough for 32-bit int
                snprintf(buffer, sizeof(buffer), "%d", n);
                synchConsole->SynchPutString(buffer);
                break;
            }

            case SC_SynchGetInt: {
                int* n_ptr = (int*)machine->ReadRegister(4);
                int i = 0, n = 0;
                char ch;
                // Read characters until newline or buffer full
                while (i < MAX_INT_SIZE - 1) {
                    ch = synchConsole->SynchGetChar();
                    if (ch == '\n' || ch == EOF) {
                        break;
                    }
                    n = n * 10 + (ch - '0');
                    i++;
                }
                DEBUG('d', "%d\n", n);
                // Write the integer back to user memory
                if (!machine->WriteMem((int)n_ptr, sizeof(int), n)) {
                    printf("Error writing integer to memory at address %d\n", (int)n_ptr);
                }
                break;
            }
        
            case SC_UserThreadCreate: {
                int f_addr = machine->ReadRegister(4);
                int arg_addr = machine->ReadRegister(5);
                unsigned int threadId = do_UserThreadCreate(f_addr, arg_addr);
                machine->WriteRegister(2, (int)threadId);
                break;
            }

            case SC_UserThreadExit: {
                do_UserThreadExit();
                break;
            }

            case SC_UserThreadJoin: {
                unsigned int threadId = (unsigned int)machine->ReadRegister(4);
                do_UserThreadJoin(threadId);
                break;
            }

            case SC_ForkExec: {
                int nameAddr = machine->ReadRegister(4);
                char *filename = new char[MAX_STRING_SIZE + 1];
                int i = 0;
                char ch;
                while (i < MAX_STRING_SIZE) {
                    if (!machine->ReadMem(nameAddr + i, 1, (int *)&ch)) {
                        printf("Error reading memory at address %d\n", nameAddr + i);
                        break;
                    }
                    filename[i] = ch;
                    if (ch == '\0') {
                        break;
                    }
                    i++;
                }
                filename[i] = '\0'; 
                do_ForkExec(filename);
                break;
            }

#ifdef FILESYS
            case SC_Create: {
                int addr = machine->ReadRegister(4);
                int initialSize = machine->ReadRegister(5);
                char name[MAX_STRING_SIZE + 1];
                copyStringFromMachine(addr, name, MAX_STRING_SIZE);
                
                bool success = fileSystem->Create(name, initialSize);
                machine->WriteRegister(2 , success ? 1 : 0);
                break;
            }

            case SC_Open: {
                int addr = machine->ReadRegister(4);

                char name[MAX_STRING_SIZE + 1];
                int i = 0;
                char ch;
                // Read the string from user memory
                while ( i < MAX_STRING_SIZE) {
                    if (!machine->ReadMem(addr + i, 1, (int *)&ch)) {
                        printf("Error reading memory at address %d\n", addr + i);
                        break;
                    }
                    name[i] = ch;
                    if (ch == '\0') {
                        break;
                    }
                    i++;
                }
                name[i] = '\0';

                OpenFile* open = fileSystem->Open(name);

                int localFd = -1;
                if (open != NULL) {
                    int sysIdx = -1;
                    for (int j = 0; j < 10; j++) {
                        if (fileSystem->OpenFileTable[j] == open) {
                            sysIdx = j;
                            break;
                        }
                    }

                    int ind = currentThread->get_idx_FirstFree();
                    if (ind != -1 && sysIdx != -1) {
                        currentThread->change_idx_valTonewval(ind, sysIdx);
                        localFd = ind;
                    } else {
                        fileSystem->Close(open);
                        localFd = -1;
                    }
                }
                machine->WriteRegister(2, localFd);
                break;
            }

            case SC_Close: {
                int fd = machine->ReadRegister(4);
                int idx_sys = currentThread->get_openfile_idx(fd);
                if (idx_sys == -1) {
                    machine->WriteRegister(2, -1);
                    break;
                }
                
                // fd to OpenFile* pointer
                OpenFile* file = fileSystem->OpenFileTable[idx_sys];
                bool res = fileSystem->Close(file);

                int r = 0;
                if(res) r = 1;

                // Clear the thread-local mapping for this fd
                currentThread->change_idx_valTonewval(fd, -1);
                machine->WriteRegister(2, r);
                break;
            }

            case SC_Read: {
                // int Read(char *buffer, int size, OpenFileId id);
                int addrBuffer = machine->ReadRegister(4); // Virtual memory address
                int size = machine->ReadRegister(5); // Nb of bytes
                int id = machine->ReadRegister(6); // Id of the file to read from

                if (id < 0 || id > 9 || size == 0){
                    machine->WriteRegister(2, -1);
                    break;
                }
                
                int ind = currentThread->get_openfile_idx(id);
                if (ind < 0 || ind > 9) {
                    machine->WriteRegister(2, -1);
                    break;
                }
                OpenFile* f = fileSystem->OpenFileTable[ind];

                char buffer[size + 1];
                int r = f->Read(buffer, size);

                for (int i = 0; i < size; i++) {
                    machine->WriteMem(addrBuffer + i, 1, (int)(unsigned char)buffer[i]); // write into virtual mem
                }

                //delete[] buffer;
                machine->WriteRegister(2, r);
                break;
            }

            case SC_Write: {
                // void Write(char *buffer, int size, OpenFileId id); 
                int addrBuffer = machine->ReadRegister(4); // Virtual memory address
                int size = machine->ReadRegister(5); // Nb of bytes
                int id = machine->ReadRegister(6); // Id of the file to read from
                if (id < 0 || id > 9 || size == 0) {
                    machine->WriteRegister(2 , -1);
                    break;
                }
                
                int ind = currentThread->get_openfile_idx(id);
                if (ind < 0 || ind > 9) {
                    machine->WriteRegister(2, -1);
                    break;
                }

                OpenFile* file = fileSystem->OpenFileTable[ind];
                ASSERT(file != nullptr);
                char buffer[size + 1];
                int v;
                for (int i = 0; i < size; i++) {
                    machine->ReadMem(addrBuffer + i, 1, &v); 
                    buffer[i] = (char)v;
                }
                int r = file->Write(buffer, size);
                machine->WriteRegister(2, r);
                break;
            }

            case SC_CreateDirectory: {
                int addr = machine->ReadRegister(4);
                char buffer[MAX_STRING_SIZE + 1];
                int i = 0;
                char ch;
                // Read the string from user memory
                while (i < MAX_STRING_SIZE) {
                    if (!machine->ReadMem(addr + i, 1, (int *)&ch)) {
                        printf("Error reading memory at address %d\n", addr + i);
                        break;
                    }
                    buffer[i] = ch;
                    if (ch == '\0') {
                        break;
                    }
                    i++;
                }
                buffer[i] = '\0'; // Null-terminate the string

                bool successful = fileSystem->CreateDirectory(buffer);
                machine->WriteRegister(2, successful ? 1 : 0);
                break;
            }

            case SC_ChangeDirectory: {
                int addr = machine->ReadRegister(4);
                char buffer[MAX_STRING_SIZE + 1];
                int i = 0;
                char ch;
                // Read the string from user memory
                while (i < MAX_STRING_SIZE) {
                    if (!machine->ReadMem(addr + i, 1, (int *)&ch)) {
                        printf("Error reading memory at address %d\n", addr + i);
                        break;
                    }
                    buffer[i] = ch;
                    if (ch == '\0') {
                        break;
                    }
                    i++;
                }
                buffer[i] = '\0'; // Null-terminate the string
                
                bool successful = fileSystem->ChangeDirectory(buffer);
                machine->WriteRegister(2, successful ? 1 : 0);
                break;
            }

            case SC_RemoveDirectory: {
                int addr = machine->ReadRegister(4);
                char buffer[MAX_STRING_SIZE + 1];
                int i = 0;
                char ch;
                // Read the string from user memory
                while (i < MAX_STRING_SIZE) {
                    if (!machine->ReadMem(addr + i, 1, (int *)&ch)) {
                        printf("Error reading memory at address %d\n", addr + i);
                        break;
                    }
                    buffer[i] = ch;
                    if (ch == '\0') {
                        break;
                    }
                    i++;
                }
                buffer[i] = '\0'; // Null-terminate the string
                
                bool successful = fileSystem->RemoveDirectory(buffer);
                machine->WriteRegister(2, successful ? 1 : 0);
                break;
            }

            case SC_ListDirectory: {
                fileSystem->ListCurrent();
                break;
            }

#endif // FILESYS
            case SC_SemCreate: {
                // void CreateSem( const char *debugName , int initialValue );
                int addr = machine->ReadRegister(4); // Sem's name
                int initVal = machine->ReadRegister(5); // Init value
                char name[MAX_STRING_SIZE + 1];
                int i = 0;
                char ch;
                // Read the string from user memory
                while (i < MAX_STRING_SIZE) {
                    if (!machine->ReadMem(addr + i, 1, (int *)&ch)) {
                        printf("Error reading memory at address %d\n", addr + i);
                        break;
                    }
                    name[i] = ch;
                    if (ch == '\0') {
                        break;
                    }
                    i++;
                }
                name[i] = '\0';

                Semaphore* sem = new Semaphore(name, initVal);
                int id = -1;
                for (int j = 0; j < NbMaxSem; j++) {
                    if (SemTable[j] == NULL) {
                        SemTable[j] = sem;
                        id = j;
                        break;
                    }
                }
                machine->WriteRegister(2, id);
                break;
            }

            case SC_P: {
                int id = machine->ReadRegister(4); // Sem id
                if (id >= 0 && id < NbMaxSem && SemTable[id] != NULL) {
                    Semaphore* s = SemTable[id];
                    s->P();
                }
                break;
            }

            case SC_V: {
                int id = machine->ReadRegister(4); // Sem id
                if (id >=0 && id < NbMaxSem && SemTable[id] != NULL) {
                    Semaphore* s = SemTable[id];
                    s->V();
                }
                break;
            }

            default: {
                printf("Unexpected user mode exception %d %d\n", which, type);
                ASSERT(FALSE);
            }
        }
        UpdatePC();
    }
    // End of addition
}
