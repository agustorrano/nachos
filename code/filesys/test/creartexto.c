#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILE_SIZE (120 * 1024) // 120 KB in bytes
#define TEXT "La vida es como una bicicleta, para mantener el equilibrio debes seguir adelante.\n"

int main() {
    FILE *file;
    file = fopen("bici", "w");
    if (file == NULL) {
        printf("Error al crear el archivo.\n");
        return 1;
    }

    int text_len = strlen(TEXT);
    int bytes_written = 0;

    while (bytes_written + text_len <= FILE_SIZE) {
        fputs(TEXT, file);
        bytes_written += text_len;
    }

    // por si no es perfectamente divisible
    int remaining_bytes = FILE_SIZE - bytes_written;
    if (remaining_bytes > 0) {
        fwrite(TEXT, 1, remaining_bytes, file);
    }

    fclose(file);
    printf("Archivo creado con éxito.\n");
    return 0;
}

