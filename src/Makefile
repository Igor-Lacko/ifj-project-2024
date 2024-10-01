CC= gcc
CFLAGS= -Wall -Wextra -pedantic -Werror

HEADERS = scanner.h vector.h error.h parser.h symtable.h stack.h expression_parser.h

MODULES = scanner.o vector.o error.o parser.o symtable.o stack.o expression_parser.o
DEBUG_MODULES = scanner-d.o vector-d.o error-d.o parser-d.o symtable-d.o stack-d.o expression_parser-d.o

TARGETS = ifj24 ifj24debug

#----MAIN IFJ24 EXECUTABLE----$
all: $(MODULES) $(HEADERS)
	$(CC) $(MODULES) -o ifj24

#----EXECUTABLE WITH DEBUG ADDITIONS----#
debug: $(DEBUG_MODULES) $(HEADERS)
	$(CC) $(DEBUG_MODULES) -o ifj24debug

#----RUN THE NORMAL/DEBUG EXECUTABLE----#
run: main
	./ifj24

run-debug: debug
	./ifj24debug < ../test_files/test.ifj24 && ./ifj24debug < ../test_files/exponent.ifj24 && ./ifj24debug < ../test_files/u8.ifj24

#----RULES----#
%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

%-d.o : %.c
	$(CC) $(CFLAGS) -DIFJ24_DEBUG -c $< -o $@

#----STAGE FOR COMMIT----#
stage:
	git add . && cd ../ && git add .

#----CLEAN----#
clean:
	rm -f $(TARGETS) *.o && clear
