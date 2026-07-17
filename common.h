// common.h
#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>
#include <time.h>

#define MAX_PATH 1024
#define MAX_FILES 500
#define SHM_NAME "/shm_sync_stats"
#define SEM_NAME "/sem_sync_stats"
#define FIFO_NAME "/tmp/fifo_sync_logger"

// Estructura de estadísticas en memoria compartida (Requerimiento)
struct stats {
    long archivos_copiados;
    long bytes_copiados;
    long errores;
};

// Estructura para almacenar metadatos de los archivos en memoria
typedef struct {
    char ruta_relativa[MAX_PATH];
    off_t tamano;
    time_t mtime;
} ArchivoMetadata;

#endif
