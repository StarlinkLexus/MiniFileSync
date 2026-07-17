// logger.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "logger.h"
#include "common.h"

void ejecutar_logger() {
    unlink(FIFO_NAME);
    if (mkfifo(FIFO_NAME, 0666) == -1) {
        perror("Error al crear FIFO");
        exit(1);
    }

    int fd_fifo = open(FIFO_NAME, O_RDONLY);
    if (fd_fifo < 0) {
        exit(1);
    }

    char buffer[512];
    while (1) {
        ssize_t leidos = read(fd_fifo, buffer, sizeof(buffer) - 1);
        if (leidos > 0) {
            buffer[leidos] = '\0';
            printf("[LOGGER] %s\n", buffer);
            fflush(stdout);
        } else if (leidos == 0) {
            usleep(100000); // Pequeña espera para evitar saturar el CPU
        } else {
            break;
        }
    }
    close(fd_fifo);
    exit(0);
}

void enviar_log(const char *mensaje) {
    int fd_fifo = open(FIFO_NAME, O_WRONLY | O_NONBLOCK);
    if (fd_fifo >= 0) {
        time_t t = time(NULL);
        struct tm *tm_info = localtime(&t);
        char timestamp[30];
        strftime(timestamp, 26, "%Y-%m-%d %H:%M:%S", tm_info);

        char log_completo[512];
        snprintf(log_completo, sizeof(log_completo), "[%s] %s\n", timestamp, mensaje);
        write(fd_fifo, log_completo, strlen(log_completo));
        close(fd_fifo);
    }
}
