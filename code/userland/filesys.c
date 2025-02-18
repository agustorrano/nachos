/// Simple program to test whether running a user program works.
///
/// Just do a “syscall” that shuts down the OS.
///
/// NOTE: for some reason, user programs with global data structures
/// sometimes have not worked in the Nachos environment.  So be careful out
/// there!  One option is to allocate data structures as automatics within a
/// procedure, but if you do this, you have to be careful to allocate a big
/// enough stack to hold the automatics!


#include "syscall.h"
#include "lib.c"


int
main(void)
{
    char rc[5];
    rc[4] = 0;
    SpaceId t1 = Exec("filesys1", 0);
    SpaceId t2 = Exec("filesys2", 0);
    OpenFileId hola = Open("a/Hola");
    while(hola == -1)
        hola = Open("a/Hola");

    OpenFileId mundo = Open("a/Mundo");
    while(mundo == -1)
        mundo = Open("a/Mundo");
    putss("Success: Ambos archivos abiertos.");
    
    char *buf = "holaholaholaholaholaholaholaholaholaholaholaholaholaholaholaholaholaholaholaholaholaholaholaholahol";
    int w = Write(buf, 100, hola);
    if(w == -1) putss("Error: no se pudo escribir en Hola");

    char buf2[1000];
    Join(t1);
    Join(t2);
    int r = Read(buf2, 1000, mundo);
    if(r > 0) {
        putss("Success: Archivo leido.");
        itoa(r, rc);
        putss(rc);
        buf2[r] = 0;
        putss(buf2);
        putss("Se leyo correctamente de Mundo.");
    } else {
        putss("Error: Error en la lectura");
    }
    Close(hola);
    Close(mundo);
    return 0;
}
