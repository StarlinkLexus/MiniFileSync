// scan.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "scanner.h"
#include "common.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <directorio>\n", argv[0]);
        return 1;
    }

    char dir_objetivo[MAX_PATH];
    // Obtener la ruta real o nombre base del directorio ingresado
    if (!realpath(argv[1], dir_objetivo)) {
        perror("Error al abrir el directorio");
        return 1;
    }

    // Tomar solo el nombre base de la carpeta ingresada (ej. "prueba")
    char *nombre_base_dir = strrchr(argv[1], '/');
    if (nombre_base_dir) {
        nombre_base_dir++; // Quitar la barra '/'
    } else {
        nombre_base_dir = argv[1];
    }

    cant_origen = 0;
    // Disparar la lectura recursiva profunda
    scan_directorio(dir_objetivo, dir_objetivo);

    // Iterar la lista e imprimir en el formato exacto por bloques solicitado
    for (int i = 0; i < cant_origen; i++) {
        // Extraer el nombre simple del archivo/carpeta (despues del ultimo '/')
        char *nombre_archivo = strrchr(lista_origen[i].ruta_relativa, '/');
        if (nombre_archivo) {
            nombre_archivo++;
        } else {
            nombre_archivo = lista_origen[i].ruta_relativa;
        }

        // Convertir la marca de tiempo epoch en formato legible textual de fecha
        char *time_str = ctime(&lista_origen[i].mtime);
        if (time_str) {
            time_str[strlen(time_str) - 1] = '\0'; // Quitar el salto de linea que añade ctime por defecto
        }

        printf("Archivo:  %s\n", nombre_archivo);
        printf("Ruta: %s/%s\n", nombre_base_dir, lista_origen[i].ruta_relativa);
        printf("Inode: %lu\n", (unsigned long)lista_origen[i].inode);
        printf("Tamaño: %ld bytes\n", (long)lista_origen[i].tamano);
        printf("Permisos: %o\n", lista_origen[i].permisos);
        printf("Ultima modificación: %s\n\n", time_str ? time_str : "Desconocida");
    }

    return 0;
}
