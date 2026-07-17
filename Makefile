CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99
LIBS = -lrt -pthread

# Compilara de forma obligatoria minisync y el utilitario scan
all: minisync scan

# 1. Compilacion del binario principal (Sincronizador Multiproceso)
minisync: main.o scanner.o copier.o logger.o
	$(CC) $(CFLAGS) -o minisync main.o scanner.o copier.o logger.o $(LIBS)

# 2. Compilacion del comando independiente 'scan' bien enlazado con scanner.o
scan: scan.o scanner.o
	$(CC) $(CFLAGS) -o scan scan.o scanner.o $(LIBS)

# Regla explicita para compilar cada archivo .c a su .o correspondiente
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpieza total de binarios y temporales
clean:
	rm -f minisync scan *.o
	rm -f /tmp/fifo_sync_logger
