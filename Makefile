CC=gcc
CFLAGS=-Wall -Wextra -pedantic -std=c99

main: scanner.c vector.c error.c
$(CC) $(CFLAGS) -o $@ $^