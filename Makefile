CC = gcc
CFLAGS = -Wall -Wextra -std=c99

all: bin/ls

bin/ls: src/ls-v1.0.0.c
	$(CC) $(CFLAGS) src/ls-v1.0.0.c -o bin/ls

clean:
	rm -rf bin/* obj/*
