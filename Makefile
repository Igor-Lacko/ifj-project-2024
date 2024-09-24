CC= gcc
CFLAGS= -Wall -Wextra -pedantic -Werror
MODULES = scanner.c vector.c error.c parser.c
HEADERS = scanner.h vector.h error.h

main: $(MODULES) $(HEADERS)
	$(CC) $(CFLAGS) $(MODULES) -o ifj24

debug: $(MODULES) $(HEADERS)
	$(CC) $(CFLAGS) -DIFJ24_DEBUG $(MODULES) -o ifj24debug

run: main
	./ifj24

run-debug: debug
	./ifj24debug < test.txt
