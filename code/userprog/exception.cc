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

#include <stdio.h>
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
            if (!fileSystem->Create(filename, SIZE_MAX_FILE))
                machine->WriteRegister(2, -1);
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
            if (!fileSystem->Remove(filename))
                machine->WriteRegister(2, -1);
            else { machine->WriteRegister(2, 0); }
            break; // tengo que hacer algo con la openfiles table?
        }
    
        case SC_READ: {
            // accede a los registros
            int bufferAddr = machine->ReadRegister(4);
            if (bufferAddr == 0) {
                DEBUG('e', "Error: address to buffer is null.\n");
                machine->WriteRegister(2, -1);
                break;
            }
            int sizeAddr = machine->ReadRegister(5);
            if (sizeAddr == 0) {
                DEBUG('e', "Error: address to size buffer is null.\n");
                machine->WriteRegister(2, -1);
                break;
            }
            int fdAddr = machine->ReadRegister(6);
            if (fdAddr == 0) {
                DEBUG('e', "Error: address to fd is null.\n");
                machine->WriteRegister(2, -1);
                break;
            }

            // lee de la consola o archivos
            char size[SIZE_MAX_FILE];
            if (!ReadStringFromUser(sizeAddr,
                                    size, sizeof size)) {
                DEBUG('e', "Error: ");
                machine->WriteRegister(2, -1);
                break;
            }
            char fdBuf[4];
            if (!ReadStringFromUser(fdAddr,
                                    fdBuf, sizeof fdBuf)) {
                DEBUG('e', "Error: ");
                machine->WriteRegister(2, -1);
                break;
            }
            int bufSize = atoi(size);
            char buffer[bufSize];
            int fd = atoi(fdBuf);
            if (fd == 1) {;} // error, es stdout
            if (fd == 0) { // lee de la consola
                int count = -1;
                do {
                    count++;
                    buffer[count] = synchConsole->ReadChar();
                }
                while (count < bufSize && buffer[count] != EOF);
                WriteBufferToUser(buffer, bufferAddr, count);
                machine->WriteRegister(2, count);
                break;
            }
            else { // lee de un archivo
                OpenFile* openfile = currentThread->GetOpenFile(fd);
                openfile->Read(buffer, (unsigned) bufSize);
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
            int sizeAddr = machine->ReadRegister(5);
            if (sizeAddr == 0) {
                DEBUG('e', "Error: address to size buffer is null.\n");
                machine->WriteRegister(2, -1);
                break;
            }
            int fdAddr = machine->ReadRegister(6);
            if (fdAddr == 0) {
                DEBUG('e', "Error: address to fd is null.\n");
                machine->WriteRegister(2, -1);
                break;
            }
            
            // escribe en la consola o archivos
            char size[SIZE_MAX_FILE];
            if (!ReadStringFromUser(sizeAddr,
                                    size, sizeof size)) {
                DEBUG('e', "Error: ");
                machine->WriteRegister(2, -1);
                break;
            }
            int bufSize = atoi(size);
            char buffer[bufSize];
            ReadBufferFromUser(bufferAddr, buffer, bufSize);
            char fdBuf[4];
            if (!ReadStringFromUser(fdAddr,
                                    fdBuf, sizeof fdBuf)) {
                DEBUG('e', "Error: ");
                machine->WriteRegister(2, -1);
                break;
            }
            int fd = atoi(fdBuf);
            if (fd == 0) {;} // error, es stdin
            if (fd == 1) { // escribe en la consola
                int count = -1;
                do {
                    count++;
                    synchConsole->WriteChar(buffer[count]);
                }
                while (count < bufSize);
                machine->WriteRegister(2, count);
                break;
            }
            else {
                // escribe en un archivo
                OpenFile* openfile = currentThread->GetOpenFile(fd);
                openfile->Write(buffer, (unsigned) bufSize);
            }
        }

        case SC_OPEN: {
           int filenameAddr = machine->ReadRegister(4);
            if (filenameAddr == 0) {
                DEBUG('e', "Error: address to filename string is null.\n");
            }

            char filename[FILE_NAME_MAX_LEN + 1];
            if (!ReadStringFromUser(filenameAddr,
                                    filename, sizeof filename)) {
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
            }
            DEBUG('e', "`Open` requested for file `%s`.\n", filename);
            OpenFile* openfile = fileSystem->Open(filename);
            currentThread->AddOpenFile(openfile);
            // if openfile == null then machine->WriteRegister(2, -1);
            break;
        }

        case SC_CLOSE: {
            int fid = machine->ReadRegister(4);
            DEBUG('e', "`Close` requested for id %u.\n", fid);
            OpenFile* openfile  = currentThread->RemoveOpenFile(fid);
            // sacamos el archivo de la lista de open files
            delete openfile; // esto esta bien?
            break;
        }

    /*  case SC_JOIN:{

        }
        
        case SC_EXEC: {

        }
        
        case SC_EXIT: {
            
        }

    */
        default:
            fprintf(stderr, "Unexpected system call: id %d.\n", scid);
            ASSERT(false);

    }

    IncrementPC();
}


/// By default, only system calls have their own handler.  All other
/// exception types are assigned the default handler.
void
SetExceptionHandlers()
{
    machine->SetHandler(NO_EXCEPTION,            &DefaultHandler);
    machine->SetHandler(SYSCALL_EXCEPTION,       &SyscallHandler);
    machine->SetHandler(PAGE_FAULT_EXCEPTION,    &DefaultHandler);
    machine->SetHandler(READ_ONLY_EXCEPTION,     &DefaultHandler);
    machine->SetHandler(BUS_ERROR_EXCEPTION,     &DefaultHandler);
    machine->SetHandler(ADDRESS_ERROR_EXCEPTION, &DefaultHandler);
    machine->SetHandler(OVERFLOW_EXCEPTION,      &DefaultHandler);
    machine->SetHandler(ILLEGAL_INSTR_EXCEPTION, &DefaultHandler);
}
