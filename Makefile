CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99 -D_GNU_SOURCE

all: bin/ls

bin/ls: ls-v1.0.0.c
mkdir -p bin obj man
$(CC) $(CFLAGS) ls-v1.0.0.c -o bin/ls

clean:
rm -rf bin/* obj/*
