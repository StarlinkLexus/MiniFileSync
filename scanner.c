// scanner.c
#include "scanner.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

ArchivoMetadata lista_origen[MAX_FILES];
int cant_origen = 0;

void scan_directorio(const char *dir_base, const char *dir_actual) {

    char ruta_completa[MAX_PATH]; // Buffer para construir la ruta completa del archivo/directorio
    struct dirent *entrada;
    struct stat info;

    DIR *dir = opendir(dir_actual); // Abrir el directorio actual para leer su contenido
    if (!dir) return;

    while ((entrada = readdir(dir)) != NULL) {
        // Evitar bucles infinitos con . y ..
        if (strcmp(entrada->d_name, ".") == 0 || strcmp(entrada->d_name, "..") == 0) {
            continue;
        }
        // Construir la ruta completa del archivo/directorio actual
        snprintf(ruta_completa, sizeof(ruta_completa), "%s/%s", dir_actual, entrada->d_name);

        if (lstat(ruta_completa, &info) == -1) continue; // Si no se puede obtener información del archivo, continuar con el siguiente

        if (cant_origen < MAX_FILES) {
            // Guardar la ruta relativa para el reporte y comparacion
            const char *relativa = ruta_completa + strlen(dir_base);

            if (*relativa == '/') relativa++; // Eliminar la barra inicial para mantener la ruta relativa limpia

            // Registrar los datos comunes en la estructura de memoria
            strncpy(lista_origen[cant_origen].ruta_relativa, relativa, MAX_PATH);
            
            lista_origen[cant_origen].tamano = info.st_size;
            lista_origen[cant_origen].mtime = info.st_mtime;
            lista_origen[cant_origen].inode = info.st_ino;
            lista_origen[cant_origen].permisos = info.st_mode & 0777; // Permisos octales
            cant_origen++;
        }

        // Si es un directorio, bajar recursivamente despues de registrarlo
        if (S_ISDIR(info.st_mode)) {
            scan_directorio(dir_base, ruta_completa);
        }
    }
    closedir(dir);
}
