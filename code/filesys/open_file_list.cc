#include "open_file_list.hh"

OpenFileEntry::OpenFileEntry(int s, char* n) 
{
    toDelete = 0;
    numThreads = 1;
    sector = s;
    strcpy(name, n);
    next = nullptr;
}

OpenFileEntry::~OpenFileEntry() 
{
    //
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
OpenFileList::OpenFileAdd(int sector, char *name) 
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
    ListNode *element = new ListNode(sector, name);
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
        if (ptr->sector == sector)
            return true; // no tendriamos que fijarnos que no esté para eliminar?
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

void 
OpenFileList::CloseOpenFile(int sector)
{
    ListNode *ptr = first, *prev_ptr = nullptr;
    for (;ptr != nullptr; prev_ptr = ptr, ptr = ptr->next)
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
                //delete ptr;
                size--;
                //Remove(ptr->name); HAY QUE HACERLO
            }
            return;
        }
}
