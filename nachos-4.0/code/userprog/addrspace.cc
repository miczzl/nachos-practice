// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -n -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "addrspace.h"
#include "machine.h"
#include "noff.h"

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
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
// 	Create an address space to run a user program.
//	Defer page allocation until Load() knows the program size.
//----------------------------------------------------------------------

AddrSpace::AddrSpace()
{
    pageTable = NULL;
    numPages = 0;
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Release the owned physical frames and page table.
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
    if (pageTable != NULL) {
        for (unsigned int i = 0; i < numPages; i++) {
            kernel->physicalPageMap->Clear(pageTable[i].physicalPage);
        }
        DEBUG(dbgAddr, "Released " << numPages << " frames; free frames: "
              << kernel->physicalPageMap->NumClear());
    }
    delete[] pageTable;
}


//----------------------------------------------------------------------
// AddrSpace::Load
// 	Load a user program into memory from a file.
//
//	Allocate physical frames, build the page table, and load NOFF segments.
//
//	"fileName" is the file containing the object code to load into memory
//----------------------------------------------------------------------

bool 
AddrSpace::Load(char *fileName) 
{
    OpenFile *executable = kernel->fileSystem->Open(fileName);
    NoffHeader noffH;
    unsigned int size;

    if (executable == NULL) {
	cerr << "Unable to open file " << fileName << "\n";
	return FALSE;
    }

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    // how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    if (numPages > kernel->physicalPageMap->NumClear()) {
        cerr << "Not enough free physical frames.\n";
        delete executable;
        return FALSE;
    }
    DEBUG(dbgAddr, "Initializing address space: " << numPages << ", " << size);

    pageTable = new TranslationEntry[numPages];
    for (unsigned int i = 0; i < numPages; i++) {
        int frame = kernel->physicalPageMap->FindAndSet();
        ASSERT(frame >= 0);

        pageTable[i].virtualPage = i;
        pageTable[i].physicalPage = frame;
        pageTable[i].valid = TRUE;
        pageTable[i].use = FALSE;
        pageTable[i].dirty = FALSE;
        pageTable[i].readOnly = FALSE;

        // Other programs may own the remaining physical memory.
        bzero(&(kernel->machine->mainMemory[frame * PageSize]), PageSize);
    }
    DEBUG(dbgAddr, "Allocated " << numPages << " frames; free frames: "
          << kernel->physicalPageMap->NumClear());

    // then, copy in the code and data segments into memory
    if (noffH.code.size > 0) {
        DEBUG(dbgAddr, "Initializing code segment.");
        DEBUG(dbgAddr, noffH.code.virtualAddr << ", " << noffH.code.size);

        LoadSegment(executable, noffH.code.virtualAddr,
                    noffH.code.inFileAddr, noffH.code.size);
    }
    if (noffH.initData.size > 0) {
        DEBUG(dbgAddr, "Initializing data segment.");
	DEBUG(dbgAddr, noffH.initData.virtualAddr << ", " << noffH.initData.size);
        LoadSegment(executable, noffH.initData.virtualAddr,
                    noffH.initData.inFileAddr, noffH.initData.size);
    }

    delete executable;			// close file
    return TRUE;			// success
}

void
AddrSpace::LoadSegment(OpenFile *executable, unsigned int virtualAddr,
                       unsigned int fileOffset, unsigned int remaining)
{
    while (remaining > 0) {
        unsigned int vpn = virtualAddr / PageSize;
        unsigned int offset = virtualAddr % PageSize;
        ASSERT(vpn < numPages);

        // The next virtual page may map to a nonadjacent physical frame.
        unsigned int chunk = PageSize - offset;
        if (chunk > remaining) {
            chunk = remaining;
        }

        unsigned int physicalAddr =
            pageTable[vpn].physicalPage * PageSize + offset;
        int bytesRead = executable->ReadAt(
            &kernel->machine->mainMemory[physicalAddr], chunk, fileOffset);
        ASSERT(bytesRead == (int)chunk);

        virtualAddr += chunk;
        fileOffset += chunk;
        remaining -= chunk;
    }
}

//----------------------------------------------------------------------
// AddrSpace::Execute
// 	Run a user program.  Load the executable into memory, then
//	Use the calling thread to run it until Exit or a machine-wide Halt.
//
//	"fileName" is the file containing the object code to load into memory
//----------------------------------------------------------------------

void 
AddrSpace::Execute(char *fileName) 
{
    if (!Load(fileName)) {
	return;				// Missing executable or insufficient free frames
    }

    kernel->currentThread->space = this;

    this->InitRegisters();		// set the initial register values
    this->RestoreState();		// load page table register

    kernel->machine->Run();		// jump to the user program

    ASSERTNOTREACHED();			// machine->Run never returns;
					// Exit finishes the thread; Halt stops Nachos
}


//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    Machine *machine = kernel->machine;
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
    DEBUG(dbgAddr, "Initializing stack pointer: " << numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	The page table remains in this object; Thread saves user registers.
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      Install this program's mapping in the shared simulated machine.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    kernel->machine->pageTable = pageTable;
    kernel->machine->pageTableSize = numPages;
}
