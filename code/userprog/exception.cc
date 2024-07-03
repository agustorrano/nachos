/// Entry points into the Nachos kernel from user programs.
///
/// There are two kinds of things that can cause control to transfer back to
/// here from user code:
///
/// * System calls: the user code explicitly requests to call a procedure in
///   the Nachos kernel.  Right now, the only function we support is `Halt`.
///
/// * Exceptions: the user code does something that the CPU cannot handle.
///   For instance, accessing memory that does not exist, arithmetic errors,
///   etc.
///
/// Interrupts (which can also cause control to transfer from user code into
/// the Nachos kernel) are handled elsewhere.
///
/// For now, this only handles the `Halt` system call.  Everything else core-
/// dumps.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "transfer.hh"
#include "syscall.h"
#include "filesys/directory_entry.hh"
#include "threads/system.hh"
#include "lib/table.hh"
#include "filesys/file_system.hh"
#include "machine/synch_console.hh"
#include "address_space.hh"
#include "args.hh"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define SIZE_MAX_FILE 1000

static void
IncrementPC()
{
    unsigned pc;

    pc = machine->ReadRegister(PC_REG);
    machine->WriteRegister(PREV_PC_REG, pc);
    pc = machine->ReadRegister(NEXT_PC_REG);
    machine->WriteRegister(PC_REG, pc);
    pc += 4;
    machine->WriteRegister(NEXT_PC_REG, pc);
}

/// Do some default behavior for an unexpected exception.
///
/// NOTE: this function is meant specifically for unexpected exceptions.  If
/// you implement a new behavior for some exception, do not extend this
/// function: assign a new handler instead.
///
/// * `et` is the kind of exception.  The list of possible exceptions is in
///   `machine/exception_type.hh`.
static void
DefaultHandler(ExceptionType et)
{
    int exceptionArg = machine->ReadRegister(2);

    fprintf(stderr, "Unexpected user mode exception: %s, arg %d.\n",
            ExceptionTypeToString(et), exceptionArg);
    ASSERT(false);
}

static void DummyExec(void* args) {
    currentThread->space->InitRegisters(); 
    currentThread->space->RestoreState(); 
    if (args != nullptr) {
        unsigned argc = WriteArgs((char**)args);
        int sp = machine->ReadRegister(STACK_REG);
        machine->WriteRegister(4, argc);
        machine->WriteRegister(5, sp);
        sp = sp - 24;
        machine->WriteRegister(STACK_REG, sp);
    }
    machine->Run();
    ASSERT(false);
}

/// Handle a system call exception.
///
/// * `et` is the kind of exception.  The list of possible exceptions is in
///   `machine/exception_type.hh`.
///
/// The calling convention is the following:
///
/// * system call identifier in `r2`;
/// * 1st argument in `r4`;
/// * 2nd argument in `r5`;
/// * 3rd argument in `r6`;
/// * 4th argument in `r7`;
/// * the result of the system call, if any, must be put back into `r2`.
///
/// And do not forget to increment the program counter before returning. (Or
/// else you will loop making the same system call forever!)
static void
SyscallHandler(ExceptionType _et)
{
    int scid = machine->ReadRegister(2);

    switch (scid) {

        case SC_HALT: {
            DEBUG('e', "Shutdown, initiated by user program.\n");
            interrupt->Halt();
            break;
        }

        case SC_CREATE: {
            int filenameAddr = machine->ReadRegister(4);
            if (filenameAddr == 0) {
                DEBUG('e', "Error: address to filename string is null.\n");
                machine->WriteRegister(2, -1);
                break;
            }

            char filename[FILE_NAME_MAX_LEN + 1];
            if (!ReadStringFromUser(filenameAddr,
                                    filename, sizeof filename)) {
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                machine->WriteRegister(2, -1);
                break;
            }
            DEBUG('e', "`Create` requested for file `%s`.\n", filename);
            if (!fileSystem->Create(filename, SIZE_MAX_FILE, false)) {
                DEBUG('e', "Error: could not create file.\n");
                machine->WriteRegister(2, -1);
            }
            else { machine->WriteRegister(2, 0); }
            break;
        }

        case SC_REMOVE: {
            int filenameAddr = machine->ReadRegister(4);
            if (filenameAddr == 0) {
                DEBUG('e', "Error: address to filename string is null.\n");
                machine->WriteRegister(2, -1);
                break;
            }
            char filename[FILE_NAME_MAX_LEN + 1];
            if (!ReadStringFromUser(filenameAddr,
                                    filename, sizeof filename)) {
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                machine->WriteRegister(2, -1);
                break;
            }
            DEBUG('e', "`Remove` requested for file `%s`.\n", filename);
            if (!fileSystem->Remove(filename)) {
                DEBUG('e', "Error: could not remove file.\n");
                machine->WriteRegister(2, -1);
            }
            else { machine->WriteRegister(2, 0); }
            break;
        }
    
        case SC_READ: {
            // accede a los registros
            int bufferAddr = machine->ReadRegister(4);
            if (bufferAddr == 0) {
                DEBUG('e', "Error: address to buffer is null.\n");
                machine->WriteRegister(2, -1);
                break;
            }
            int size = machine->ReadRegister(5);
            if (size < 0) {
                DEBUG('e', "Error: negative size.\n");
                machine->WriteRegister(2, -1);
                break;
            }
            OpenFileId fd = machine->ReadRegister(6);
            if (fd < 0) {
                DEBUG('e', "Error: negative fd.\n");
                machine->WriteRegister(2, -1);
                break;
            }

            char buffer[size];
            if (fd == CONSOLE_OUTPUT) {
                DEBUG('e', "Error: reading from stdout.\n");
                machine->WriteRegister(2, -1);
                break;
            } 
            if (fd == CONSOLE_INPUT) { // lee de la consola
                DEBUG('e', "`Read` requested for console.\n");
                synchConsole->ReadBuffer(buffer, size);
                // int count = 0;
                // while (count < size) {
                //     buffer[count] = synchConsole->ReadChar();
                //     count++;
                // }
                WriteBufferToUser(buffer, bufferAddr, size);
                machine->WriteRegister(2, size);
                break;
            }
            else { // lee de un archivo
                DEBUG('e', "`Read` requested for fd `%d`.\n", fd);
                OpenFile* openfile = currentThread->GetOpenFile(fd);
                if (openfile == nullptr) {
                    DEBUG('e', "Error: file is not open.\n");
                    machine->WriteRegister(2, -1);
                    break; 
                }
                int count = openfile->Read(buffer, (unsigned) size);
                if (count > 0)
                    WriteBufferToUser(buffer, bufferAddr, size);
                machine->WriteRegister(2, count);
                break;
            }
        }

        case SC_WRITE: {
            // accede a los registros
            int bufferAddr = machine->ReadRegister(4);
            if (bufferAddr == 0) {
                DEBUG('e', "Error: address to buffer is null.\n");
                machine->WriteRegister(2, -1);
                break;
            }
            int size = machine->ReadRegister(5);
            if (size < 0) {
                DEBUG('e', "Error: negative size.\n");
                machine->WriteRegister(2, -1);
                break;
            }
            OpenFileId fd = machine->ReadRegister(6);
            if (fd < 0) {
                DEBUG('e', "Error: negative fd.\n");
                machine->WriteRegister(2, -1);
                break;
            }

            char buffer[size+1];
            ReadBufferFromUser(bufferAddr, buffer, sizeof buffer);
            if (fd == CONSOLE_INPUT) {
                DEBUG('e', "Error: writing in stdin.\n");
                machine->WriteRegister(2, -1);
                break;
            }
            if (fd == CONSOLE_OUTPUT) { // escribe en la consola
                DEBUG('e', "`Write` requested for console.\n");
                synchConsole->WriteBuffer(buffer, size);
                // int count = -1;
                // do {
                //     count++;
                //     synchConsole->WriteChar(buffer[count]);
                // } while (count < size);
                machine->WriteRegister(2, size);
                break;
            }
            else {  // escribe en un archivo
                DEBUG('e', "`Write` requested for fd `%d`.\n", fd);
                OpenFile* openfile = currentThread->GetOpenFile(fd);
                if (openfile == nullptr) {
                    DEBUG('e', "Error: file is not open.\n");
                    machine->WriteRegister(2, -1);
                    break; 
                }
                ReadBufferFromUser(bufferAddr, buffer, sizeof buffer);
                int count = openfile->Write(buffer, (unsigned) size);
                machine->WriteRegister(2, count);
                break;
            }
        }

        case SC_OPEN: {
            int filenameAddr = machine->ReadRegister(4);
            if (filenameAddr == 0) {
                DEBUG('e', "Error: address to filename string is null.\n");
                machine->WriteRegister(2, -1);
                break;
            }

            char filename[FILE_NAME_MAX_LEN + 1];
            if (!ReadStringFromUser(filenameAddr,
                                    filename, sizeof filename)) {
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                machine->WriteRegister(2, -1);
                break;
            }
            DEBUG('e', "`Open` requested for file `%s`.\n", filename);
            OpenFile* openfile = fileSystem->Open(filename);
            if (openfile == nullptr) {
                DEBUG('e', "Error: could not open the file.\n");
                machine->WriteRegister(2, -1);
                break; 
            }
            OpenFileId fd = currentThread->AddOpenFile(openfile);
            machine->WriteRegister(2, fd);
            break;
        }

        case SC_CLOSE: {
            OpenFileId fd = machine->ReadRegister(4);
            if (fd < 0) {
                DEBUG('e', "Error: negative fd.\n");
                machine->WriteRegister(2, -1);
                break;
            }
            DEBUG('e', "`Close` requested for fd %d.\n", fd);
            // sacamos el archivo de la lista de openfiles (si está)
            OpenFile* openfile  = currentThread->RemoveOpenFile(fd);
            delete openfile; 
            machine->WriteRegister(2, 0);
            break;
        }

        case SC_JOIN:{
            SpaceId sid = machine->ReadRegister(4);
            if (sid < 0) {
                DEBUG('e', "Error: negative pid.\n");
                machine->WriteRegister(2, -1);
                break;
            }

            if (!threadsTable->HasKey(sid)) {
                DEBUG('e', "Error: pid %d does not exists.\n", sid);
                machine->WriteRegister(2, -1);
                break;
            }

            DEBUG('e', "`Join` requested with pid %d.\n", sid);
            Thread* t = threadsTable->Remove(sid);
            int st;
            t->Join(&st);
            machine->WriteRegister(2, st);
            break;
        }
        
        case SC_EXEC: {
            int filenameAddr = machine->ReadRegister(4);
            int allowJoin = machine->ReadRegister(5);
            if (filenameAddr == 0) {
                DEBUG('e', "Error: address to filename string is null.\n");
                machine->WriteRegister(2, -1);
                break;
            }

            char filename[FILE_NAME_MAX_LEN + 1];
            if (!ReadStringFromUser(filenameAddr,
                                    filename, sizeof filename)) {
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                machine->WriteRegister(2, -1);
                break;
            }
            DEBUG('e', "`Exec` requested.\n");
            // crear un nuevo hilo
            Thread *newProc = new Thread("child", allowJoin, currentThread->GetPriority());
            
            // crear su AddressSpace
            OpenFile *executable = fileSystem->Open(filename);
            if (executable == nullptr) {
                DEBUG('e', "Error: unable to open file %s\n", filename);
                delete newProc;
                machine->WriteRegister(2,-1);
                break;
            }
            AddressSpace *space = new AddressSpace(executable);
            newProc->space = space;

            #ifndef USE_DEMANDLOADING 
            delete executable;
            #endif 

            SpaceId sid = threadsTable->Add(newProc);
            if (sid == -1) {
                DEBUG('e', "Error: too many processes.\n");
                delete space;
                delete newProc;
                machine->WriteRegister(2, -1);
                break;
            }
            machine->WriteRegister(2, sid);

            newProc->Fork(DummyExec, nullptr);
            break;
        }

        case SC_EXEC2: {
            int filenameAddr = machine->ReadRegister(4);
            int argsAddr = machine->ReadRegister(5);
            int allowJoin = machine->ReadRegister(6);
            if (filenameAddr == 0) {
                DEBUG('e', "Error: address to filename string is null.\n");
                machine->WriteRegister(2, -1);
                break;
            }

            char filename[FILE_NAME_MAX_LEN + 1];
            if (!ReadStringFromUser(filenameAddr,
                                    filename, sizeof filename)) {
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                machine->WriteRegister(2, -1);
                break;
            }

            char** args = SaveArgs(argsAddr);

            DEBUG('e', "`Exec2` requested.\n");
            // crear un nuevo hilo
            Thread *newProc = new Thread("child", allowJoin, currentThread->GetPriority());
            
            // crear su AddressSpace
            OpenFile *executable = fileSystem->Open(filename);
            if (executable == nullptr) {
                DEBUG('e', "Error: unable to open file %s\n", filename);
                delete newProc;
                machine->WriteRegister(2,-1);
                break;
            }
            AddressSpace *space = new AddressSpace(executable);
            newProc->space = space;

            #ifndef USE_DEMANDLOADING 
            delete executable;
            #endif 
            
            SpaceId sid = threadsTable->Add(newProc);
            newProc->pid = sid;
            if (sid == -1) {
                DEBUG('e', "Error: too many processes.\n");
                delete space;
                delete newProc;
                machine->WriteRegister(2, -1);
                break;
            }
            machine->WriteRegister(2, sid);
            
            newProc->Fork(DummyExec, args);
            break;
        }
            
        case SC_EXIT: {
            int status = machine->ReadRegister(4);
            DEBUG('e', "`Exit` requested with status %d.\n", status);
            // liberamos la memoria del mapa de bits
            // int numPhysPages = machine->GetNumPhysicalPages();
            // for (int i = 0; i < numPhysPages; i++)
            //     machine->freeMap->Clear(i);
            currentThread->Finish(status);
            break;
        }
        default:
            fprintf(stderr, "Unexpected system call: id %d.\n", scid);
            ASSERT(false);

    }

    IncrementPC();
}

static void
PageFaultHandler (ExceptionType _et)
{
    uint32_t vAddr = machine->ReadRegister(BAD_VADDR_REG);
    uint32_t vpn = DivRoundDown(vAddr, PAGE_SIZE);
    // DEBUG('e', "Page [%d] Fault.\n", vpn);
    int i = stats->numPageFaults++ % TLB_SIZE; 
    TranslationEntry page = currentThread->space->CheckPageinMemory(vpn);
    if (machine->GetMMU()->tlb[i].valid) {
        TranslationEntry* pageTable = currentThread->space->GetPageTable();
        pageTable[machine->GetMMU()->tlb[i].virtualPage].use = machine->GetMMU()->tlb[i].use;
        pageTable[machine->GetMMU()->tlb[i].virtualPage].dirty = machine->GetMMU()->tlb[i].dirty;
    }
    machine->GetMMU()->tlb[i] = page; 
}

static void
ReadOnlyHandler (ExceptionType _et) 
{
    int vAddr = machine->ReadRegister(BAD_VADDR_REG);
    fprintf(stderr, "Tried to modify the contents of a readOnly page. VirtualAddr: %d.\n", vAddr);
    ASSERT(false);
}

/// By default, only system calls have their own handler.  All other
/// exception types are assigned the default handler.
void
SetExceptionHandlers()
{
    machine->SetHandler(NO_EXCEPTION,            &DefaultHandler);
    machine->SetHandler(SYSCALL_EXCEPTION,       &SyscallHandler);
    machine->SetHandler(PAGE_FAULT_EXCEPTION,    &PageFaultHandler);
    machine->SetHandler(READ_ONLY_EXCEPTION,     &ReadOnlyHandler);
    machine->SetHandler(BUS_ERROR_EXCEPTION,     &DefaultHandler);
    machine->SetHandler(ADDRESS_ERROR_EXCEPTION, &DefaultHandler);
    machine->SetHandler(OVERFLOW_EXCEPTION,      &DefaultHandler);
    machine->SetHandler(ILLEGAL_INSTR_EXCEPTION, &DefaultHandler);
}
