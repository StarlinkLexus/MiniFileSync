CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99
LIBS = -lrt -pthread

TARGET = minisync
SRCS = main.c scanner.c copier.c logger.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)
	rm -f /tmp/fifo_sync_logger
