// userkernel.cc 
//	Initialization and cleanup routines for the version of the
//	Nachos kernel that supports running user programs.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synchconsole.h"
#include "userkernel.h"

//----------------------------------------------------------------------
// UserProgKernel::UserProgKernel
// 	Interpret command line arguments in order to determine flags 
//	for the initialization (see also comments in main.cc)  
//----------------------------------------------------------------------

UserProgKernel::UserProgKernel(int argc, char **argv) 
		: ThreadedKernel(argc, argv)
{
    debugUserProg = FALSE;
    consoleInput = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0) {
            debugUserProg = TRUE;
        } else if (strcmp(argv[i], "-e") == 0) {
            ASSERT(i + 1 < argc);
            userPrograms.push_back(argv[++i]);
        } else if (strcmp(argv[i], "-u") == 0) {
            cout << "Partial usage: nachos [-s] [-e userProgram]...\n";
        }
    }
}

//----------------------------------------------------------------------
// UserProgKernel::Initialize
// 	Initialize Nachos global data structures.
//----------------------------------------------------------------------

void
UserProgKernel::Initialize()
{
    ThreadedKernel::Initialize();	// init multithreading

    machine = new Machine(debugUserProg);
    physicalPageMap = new Bitmap(NumPhysPages);
    fileSystem = new FileSystem();
}

//----------------------------------------------------------------------
// UserProgKernel::~UserProgKernel
// 	Nachos is halting.  De-allocate global data structures.
//	Automatically calls destructor on base class.
//----------------------------------------------------------------------

UserProgKernel::~UserProgKernel()
{
    delete consoleInput;
    delete fileSystem;
    delete machine;
    delete physicalPageMap;
}

static void RunUserProgram(void *arg)
{
    char *fileName = (char *)arg;
    AddrSpace *space = new AddrSpace();

    space->Execute(fileName);

    // Only a failed load returns here; Exit frees successful address spaces.
    delete space;
}

//----------------------------------------------------------------------
// UserProgKernel::Run
// 	Queue one thread per -e argument, or a halt program if none are given.
//----------------------------------------------------------------------

void
UserProgKernel::Run()
{
    if (userPrograms.empty()) {
        userPrograms.push_back((char *)"../test/halt");
    }

    for (unsigned int i = 0; i < userPrograms.size(); i++) {
        Thread *t = new Thread(userPrograms[i]);
        t->Fork(RunUserProgram, userPrograms[i]);
    }

    // Finish the startup thread instead of returning from the host main().
    ThreadedKernel::Run();
}

//----------------------------------------------------------------------
// UserProgKernel::SelfTest
//      Test whether this module is working.
//----------------------------------------------------------------------

void
UserProgKernel::SelfTest() {
    char ch;

    // Batch execution must not wait for input in the interactive self-test.
    if (!userPrograms.empty())
	return;

    ThreadedKernel::SelfTest();

    // test out the console device

    cout << "Testing the console device.\n" 
	<< "Typed characters will be echoed, until q is typed.\n"
    	<< "Note newlines are needed to flush input through UNIX.\n";
    cout.flush();

    SynchConsoleInput *input = new SynchConsoleInput(NULL);
    SynchConsoleOutput *output = new SynchConsoleOutput(NULL);

    do {
    	ch = input->GetChar();
    	output->PutChar(ch);   // echo it!
    } while (ch != 'q');

    cout << "\n";

    // self test for running user programs is to run the halt program above
}
