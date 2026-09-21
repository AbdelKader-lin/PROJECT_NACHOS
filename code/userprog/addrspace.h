// addrspace.h
//      Data structures to keep track of executing user programs
//      (address spaces).
//
//      For now, we don't keep any information about address spaces.
//      The user level CPU state is saved and restored in the thread
//      executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"
#include "machine.h"
#include "translate.h"
#include "bitmap.h"
#include "synch.h" 
#include "frameprovider.h"

#define UserStackSize 1024 // increase this as necessary!
#define MaxNumberThreads 5

class AddrSpace {
  	public:
		BitMap *bitmap;
		AddrSpace(OpenFile *executable); // Create an address space,
		// initializing it with the program
		// stored in the file "executable"
		~AddrSpace(); // De-allocate an address space

		void InitRegisters(); // Initialize user-level CPU registers,
		// before jumping to user code

		void SaveState();    // Save/restore address space-specific
		void RestoreState(); // info on a context switch
		int GetNumPages();

		void AddThread();
		
		bool RemoveThread();
		void WaitForAllThreadsToExit();
		unsigned int GetThreadCount() { return threadCount; }
		int TestAndSetHalt();

		void ReadAtVirtual(OpenFile *executable, int virtAddr,
						int numBytes, int position, TranslationEntry *pTable, 
						unsigned nbPages);

    	unsigned int GetAddressSpaceID() { return address_space_id; }

		// For Join: signal when this address space exits
		void SetExitStatus(int status);
		int WaitForExit();

		int Sbrk(unsigned n);
  	
	private:
		static unsigned int address_space_counter;
		unsigned int address_space_id;

		TranslationEntry *pageTable; // Assume linear page table translation for now!
		unsigned int numPages; // Number of pages in the virtual address space

		Semaphore *threadCountMutex;
		unsigned int threadCount;
		Semaphore *haltSemaphore;

		// For Join: signal when this address space exits
		Semaphore *exitSemaphore;
		int exitStatus;

		unsigned int halt;
		Semaphore *haltMutex;

		unsigned brk;	// the current head of the stack
};

#endif // ADDRSPACE_H
