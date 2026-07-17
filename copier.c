// copier.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>

#include "copier.h"
#include "common.h"
#include "logger.h"

int copiarArchivo(const char *origen, const char *destino, long *bytes_escritos) {
    int fd_origen = open(origen, O_RDONLY);
    if (fd_origen < 0) return -1;

    int fd_destino = open(destino, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_destino < 0) {
        close(fd_origen);
        return -1;
    }

    char buffer[4096];
    ssize_t bytes_leidos;
    *bytes_escritos = 0;

    while ((bytes_leidos = read(fd_origen, buffer, sizeof(buffer))) > 0) {
        ssize_t bytes_prov = write(fd_destino, buffer, bytes_leidos);
        
        if (bytes_prov < 0) {
            close(fd_origen);
            close(fd_destino);
            return -1;
        }
        *bytes_escritos += bytes_prov;
    }

    close(fd_origen);
    close(fd_destino);
    return 0;
}

// Crea las subcarpetas necesarias en el destino de forma automática
void asegurar_directorios(const char *ruta_archivo) {
    char copia[MAX_PATH];
    strncpy(copia, ruta_archivo, MAX_PATH);
    char *p = strrchr(copia, '/');
    
    if (p) {
        *p = '\0';
        char comando[MAX_PATH + 10];
        snprintf(comando, sizeof(comando), "mkdir -p %s", copia);
        system(comando); 
    }
}

void ejecutar_worker(int id, int pipe_lectura, struct stats *stats_shm, sem_t *semaforo) {
    char ruta_tarea[MAX_PATH];
    char dir_origen[MAX_PATH], dir_backup[MAX_PATH];

    read(pipe_lectura, dir_origen, MAX_PATH);
    read(pipe_lectura, dir_backup, MAX_PATH);

    while (1) {
        if (read(pipe_lectura, ruta_tarea, MAX_PATH) <= 0) break; // Terminar si el pipe se cierra

        char ruta_completa_origen[MAX_PATH * 2], ruta_completa_destino[MAX_PATH * 2];
        snprintf(ruta_completa_origen, sizeof(ruta_completa_origen), "%s/%s", dir_origen, ruta_tarea);
        snprintf(ruta_completa_destino, sizeof(ruta_completa_destino), "%s/%s", dir_backup, ruta_tarea);

        asegurar_directorios(ruta_completa_destino);

        long bytes_copiados = 0;
        int status = copiarArchivo(ruta_completa_origen, ruta_completa_destino, &bytes_copiados);

        // Actualizar estadísticas usando Semáforos de forma segura
        sem_wait(semaforo);
        if (status == 0) {
            stats_shm->archivos_copiados += 1;
            stats_shm->bytes_copiados += bytes_copiados;
            char log_msg[512];
            snprintf(log_msg, sizeof(log_msg), "Worker %d copio con exito: %s (%ld bytes)", id, ruta_tarea, bytes_copiados);
            enviar_log(log_msg);
        } else {
            stats_shm->errores += 1;
            char log_msg[512];
            snprintf(log_msg, sizeof(log_msg), "Worker %d ERROR al copiar: %s", id, ruta_tarea);
            enviar_log(log_msg);
        }
        sem_post(semaforo);
    }
    close(pipe_lectura);
    exit(0);
}
