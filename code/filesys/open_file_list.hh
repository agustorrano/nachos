#ifndef NACHOS_FILESYS_OPENFILELIST__HH
#define NACHOS_FILESYS_OPENFILELIST__HH

#include <string.h>
#include "directory_entry.hh"

class OpenFileEntry {
public:

    bool toDelete;

    unsigned numThreads;

    int sector;

    char name[FILE_NAME_MAX_LEN + 1];

    OpenFileEntry *next;

    OpenFileEntry(int s, char* name);

    ~OpenFileEntry();
};
typedef OpenFileEntry ListNode;


class OpenFileList {
public:
    
    OpenFileList();

    ~OpenFileList();

    bool IsEmpty();

    bool OpenFileAdd(int sector, char *name);

    bool IsOpen(int sector);

    void MarkToDelete(int sector);

    void CloseOpenFile(int sector);

private:
    typedef OpenFileEntry ListNode;

    ListNode *first;  ///< Head of the list, null if list is empty.
    ListNode *last;   ///< Last element of list.
    int size;
};


#endif