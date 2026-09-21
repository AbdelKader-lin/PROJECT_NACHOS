// addrspace.cc
//      Routines to manage address spaces (executing user programs).
//
//      In order to run a user program, you must:
//
//      1. link with the -N -T 0 option
//      2. run coff2noff to convert the object file to Nachos format
//              (Nachos object code format is essentially just a simpler
//              version of the UNIX executable object code format)
//      3. load the NOFF file into the Nachos file system
//              (if you haven't implemented the file system yet, you
//              don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "addrspace.h"
#include "copyright.h"
#include "noff.h"
#include "system.h"

#include "machine.h"
#include "translate.h"
#include "frameprovider.h"

#include <strings.h> /* for bzero */


extern FrameProvider* frameprovider;

//----------------------------------------------------------------------
// SwapHeader
//      Do little endian to big endian conversion on the bytes in the
//      object file header, in case the file was generated on a little
//      endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void SwapHeader(NoffHeader *noffH) {
    noffH->noffMagic = WordToHost(noffH->noffMagic);
    noffH->code.size = WordToHost(noffH->code.size);
    noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
    noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
    noffH->initData.size = WordToHost(noffH->initData.size);
    noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
    noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
    noffH->uninitData.size = WordToHost(noffH->uninitData.size);
    noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
    noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
//      Create an address space to run a user program.
//      Load the program from a file "executable", and set everything
//      up so that we can start executing user instructions.
//
//      Assumes that the object code file is in NOFF format.
//
//      First, set up the translation from program memory to physical
//      memory.  For now, this is really simple (1:1), since we are
//      only uniprogramming, and we have a single unsegmented page table
//
//      "executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------
unsigned int AddrSpace::address_space_counter = 0;

AddrSpace::AddrSpace(OpenFile *executable) {
    address_space_id = address_space_counter++;

    NoffHeader noffH;
    unsigned int i, size;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    
    if ((noffH.noffMagic != NOFFMAGIC) &&
        (WordToHost(noffH.noffMagic) == NOFFMAGIC))
        SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    // how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size +
           UserStackSize; // we need to increase the size
    // to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    ASSERT(numPages <= NumPhysPages); // check we're not trying
    // to run anything too big --
    // at least until we have
    // virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", numPages, size);
    // first, set up the translation
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
        int frame = frameprovider->GetEmptyFrame();
        ASSERT(frame != -1);

        pageTable[i].virtualPage = i;
        pageTable[i].physicalPage = frame;
        pageTable[i].valid = TRUE;
        pageTable[i].use = FALSE;
        pageTable[i].dirty = FALSE;
        pageTable[i].readOnly = FALSE; // if the code segment was entirely on
                                       // a separate page, we could set its
                                       // pages to be read-only
    }

    unsigned int dataPages = divRoundUp(noffH.code.size + noffH.initData.size + noffH.uninitData.size, PageSize);
    brk = dataPages;

    // zero out the entire address space, to zero the unitialized data segment
    // and the stack segment
    // bzero(machine->mainMemory, size);
    for (i = 0; i < numPages; i++) {
        int physAddr = pageTable[i].physicalPage * PageSize;
        bzero(&machine->mainMemory[physAddr], PageSize);
    }

    // then, copy in the code and data segments into memory
    if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n",
            noffH.code.virtualAddr, noffH.code.size);
        ReadAtVirtual(executable, noffH.code.virtualAddr,
            noffH.code.size, noffH.code.inFileAddr, pageTable, numPages);
    }
    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n",
              noffH.initData.virtualAddr, noffH.initData.size);
        ReadAtVirtual(executable, noffH.initData.virtualAddr, noffH.initData.size,
            noffH.initData.inFileAddr, pageTable, numPages);
    }
    threadCountMutex = new Semaphore("Thread Count Mutex", 1);
    threadCount = 0;
    haltSemaphore = new Semaphore("Halt Semaphore", 0);

    halt = 0;
    haltMutex = new Semaphore("Halt Mutex", 1);

    exitSemaphore = new Semaphore("Exit Semaphore", 0);
    exitStatus = 0;

    // initialize the bitmap
    bitmap = new BitMap(MaxNumberThreads + 1);
    bitmap->Mark(0);
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
//      Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace() {
    // LB: Missing [] for delete
    // delete pageTable;
    for (int i = 0; i < (int)numPages; i++) {
        if (pageTable[i].valid)
            frameprovider->ReleaseFrame(pageTable[i].physicalPage);
    }
    delete[] pageTable;
    delete[] bitmap;
    delete exitSemaphore;
    // End of modification
}

void AddrSpace::SetExitStatus(int status) {
    exitStatus = status;
    exitSemaphore->V();
}

int AddrSpace::WaitForExit() {
    exitSemaphore->P();
    return exitStatus;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
//      Set the initial values for the user-level register set.
//
//      We write these directly into the "machine" registers, so
//      that we can immediately jump to user code.  Note that these
//      will be saved/restored into the currentThread->userRegisters
//      when this thread is context switched out.
//----------------------------------------------------------------------

void AddrSpace::InitRegisters() {
    int i;

    for (i = 0; i < NumTotalRegs; i++)
        machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

    // Set the stack register to the end of the address space, where we
    // allocated the stack; but subtract off a bit, to make sure we don't
    // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
//      On a context switch, save any machine state, specific
//      to this address space, that needs saving.
//
//      For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() {}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
//      On a context switch, restore the machine state so that
//      this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() {
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}

int AddrSpace::GetNumPages() {
    return numPages;
}

void AddrSpace::AddThread() {
    threadCountMutex->P();
    threadCount++;
    threadCountMutex->V();
}

bool AddrSpace::RemoveThread() {
    int userThreadsLeft;
    threadCountMutex->P();
    threadCount--;
    userThreadsLeft = threadCount;
    if (threadCount == 0) {
        haltSemaphore->V();
    }
    threadCountMutex->V();
    return userThreadsLeft == -1;
}

void AddrSpace::WaitForAllThreadsToExit() {
    threadCountMutex->P();
    if (threadCount > 0) {
        threadCountMutex->V();
        haltSemaphore->P();
    } else {
        threadCountMutex->V();
    }
}

int AddrSpace::TestAndSetHalt() {
    haltMutex->P();
    int oldHalt = halt;
    halt = 1;
    haltMutex->V();
    return oldHalt;
}

void my_translate(int* physAddr, int virtAddr, unsigned nbPages, TranslationEntry *pTable, int nbBytes_tocheck){
    
    TranslationEntry* tmpTable = machine->pageTable ;
    unsigned int tmpSize = machine->pageTableSize ;

    machine->pageTable = pTable ;
    machine->pageTableSize = nbPages ;

    ExceptionType exception = machine->Translate( virtAddr , physAddr , nbBytes_tocheck , true );
    ASSERT( exception == NoException ); 

    
    machine->pageTable = tmpTable ;
    machine->pageTableSize = tmpSize ;

}

void my_translate2(int* physAddr, int virtAddr, unsigned nbPages, TranslationEntry *pTable, int nbBytes_tocheck) {
        //int i;
        unsigned int vpn, offset;
        TranslationEntry *entry;
        unsigned int pageFrame;
        bool writing = true;

        DEBUG('a', "\tTranslate 0x%x, %s: ", virtAddr, writing ? "write" : "read");

        vpn = (unsigned)virtAddr / PageSize;
        offset = (unsigned)virtAddr % PageSize;

        if (vpn >= nbPages) {
            DEBUG('a', "virtual page # %d too large for page table size %d!\n", virtAddr, nbPages);
            machine->RaiseException(AddressErrorException, virtAddr);
        } else if (!pTable[ vpn ].valid) {
            DEBUG('a', "virtual page # %d is not valid!\n", virtAddr, nbPages);
            machine->RaiseException(PageFaultException, virtAddr);
            return;
        }
        entry = &pTable[vpn];
        
        if (entry == NULL) { // not found
            DEBUG('a', "*** no valid TLB entry found for this virtual page!\n");
            machine->RaiseException(PageFaultException, virtAddr);
            return;
        }

        if (entry->readOnly && writing) { // trying to write to a read-only page
            //DEBUG('a', "%d mapped read-only at %d in TLB!\n", virtAddr, i);
            DEBUG('a', "%d mapped read-only!\n", virtAddr);
            machine->RaiseException(ReadOnlyException, virtAddr);
            return; 
        }
        pageFrame = entry->physicalPage;

        // if the pageFrame is too big, there is something really wrong!
        // An invalid translation was loaded into the page table or TLB.
        if (pageFrame >= NumPhysPages) {
            DEBUG('a', "*** frame %d > %d!\n", pageFrame, NumPhysPages);
            return; //BusErrorException;
        }
        entry->use = TRUE; // set the use, dirty bits
        if (writing)
            entry->dirty = TRUE;
        *physAddr = pageFrame * PageSize + offset;
        ASSERT(((offset + nbBytes_tocheck) <= PageSize));
        DEBUG('a', "phys addr = 0x%x\n", *physAddr);
}

// Writes to the virtal space deﬁned by pageTable and numPages.
// You can use a temporary buﬀer, that you will fill with ReadAt, 
// and then you will then re-copy into memory with WriteMem. 

void AddrSpace::ReadAtVirtual(OpenFile *executable, int virtAddr,
    int numBytes, int position, TranslationEntry *pTable, unsigned nbPages) {

    char* into = new char[numBytes];
    executable->ReadAt(into, numBytes, position);

    int physAddr;
    my_translate(&physAddr, virtAddr, nbPages, pTable, 1);
    //machine->Translate(virtAddr, &physAddr, 1, true);

    int nb_bytes = numBytes;
    while(nb_bytes >= 4) {
        for (int j = 0; j < 4; j++) {
            machine->mainMemory[physAddr + j] = (unsigned char)*(into + j);
        }
        physAddr += 4;
        virtAddr += 4;
        if ((virtAddr % PageSize) == 0) {
            my_translate(&physAddr, virtAddr, nbPages, pTable, 4);
            //machine->Translate(virtAddr, &physAddr, 4, true);
        }
        nb_bytes -= 4;
        into += 4;
    }
    while(nb_bytes >= 2) {
        for (int j = 0; j < 2; j++) {
            machine->mainMemory[physAddr + j] = (unsigned char)*(into + j);
        }
        physAddr += 2;
        virtAddr += 2; 
        if ((virtAddr % PageSize) == 0) {
            my_translate(&physAddr, virtAddr, nbPages, pTable, 2);
            //machine->Translate(virtAddr, &physAddr, 2, true);
        }
        nb_bytes -= 2;
        into += 2;
    }
    if (nb_bytes == 1) {
        machine->mainMemory[physAddr] = (unsigned char)(*into);
        //physAddr++;
        virtAddr++; 
        if ((virtAddr % PageSize) == 0) {
            my_translate(&physAddr, virtAddr, nbPages, pTable, 1);
            //machine->Translate(virtAddr, &physAddr, 1, true);
        }
        nb_bytes--;
        into += 1;
    }
}

int AddrSpace::Sbrk(unsigned n) {
    unsigned oldBrk = brk;

    if (brk + n > numPages) return -1;

    for (unsigned i = 0; i < n; i++) {
        int frame = frameprovider->GetEmptyFrame();
        if (frame == -1) {
            for (unsigned j = 0; j < i; j++) {
                frameprovider->ReleaseFrame(
                    pageTable[oldBrk + j].physicalPage
                );
                pageTable[oldBrk + j].valid = false;
            }
            return -1;
        }

        pageTable[oldBrk + i].physicalPage = frame;
        pageTable[oldBrk + i].valid = true;
        pageTable[oldBrk + i].readOnly = false;
    }
    brk += n;
    return brk * PageSize;
}