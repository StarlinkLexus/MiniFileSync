// scanner.h
#ifndef SCANNER_H
#define SCANNER_H

#include "common.h"

// Variables globales para almacenar los archivos encontrados
extern ArchivoMetadata lista_origen[MAX_FILES];
extern int cant_origen;

void scan_directorio(const char *dir_base, const char *dir_actual);

#endif
