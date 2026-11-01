// addrspace.h 
//	Data structures to keep track of executing user programs 
//	(address spaces).
//
//	Each address space owns a page table and its allocated physical frames.
//	User CPU registers are saved and restored by the executing thread.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"

#define UserStackSize		1024 	// increase this as necessary!

class AddrSpace {
  public:
    AddrSpace();			// Create an address space.
    ~AddrSpace();			// De-allocate an address space

    void Execute(char *fileName);	// Run the the program
					// stored in the file "executable"

    void SaveState();			// Save/restore address space-specific
    void RestoreState();		// info on a context switch 

  private:
    TranslationEntry *pageTable;	// Virtual pages mapped to owned physical frames
    unsigned int numPages;		// Number of pages in the virtual 
					// address space

    bool Load(char *fileName);		// Load the program into memory
					// Return false if missing or memory is full

    void LoadSegment(OpenFile *executable, unsigned int virtualAddr,
                     unsigned int fileOffset, unsigned int remaining);

    void InitRegisters();		// Initialize user-level CPU registers,
					// before jumping to user code

};

#endif // ADDRSPACE_H
