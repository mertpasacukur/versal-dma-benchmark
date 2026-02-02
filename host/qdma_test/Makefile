# QDMA Benchmark Application Makefile
# Target: VPK120 Host-Side Benchmark

CC = gcc
CFLAGS = -Wall -Wextra -O2 -g
LDFLAGS = -lpthread -lrt

# Source files
SRCS = qdma_benchmark.c
OBJS = $(SRCS:.c=.o)
TARGET = qdma_benchmark

# Default target
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/

uninstall:
	rm -f /usr/local/bin/$(TARGET)

# Run quick test
test: $(TARGET)
	./$(TARGET) -s 65536 -i 100

# Run full benchmark
benchmark: $(TARGET)
	./$(TARGET) -s 1048576 -i 1000 -v -o results.csv

.PHONY: all clean install uninstall test benchmark
