#include "open_file_table.hh"

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

OpenFileTable::OpenFileTable(int cap){
  	table = new OpenFileList[cap];
  	numElems = 0;
  	capacity = cap;
  	for (int i = 0; i < cap; i++)
  	  table[i] = OpenFileList();
}

OpenFileTable::~OpenFileTable(){
	delete[] table;
}

void
OpenFileTable::ReHash() {
	int oldCapacity = capacity;
    capacity *= 2;
    OpenFileList* newTable = new OpenFileList[capacity];

    for (int i = 0; i < oldCapacity; ++i) {
        ListNode* current = table[i].first;
        while (current != nullptr) {
            int newHash = getHash(current->sector);

            if (newTable[newHash].first == nullptr) {
                newTable[newHash].first = current;
                newTable[newHash].last = current;
                current->next = nullptr;
            } else {
                newTable[newHash].last->next = current;
                newTable[newHash].last = current;
                current->next = nullptr;
            }

            ListNode* next = current->next;
            current = next;
        }
	}	
}

bool
OpenFileTable::OpenFileAdd(int sector, char *name) {
	int hash = getHash(sector);
    numElems++;
	return table[hash].OpenFileAdd(sector, name); // lo agrego a la casilla de la tabla que corresponde
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


void
OpenFileTable::CloseOpenFile(int sector) {
	int hash = getHash(sector);
    numElems--;
	return table[hash].CloseOpenFile(sector); 
}
