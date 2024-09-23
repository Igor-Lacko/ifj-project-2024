CC= gcc
CFLAGS= -Wall -Wextra -pedantic -Werror
MODULES = scanner.c vector.c error.c
HEADERS = scanner.h vector.h error.h

main: $(MODULES) $(HEADERS)
	$(CC) $(CFLAGS) $(MODULES) -o ifj24

run: main
	./ifj24