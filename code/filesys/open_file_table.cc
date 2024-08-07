#include "open_file_table.hh"
#include "threads/lock.hh"
#include "threads/condition.hh"

OpenFileEntry::OpenFileEntry(int s) 
{
    toDelete = 0;
    numThreads = 1;
    sector = s;
    lockReadCount = new Lock("lockReadCount");
    readCount = 0;
    noReaders = new Condition("noReaders", lockReadCount);
    next = nullptr;
}

OpenFileEntry::~OpenFileEntry() 
{
    delete lockReadCount;
    delete noReaders;
}


OpenFileList::OpenFileList()
{
    first = last = nullptr;
    size = 0;
}


OpenFileList::~OpenFileList()
{
    ListNode *element;

    while (first != nullptr) {
        element = first->next;
        delete first;
        first = element;
    }
}


bool
OpenFileList::IsEmpty()
{
    return size == 0;
}


bool
OpenFileList::OpenFileAdd(int sector) 
{   
    ListNode *ptr;
    for (ptr = first; ptr != nullptr; ptr = ptr->next) {
        if (ptr->sector == sector) {
            if (!ptr->toDelete) {
                ptr->numThreads++;
                return true;
            }
            return false;
        }
    } // no lo encontró, lo agrego.
    ListNode *element = new ListNode(sector);
    if (IsEmpty()) {
        first = element;
        last = element;
    } else {
        last->next = element;
        last = element;
    }
    size++;
    return true;
}

bool
OpenFileList::IsOpen(int sector) 
{
    ListNode *ptr;
    for (ptr = first; ptr != nullptr; ptr = ptr->next)
        if (ptr->sector == sector) {
            if (ptr->numThreads > 0)
                return true;
            return false;
        }
    return false;
}

void
OpenFileList::MarkToDelete(int sector)
{
    ListNode *ptr;
    for (ptr = first; ptr != nullptr; ptr = ptr->next)
        if (ptr->sector == sector) 
            ptr->toDelete = 1;
}

bool 
OpenFileList::CloseOpenFile(int sector)
{
    ListNode *ptr = first, *prev_ptr = nullptr;
    for (;ptr != nullptr; prev_ptr = ptr, ptr = ptr->next) {
        if (ptr->sector == sector) {
            ptr->numThreads--;
            if (ptr->numThreads == 0 && ptr->toDelete) {
                // lo borramos de la lista
                if (prev_ptr) {
                prev_ptr->next = ptr->next;
                }
                if (first == ptr) {
                    first = ptr->next;
                }
                if (last == ptr) {
                    last = prev_ptr;
                }
                delete ptr;
                size--;
                return true; // retorna true si hay que llamar a release
            }
            return false;
        }
    }   
    return false;
}

void 
OpenFileList::AcquireRead(int sector)
{
    ListNode *ptr;
    for (ptr = first; ptr != nullptr; ptr = ptr->next) {
        if (ptr->sector == sector) {
            if (!ptr->lockReadCount->IsHeldByCurrentThread()) {
                ptr->lockReadCount->Acquire();
                ptr->readCount++;
                ptr->lockReadCount->Release();
            }
            return;
        }
    }
    ASSERT(0);
    return;
}

void 
OpenFileList::ReleaseRead(int sector)
{
    ListNode *ptr;
    for (ptr = first; ptr != nullptr; ptr = ptr->next) {
        if (ptr->sector == sector) {
            if (!ptr->lockReadCount->IsHeldByCurrentThread()) {
                ptr->lockReadCount->Acquire();
                ptr->readCount--;
                if (ptr->readCount == 0)
                    ptr->noReaders->Broadcast();
                ptr->lockReadCount->Release();
            }
            return;
        }
    }
    ASSERT(0);
    return;
}

void 
OpenFileList::AcquireWrite(int sector)
{
    ListNode *ptr;
    for (ptr = first; ptr != nullptr; ptr = ptr->next) {
        if (ptr->sector == sector) {
            ptr->lockReadCount->Acquire();
            while (ptr->readCount > 0)
                ptr->noReaders->Wait();
            return;
        }
    }
    ASSERT(0);
    return;
}

void 
OpenFileList::ReleaseWrite(int sector)
{
    ListNode *ptr;
    for (ptr = first; ptr != nullptr; ptr = ptr->next) {
        if (ptr->sector == sector) {
            ptr->noReaders->Broadcast();
            ptr->lockReadCount->Release();
            return;
        }
    }
    ASSERT(0);
    return;
}

OpenFileTable::OpenFileTable(int cap){
  	table = new OpenFileList[cap];
  	capacity = cap;
  	for (int i = 0; i < cap; i++)
  	  table[i] = OpenFileList();
}

OpenFileTable::~OpenFileTable(){
	delete[] table;
}

bool
OpenFileTable::OpenFileAdd(int sector) {
	int hash = getHash(sector);
	return table[hash].OpenFileAdd(sector); // lo agrego a la casilla de la tabla que corresponde
}

bool
OpenFileTable::IsOpen(int sector) {
	int hash = getHash(sector);
	return table[hash].IsOpen(sector); 
}

void
OpenFileTable::MarkToDelete(int sector) {
	int hash = getHash(sector);
	table[hash].MarkToDelete(sector); 
}


bool
OpenFileTable::CloseOpenFile(int sector) {
	int hash = getHash(sector);
	return table[hash].CloseOpenFile(sector); 
}

void 
OpenFileTable::AcquireRead(int sector)
{
    int hash = getHash(sector);
    table[hash].AcquireRead(sector);
}

void 
OpenFileTable::ReleaseRead(int sector)
{
    int hash = getHash(sector);
    table[hash].ReleaseRead(sector);
}

void 
OpenFileTable::AcquireWrite(int sector)
{
    int hash = getHash(sector);
    table[hash].AcquireWrite(sector);
}

void 
OpenFileTable::ReleaseWrite(int sector)
{
    int hash = getHash(sector);
    table[hash].ReleaseWrite(sector);
}
