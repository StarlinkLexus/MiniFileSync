// copier.h
#ifndef COPIER_H
#define COPIER_H

#include <semaphore.h>
#include "common.h"

int copiarArchivo(const char *origen, const char *destino, long *bytes_escritos);
void asegurar_directorios(const char *ruta_archivo);
void ejecutar_worker(int id, int pipe_lectura, struct stats *stats_shm, sem_t *semaforo);

#endif
