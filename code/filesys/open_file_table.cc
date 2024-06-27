#include "open_file_table.hh"


OpenFileEntry::OpenFileEntry(char* n) 
{
    toDelete = 0;
    numThreads = 1;
    strcpy(name, n);
}

OpenFileEntry::~OpenFileEntry() 
{
    //
}

bool 
OpenFileTable::OpenFileAdd(int sector, const char* name) {
    // Buscar la entrada en el unordered_map
    auto it = openFilesTable.find(sector);
    // find devuelve el final si no lo encuentra
    if (it != openFilesTable.end()) {
        // Si se encuentra y toDelete es falso, incrementar numThreads
        if (!it->second.toDelete) {
            it->second.numThreads++; //second accede a la openfileentry
            return true;
        }
        // Si se encuentra y toDelete es verdadero, no hace nada (no puede abrirlo)
        return false;
    } 
    
    // No se encontró, agregar una nueva entrada
    OpenFileEntry newEntry = OpenFileEntry((char*)name);
    openFilesTable[sector] = newEntry;
    return true;
}

bool 
OpenFileTable::IsOpen(int sector) 
{
    auto it = openFilesTable.find(sector);
    return it != openFilesTable.end();
}


void 
OpenFileTable::MarkToDelete(int sector)
{
    auto it = openFilesTable.find(sector);
    if (it != openFilesTable.end()) { // si lo encuentra lo marca
        it->second.toDelete = 1;
    }
}

void 
OpenFileTable::CloseOpenFile(int sector)
{
    auto it = openFilesTable.find(sector);
    if (it != openFilesTable.end()) { // si lo encuentra lo marca
        it->second.numThreads--;
        if (it->second.numThreads == 0 && it->second.toDelete){
            // borrar de la tabla   
            openFilesTable.erase(it);
        }
    }
}
