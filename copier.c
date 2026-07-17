// copier.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <semaphore.h>
#include "common.h"

// Puntero local del Worker para la memoria compartida
static struct stats *shm_ptr = NULL;

void ejecutar_worker(int id, int pipe_lectura, const char *dir_origen, const char *dir_backup) {
    char ruta_tarea[MAX_PATH];
    char ruta_completa_origen[MAX_PATH * 2];
    char ruta_completa_backup[MAX_PATH * 2];
    char buffer[4096];
    char log_msg[512];

    // Mapear la memoria compartida para poder actualizar las estadisticas
    int fd_shm = shm_open(SHM_NAME, O_RDWR, 0666);
    if (fd_shm != -1) {
        shm_ptr = (struct stats *)mmap(NULL, sizeof(struct stats), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    }

    // Abrir el canal FIFO del Logger en modo escritura
    int fd_fifo = open(FIFO_NAME, O_WRONLY);

    // Bucle infinito de escucha: el Worker se duerme en el read() hasta que el Monitor le mande una ruta
    while (read(pipe_lectura, ruta_tarea, MAX_PATH) > 0) {
        // Construir rutas absolutas de trabajo
        snprintf(ruta_completa_origen, sizeof(ruta_completa_origen), "%s/%s", dir_origen, ruta_tarea);
        snprintf(ruta_completa_backup, sizeof(ruta_completa_backup), "%s/%s", dir_backup, ruta_tarea);

        // Abrir archivo origen para lectura
        int fd_origen = open(ruta_completa_origen, O_RDONLY);
        if (fd_origen == -1) {
            // Reportar error si no se puede abrir el archivo de origen
            if (shm_ptr) {
                sem_wait(&(shm_ptr->sem_control));
                shm_ptr->errores++;
                sem_post(&(shm_ptr->sem_control));
            }
            if (fd_fifo != -1) {
                snprintf(log_msg, sizeof(log_msg), "[LOGGER] Worker %d ERROR al abrir origen: %s", id, ruta_tarea);
                write(fd_fifo, log_msg, strlen(log_msg) + 1);
            }
            continue;
        }

        // Obtener permisos del archivo origen para replicarlos exactamente en el backup
        struct stat info_orig;
        mode_t permisos = 0644; // Por defecto si falla stat
        if (fstat(fd_origen, &info_orig) == 0) {
            permisos = info_orig.st_mode & 0777;
        }

        // Abrir archivo destino para escritura (Crear o truncar si ya existe) con bajos niveles
        int fd_backup = open(ruta_completa_backup, O_WRONLY | O_CREAT | O_TRUNC, permisos);
        if (fd_backup == -1) {
            close(fd_origen);
            if (shm_ptr) {
                sem_wait(&(shm_ptr->sem_control));
                shm_ptr->errores++;
                sem_post(&(shm_ptr->sem_control));
            }
            if (fd_fifo != -1) {
                snprintf(log_msg, sizeof(log_msg), "[LOGGER] Worker %d ERROR al crear destino: %s", id, ruta_tarea);
                write(fd_fifo, log_msg, strlen(log_msg) + 1);
            }
            continue;
        }

        // Proceso fisico de copia usando llamadas al sistema puras (read / write)
        ssize_t bytes_leidos;
        long bytes_copiados = 0;
        int error_escritura = 0;

        while ((bytes_leidos = read(fd_origen, buffer, sizeof(buffer))) > 0) {
            if (write(fd_backup, buffer, bytes_leidos) != bytes_leidos) {
                error_escritura = 1;
                break;
            }
            bytes_copiados += bytes_leidos;
        }

        close(fd_origen);
        close(fd_backup);

        // Seccion Critica: Actualizar estadisticas en la memoria compartida resguardado por el semaforo interno
        if (shm_ptr) {
            sem_wait(&(shm_ptr->sem_control)); // Entrar a seccion critica
            if (error_escritura || bytes_leidos == -1) {
                shm_ptr->errores++;
            } else {
                shm_ptr->archivos_copiados++;
                shm_ptr->bytes_copiados += bytes_copiados;
            }
            sem_post(&(shm_ptr->sem_control)); // Salir de seccion critica
        }

        // Enviar mensaje de log descriptivo al FIFO
        if (fd_fifo != -1) {
            if (error_escritura || bytes_leidos == -1) {
                snprintf(log_msg, sizeof(log_msg), "[LOGGER] Worker %d ERROR al copiar: %s", id, ruta_tarea);
            } else {
                snprintf(log_msg, sizeof(log_msg), "[LOGGER] Worker %d copio con exito: %s (%ld bytes)", id, ruta_tarea, bytes_copiados);
            }
            write(fd_fifo, log_msg, strlen(log_msg) + 1);
        }
    }

    // Limpieza final de descriptores al cerrar el canal de comunicacion
    if (shm_ptr) munmap(shm_ptr, sizeof(struct stats));
    if (fd_fifo != -1) close(fd_fifo);
    close(pipe_lectura);
}
