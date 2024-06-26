#include "synch_bitmap.hh"

SynchBitmap::SynchBitmap(unsigned nitems)
{
    bitmap = new Bitmap(nitems);
    lockBitmap = new Lock("lockBitmap");
}

SynchBitmap::~SynchBitmap()
{
    delete bitmap;
    delete lockBitmap;
}

void 
SynchBitmap::FetchFrom(OpenFile *file)
{
    lockBitmap->Acquire();
    bitmap->FetchFrom(file);
    lockBitmap->Release();
}

void 
SynchBitmap::WriteBack(OpenFile *file) const
{
    lockBitmap->Acquire();
    bitmap->WriteBack(file);
    lockBitmap->Release();
}

// hay casos en los que se hace FetchFrom y WriteBack, uno después del otro
// supongo que ese caso tendría que ser atómico
// el problema es que no si o si se hace en la línea siguiente
// hago las funaciones por las dudas
void 
SynchBitmap::FetchNoRelease(OpenFile *file)
{
    lockBitmap->Acquire();
    bitmap->FetchFrom(file);
}

void 
SynchBitmap::WriteNoAquire(OpenFile *file) const
{
    bitmap->WriteBack(file);
    lockBitmap->Release();
}