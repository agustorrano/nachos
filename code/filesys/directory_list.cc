#include "directory_list.hh"
#include "threads/lock.hh"

DirectoryList::DirectoryList()
{
    first = last = nullptr;
    size = 0;
}

DirectoryList::~DirectoryList()
{
    DirEntry *element;

    while (first != nullptr) {
        element = first->next;
        delete first->lockDir;
        delete first;
        first = element;
    }
}

bool
DirectoryList::IsEmpty()
{
    return size == 0;
}

Lock*
DirectoryList::AddDirectory(int sector)
{
    DirEntry *ptr;
    for (ptr = first; ptr != nullptr; ptr = ptr->next) {
        if (ptr->sector == sector) {
            ptr->numThreads++;
            return ptr->lockDir;
        }
    }

    DirEntry *element = new DirEntry();
    element->sector = sector;
    element->numThreads = 1;
    element->lockDir = new Lock("subdirectoryLock");
    element->next = nullptr;
    
    if (IsEmpty()) {
        first = element;
        last = element;
    } else {
        last->next = element;
        last = element;
    }
    size++;
    return element->lockDir;
}

bool
DirectoryList::CloseDirectory(int sector)
{
    DirEntry *ptr = first, *prev_ptr = nullptr;
    for (; ptr != nullptr; prev_ptr = ptr, ptr = ptr->next) {
        if (ptr->sector == sector) {
            ptr->numThreads--;
            if (ptr->numThreads == 0) {
                if (prev_ptr) {
                    prev_ptr->next = ptr->next;
                }
                if (first == ptr) {
                    first = ptr->next;
                }
                if (last == ptr) {
                    last = prev_ptr;
                }
                delete ptr->lockDir;
                delete ptr;
                size--;
                return true;
            }
            return false;
        }
    }
    return false;
}

bool 
DirectoryList::CanDelete(int sector)
{
    for (DirEntry *ptr = first; ptr != nullptr; ptr = ptr->next) {
        if (ptr->sector == sector)
            return false;
    }
    return true;
}