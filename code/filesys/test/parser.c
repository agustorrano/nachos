#include <stdio.h>
#include <string.h>


int parseDir(char* string, char* outName, char* outDir[10]) {
    const char *delim = "/";
    int ntok = 0;
    char *saveptr;
    char *first = strtok_r(string, delim, &saveptr);
    outDir[ntok] = first;
    for (char *token = strtok_r(NULL, delim, &saveptr); token != NULL; token = strtok_r(NULL, delim, &saveptr))
    {
      ntok++;
      outDir[ntok] = token;
    }    
    strcpy(outName, outDir[ntok]);
    outDir[ntok] = NULL;
    return ntok;
}

int main() {
    char ruta[] = "./file";
    const char* nombre = "";
    char *directorios[10] = {NULL};

    int cantDir = parseDir(ruta, nombre, directorios);

    printf("Nombre del archivo: %s\n", nombre);

    printf("Directorios: ");
    for (int i = 0; i < cantDir; i++)
      printf("[%s] ", directorios[i]);
    printf("\n");
    return 0;
}
