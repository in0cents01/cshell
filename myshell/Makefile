# Makefile for myshell - a tiny Unix shell in C.

CC      = gcc
CFLAGS  = -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -O2 -g
TARGET  = myshell
OBJS    = main.o parser.o executor.o builtins.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

main.o: main.c parser.h executor.h builtins.h
parser.o: parser.c parser.h
executor.o: executor.c executor.h
builtins.o: builtins.c builtins.h

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
