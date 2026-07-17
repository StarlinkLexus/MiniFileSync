// Librerias
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <signal.h>

// Archivos de cabecera del proyecto
#include "common.h"
#include "scanner.h"
#include "copier.h"
#include "logger.h"

// Variables globales para la gestión de procesos y tuberías (pipes)
pid_t w1_pid = -1, w2_pid = -1, logger_pid = -1;
int pipe_w1[2], pipe_w2[2];

// Manejador de señales para limpieza automática de recursos
void manejar_salida(int sig) {
    (void)sig;
    enviar_log("Señal de parada recibida. Limpiando recursos...");
    
    close(pipe_w1[1]); close(pipe_w2[1]); // Cierra escritura de los pipes para apagar workers
    if (w1_pid > 0) waitpid(w1_pid, NULL, 0);
    if (w2_pid > 0) waitpid(w2_pid, NULL, 0);
    if (logger_pid > 0) { kill(logger_pid, SIGKILL); waitpid(logger_pid, NULL, 0); }

    shm_unlink(SHM_NAME);
    sem_unlink(SEM_NAME);
    unlink(FIFO_NAME);
    exit(0);
}

// Lógica para sincronizar únicamente archivos nuevos o modificados (Sincronización Incremental)
int necesita_sincronizacion(const char *dir_backup, ArchivoMetadata file) {
    char ruta_destino[MAX_PATH * 2];
    snprintf(ruta_destino, sizeof(ruta_destino), "%s/%s", dir_backup, file.ruta_relativa);

    struct stat info_destino;
    if (stat(ruta_destino, &info_destino) == -1) return 1; // No existe -> se copia

    return (info_destino.st_size != file.tamano || info_destino.st_mtime < file.mtime); // ¿Cambió?
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Uso: %s <dir_origen> <dir_backup> [-d]\n", argv[0]);
        return 1;
    }

    char dir_origen[MAX_PATH], dir_backup[MAX_PATH];
    if (!realpath(argv[1], dir_origen) || !realpath(argv[2], dir_backup)) return 1;

    // PROCESO DAEMON 
        if (argc == 4 && strcmp(argv[3], "-d") == 0) {
        pid_t pid = fork();
        if (pid < 0) exit(1);
        if (pid > 0) exit(0); // Cierra proceso padre de la consola
        setsid();
        chdir("/");
        int fd_null = open("/dev/null", O_RDWR);
        dup2(fd_null, STDIN_FILENO); dup2(fd_null, STDOUT_FILENO); dup2(fd_null, STDERR_FILENO);
        close(fd_null);
    }

    signal(SIGINT, manejar_salida);
    signal(SIGTERM, manejar_salida);

    //CONFIGURACIÓN IPC (Memoria compartida y Semaforo) 
    shm_unlink(SHM_NAME);
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(struct stats));
    struct stats *stats_shm = (struct stats *)mmap(NULL, sizeof(struct stats), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    memset(stats_shm, 0, sizeof(struct stats)); // Inicializar stats en 0

    sem_unlink(SEM_NAME);
    sem_t *semaforo = sem_open(SEM_NAME, O_CREAT, 0666, 1);

    //LANZAR LOGGER Y CREAR PIPES
    logger_pid = fork();
    if (logger_pid == 0) ejecutar_logger();
    usleep(100000); // Esperar inicialización del pipe FIFO

    pipe(pipe_w1); pipe(pipe_w2);

    //LANZAR WORKERS (Hijos dedicados a copiar)
    w1_pid = fork();
    if (w1_pid == 0) {
        close(pipe_w1[1]); close(pipe_w2[0]); close(pipe_w2[1]);
        ejecutar_worker(1, pipe_w1[0], stats_shm, semaforo);
    }

    w2_pid = fork();
    if (w2_pid == 0) {
        close(pipe_w2[1]); close(pipe_w1[0]); close(pipe_w1[1]);
        ejecutar_worker(2, pipe_w2[0], stats_shm, semaforo);
    }

    close(pipe_w1[0]); close(pipe_w2[0]); // El Monitor solo escribe

    // Indicar directorios iniciales de trabajo a los Workers
    write(pipe_w1[1], dir_origen, MAX_PATH); write(pipe_w1[1], dir_backup, MAX_PATH);
    write(pipe_w2[1], dir_origen, MAX_PATH); write(pipe_w2[1], dir_backup, MAX_PATH);

    enviar_log("Monitor activo. Iniciando sincronización continua...");

    //BUCLE DEL MONITOR (Cada 5 segundos)
    while (1) {
        cant_origen = 0;
        scan_directorio(dir_origen, dir_origen);

        int worker_sel = 0, cambios = 0;
        for (int i = 0; i < cant_origen; i++) {
            if (necesita_sincronizacion(dir_backup, lista_origen[i])) {
                cambios++;
                // Distribuir de forma alternada (Worker 1 / Worker 2)
                int pipe_activo = (worker_sel == 0) ? pipe_w1[1] : pipe_w2[1];
                write(pipe_activo, lista_origen[i].ruta_relativa, MAX_PATH);
                worker_sel = !worker_sel;
            }
        }

        if (cambios > 0) {
            char buffer_log[128];
            snprintf(buffer_log, sizeof(buffer_log), "Se asignaron %d cambios detectados.", cambios);
            enviar_log(buffer_log);
        }

        sleep(5); // Pausa de 5 segundos antes de la siguiente verificación
    }
    return 0;
}
