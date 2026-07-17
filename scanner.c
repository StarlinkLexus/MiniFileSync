// scanner.c
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#include "scanner.h"

ArchivoMetadata lista_origen[MAX_FILES];
int cant_origen = 0;

void scan_directorio(const char *dir_base, const char *dir_actual) {
    char ruta_completa[MAX_PATH];
    struct dirent *entrada;
    struct stat info;

    DIR *dir = opendir(dir_actual);
    if (!dir) return;

    while ((entrada = readdir(dir)) != NULL) {
        // Evitar bucles infinitos con . y ..
        if (strcmp(entrada->d_name, ".") == 0 || strcmp(entrada->d_name, "..") == 0) {
            continue;
        }

        snprintf(ruta_completa, sizeof(ruta_completa), "%s/%s", dir_actual, entrada->d_name);

        if (lstat(ruta_completa, &info) == -1) continue;

        if (S_ISDIR(info.st_mode)) {
            // Recursión
            scan_directorio(dir_base, ruta_completa);
        } else if (S_ISREG(info.st_mode)) {
            if (cant_origen < MAX_FILES) {
                // Obtener ruta relativa respecto al directorio base de origen
                const char *relativa = ruta_completa + strlen(dir_base);

                if (*relativa == '/') relativa++;

                strncpy(lista_origen[cant_origen].ruta_relativa, relativa, MAX_PATH);
                lista_origen[cant_origen].tamano = info.st_size;
                lista_origen[cant_origen].mtime = info.st_mtime;
                cant_origen++;
            }
        }
    }
    closedir(dir);
}
