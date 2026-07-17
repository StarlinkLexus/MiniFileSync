// common.h
#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>
#include <time.h>
#include <semaphore.h> // Asegúrate de incluir esto

#define MAX_PATH 1024
#define MAX_FILES 500
#define SHM_NAME "/shm_sync_stats"
#define FIFO_NAME "/tmp/fifo_sync_logger"

// Estructura de estadisticas en memoria compartida con semáforo integrado
struct stats {
    sem_t sem_control; // Semáforo basado en memoria
    long archivos_copiados;
    long bytes_copiados;
    long errores;
};

typedef struct {
    char ruta_relativa[MAX_PATH];
    off_t tamano;        
    time_t mtime;        
    ino_t inode;         
    mode_t permisos;     
} ArchivoMetadata;

#endif
