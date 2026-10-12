CC = gcc
CFLAGS = -Wall -Wextra -std=c99

all: bin/ls-v1.0.0

bin/ls-v1.0.0: src/ls-v1.0.0.c
	$(CC) $(CFLAGS) src/ls-v1.0.0.c -o bin/ls-v1.0.0

clean:
	rm -rf bin/* obj/*
