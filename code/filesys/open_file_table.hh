#ifndef NACHOS_FILESYS_OPENFILETABLE__HH
#define NACHOS_FILESYS_OPENFILETABLE__HH

#include <string.h>
#include "directory_entry.hh"

class Lock;
class Condition;

class OpenFileEntry {
public:

    bool toDelete;

    unsigned numThreads;

    int sector;

    Lock *lockReadCount;

    int readCount;

    Condition *noReaders;

    OpenFileEntry *next;

    OpenFileEntry(int s);

    ~OpenFileEntry();
};
typedef OpenFileEntry ListNode;


class OpenFileList {
public:
    
    OpenFileList();

    ~OpenFileList();

    bool IsEmpty();

    bool OpenFileAdd(int sector);

    bool IsOpen(int sector);

    void MarkToDelete(int sector);

    bool CloseOpenFile(int sector);

    void AcquireRead(int sector);

    void ReleaseRead(int sector);

    void AcquireWrite(int sector);

    void ReleaseWrite(int sector);

    typedef OpenFileEntry ListNode;

    ListNode *first;  ///< Head of the list, null if list is empty.
    ListNode *last;   ///< Last element of list.
    
    int size;
};

#define TABLE_INIT_CAPACITY 10

class OpenFileTable{ // tabla hash
private:
  	OpenFileList *table; // tabla donde cada casillero es una lista
	int capacity;

  	// Funcion Hash
  	int getHash(int key){
  	  return key % capacity;
  	}

public: 
    OpenFileTable(int capacity);

    ~OpenFileTable();
	
    bool OpenFileAdd(int sector);

    bool IsOpen(int sector);

    void MarkToDelete(int sector);

    bool CloseOpenFile(int sector);

    void AcquireRead(int sector);

    void ReleaseRead(int sector);

    void AcquireWrite(int sector);

    void ReleaseWrite(int sector);
};

#endif