#include "syscall.h"
#include "lib.c"


int
main(void)
{
    putss("HOLA!\n");
    int i = Create("a/Hola");  
    if(i != -1 ) putss("Success: Archivo Hola creado por filesyst1.");
    else putss("Error: No se pudo crear archivo Hola.");
    OpenFileId mundo = Open("a/Mundo");
    while(mundo == -1)
        mundo = Open("a/Mundo");
    putss("filesys1 escribiendo..");
    for(int i = 0; i < 100; i++)
        if(Write("Mundo", 5, mundo) == -1)
            putss("Error: No se pudo escribir mundo");
    putss("filesys1 escribió en Mundo. ");
    Close(mundo);
    Remove("a/Mundo");
    Exit(0);
    return 0;
}