#include "syscall.h"
#include "lib.c"


int
main(void)
{
    putss("HOLA!\n");
    int j = Create("a/Mundo");   
    if(j != -1 ) putss("Success: Archivo Mundo creado por filesyst2.");
    else putss("Error: No se pudo crear archivo Mundo.");
    
    OpenFileId hola = Open("a/Hola");
    while(hola == -1)
        hola = Open("a/Hola");
    putss("Success: Archivo Hola abierto por filesyst2");
    
    char buf[100], rc[5];
    buf[99] = 0;
    int r= -1;
    while(r < 1) {
        r = Read(buf, 100, hola);
       // putstr("leyendo a hola");
    }
    if(strlenn(buf) == 99) {
        putss("Success: Se leyo Hola correctamente:");
        putss(buf);
    } else {
        putss("Error: Hubo un error en la lectura del archivo");
        itoa(r, rc);
        putss(buf);
        putss(rc);
        return 0;
    }
    Close(hola);
    Remove("a/Hola");
    hola = Open("a/Hola");
    if(hola == -1) putss("Success: Se elimino el archivo del directorio con exito");
    else putss("Error: Se abrio un archivo que esta previamente removido");
    Exit(0);
    return 0;
}