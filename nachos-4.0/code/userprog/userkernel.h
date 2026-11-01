// userkernel.h
//	Global variables for the Nachos kernel, for the assignment
//	supporting running user programs.
//
//	The kernel supporting user programs is a version of the 
//	basic multithreaded kernel.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef USERKERNEL_H  
#define USERKERNEL_H

#include "kernel.h"
#include "filesys.h"
#include "machine.h"
#include "bitmap.h"
#include <vector>

class SynchConsoleInput;

class UserProgKernel : public ThreadedKernel {
  public:
    UserProgKernel(int argc, char **argv);
				// Interpret command line arguments
    ~UserProgKernel();		// deallocate the kernel

    void Initialize();		// initialize the kernel 

    void Run();			// Start one thread per selected user program

    void SelfTest();		// test whether kernel is working

// These are public for notational convenience.
    Machine *machine;
    Bitmap *physicalPageMap; // Shared frame allocation map; a set bit means in use
    FileSystem *fileSystem;
    SynchConsoleInput *consoleInput;

  private:
    bool debugUserProg;		// single step user program
    std::vector<char *> userPrograms;		// Borrowed argv filenames, one per -e
};

#endif
