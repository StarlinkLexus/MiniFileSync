// main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include "common.h"
#include "scanner.h"

int pipe_w1[2];
int pipe_w2[2];
int fd_fifo;

struct stats *shm_ptr = NULL;

void limpiar_recursos() {
    printf("\n[MONITOR] Cerrando canales e IPCs del sistema de forma segura...\n");
    close(pipe_w1[1]);
    close(pipe_w2[1]);
    close(fd_fifo);
    
    if (shm_ptr) {
        sem_destroy(&(shm_ptr->sem_control)); // Destruye el semáforo en memoria
        munmap(shm_ptr, sizeof(struct stats));
    }
    shm_unlink(SHM_NAME);
    unlink(FIFO_NAME);
}

void capturar_signal(int sig) {
    (void)sig;
    limpiar_recursos();
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Uso: %s <directorio_origen> <directorio_backup> [-d]\n", argv[0]);
        return 1;
    }

    char dir_origen[MAX_PATH];
    char dir_backup[MAX_PATH];
    
    if (!realpath(argv[1], dir_origen) || !realpath(argv[2], dir_backup)) {
        perror("Error al verificar rutas de directorios");
        return 1;
    }

    int modo_daemon = 0;
    if (argc == 4 && strcmp(argv[3], "-d") == 0) {
        modo_daemon = 1;
    }

    signal(SIGINT, capturar_signal);
    signal(SIGTERM, capturar_signal);

    // Inicializar Memoria Compartida
    shm_unlink(SHM_NAME); // Asegura limpieza previa
    int fd_shm = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(fd_shm, sizeof(struct stats));
    shm_ptr = (struct stats *)mmap(NULL, sizeof(struct stats), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    
    //Inicializar Semáforo basado en memoria
    if (sem_init(&(shm_ptr->sem_control), 1, 1) == -1) {
        perror("Error al inicializar el semaforo en memoria compartida");
        return 1;
    }

    shm_ptr->archivos_copiados = 0;
    shm_ptr->bytes_copiados = 0;
    shm_ptr->errores = 0;

    // Inicializar FIFO para el Logger
    unlink(FIFO_NAME);
    if (mkfifo(FIFO_NAME, 0666) == -1) {
        perror("Error al crear el FIFO del Logger");
        return 1;
    }

    if (pipe(pipe_w1) == -1 || pipe(pipe_w2) == -1) {
        perror("Error al crear Pipes de control");
        return 1;
    }

    // Manejo de Daemonizacion
    if (modo_daemon) {
        pid_t pid_daemon = fork();
        if (pid_daemon < 0) exit(1);
        if (pid_daemon > 0) exit(0);
        if (setsid() < 0) exit(1);
        close(STDIN_FILENO); close(STDOUT_FILENO); close(STDERR_FILENO);
        open("/dev/null", O_RDONLY); open("/dev/null", O_WRONLY); open("/dev/null", O_WRONLY);
    }

    // Lanzar Logger y Workers
    pid_t pid_logger = fork();
    if (pid_logger == 0) {
        close(pipe_w1[0]); close(pipe_w1[1]);
        close(pipe_w2[0]); close(pipe_w2[1]);
        extern void ejecutar_logger();
        ejecutar_logger();
        exit(0);
    }
    // Lanzar Workers
    pid_t pid_w1 = fork();
    if (pid_w1 == 0) {
        close(pipe_w1[1]); close(pipe_w2[0]); close(pipe_w2[1]);
        extern void ejecutar_worker(int id, int pipe_lectura, const char *orig, const char *dest);
        ejecutar_worker(1, pipe_w1[0], dir_origen, dir_backup);
        exit(0);
    }

    pid_t pid_w2 = fork();
    if (pid_w2 == 0) {
        close(pipe_w2[1]); close(pipe_w1[0]); close(pipe_w1[1]);
        extern void ejecutar_worker(int id, int pipe_lectura, const char *orig, const char *dest);
        ejecutar_worker(2, pipe_w2[0], dir_origen, dir_backup);
        exit(0);
    }

    close(pipe_w1[0]); close(pipe_w2[0]);
    fd_fifo = open(FIFO_NAME, O_WRONLY);
    // Enviar mensaje inicial al Logger
    char log_inicial[256];
    snprintf(log_inicial, sizeof(log_inicial), "[MONITOR] Inicializacion correcta. Sincronizando: %s -> %s", dir_origen, dir_backup);
    write(fd_fifo, log_inicial, strlen(log_inicial) + 1);

    while (1) {
        cant_origen = 0;
        scan_directorio(dir_origen, dir_origen);

        int cambios_enviados = 0;

        for (int i = 0; i < cant_origen; i++) {
            char ruta_origen_completa[MAX_PATH * 2];
            snprintf(ruta_origen_completa, sizeof(ruta_origen_completa), "%s/%s", dir_origen, lista_origen[i].ruta_relativa);

            char ruta_destino_completa[MAX_PATH * 2];
            snprintf(ruta_destino_completa, sizeof(ruta_destino_completa), "%s/%s", dir_backup, lista_origen[i].ruta_relativa);

            struct stat comprobar_dir;
            if (stat(ruta_origen_completa, &comprobar_dir) == 0) {
                if (S_ISDIR(comprobar_dir.st_mode)) {
                    mkdir(ruta_destino_completa, comprobar_dir.st_mode & 0777);
                    continue;
                }
            } else {
                if (strcmp(lista_origen[i].ruta_relativa, "docs") == 0 || 
                    lista_origen[i].ruta_relativa[strlen(lista_origen[i].ruta_relativa) - 1] == '/') {
                    mkdir(ruta_destino_completa, 0755);
                    continue;
                }
            }

            struct stat info_origen;
            if (stat(ruta_origen_completa, &info_origen) == -1) continue;

            struct stat info_backup;
            int necesita_copia = 0;

            if (stat(ruta_destino_completa, &info_backup) == -1) {
                necesita_copia = 1;
            } else {
                if (info_origen.st_size != info_backup.st_size || 
                    info_origen.st_mtime > info_backup.st_mtime) {
                    necesita_copia = 1;
                }
            }

            if (necesita_copia) {
                int canal_worker = (cambios_enviados % 2 == 0) ? pipe_w1[1] : pipe_w2[1];
                write(canal_worker, lista_origen[i].ruta_relativa, MAX_PATH);
                cambios_enviados++;
            }
        }

        if (cambios_enviados > 0) {
            char msg_log[256];
            snprintf(msg_log, sizeof(msg_log), "[LOGGER] Se asignaron %d cambios detectados a los Workers.", cambios_enviados);
            write(fd_fifo, msg_log, strlen(msg_log) + 1);
            
            if (!modo_daemon) {
                sem_wait(&(shm_ptr->sem_control)); // Usa el semáforo interno
                printf("\n--- ESTADISTICAS DEL SISTEMA (SHM) ---\n");
                printf("Archivos transferidos con exito: %ld\n", shm_ptr->archivos_copiados);
                printf("Volumen total copiado: %ld bytes\n", shm_ptr->bytes_copiados);
                printf("Errores registrados: %ld\n", shm_ptr->errores);
                printf("--------------------------------------\n\n");
                sem_post(&(shm_ptr->sem_control));
            }
        }

        sleep(5);
    }
    return 0;
}
