CC= gcc
CFLAGS= -Wall -Wextra -pedantic -Werror

HEADERS = scanner.h vector.h error.h core_parser.h symtable.h stack.h expression_parser.h codegen.h embedded_functions.h function_parser.h

MODULES = scanner.o vector.o error.o core_parser.o symtable.o stack.o expression_parser.o codegen.o embedded_functions.o function_parser.o
DEBUG_MODULES = scanner-d.o vector-d.o error-d.o core_parser-d.o symtable-d.o stack-d.o expression_parser-d.o codegen-d.o embedded_functions-d.o function_parser-d.o

#----CURRENT DEBUG FILE----#
TEST_FILE = ../test_files/factorial_recursive.ifj24

#----CURRENT OUTPUT FILE----#
OUTPUT_FILE = ../test_outputs/testifj24.ifjcode

TARGETS = ifj24 ifj24debug

#----MAIN IFJ24 EXECUTABLE----$
all: $(MODULES) $(HEADERS)
	$(CC) $(MODULES) -o ifj24

#----EXECUTABLE WITH DEBUG ADDITIONS----#
debug: $(DEBUG_MODULES) $(HEADERS)
	$(CC) $(DEBUG_MODULES) -o ifj24debug

#----RUN THE NORMAL/DEBUG EXECUTABLE----#
run: all
	./ifj24

run-debug: debug
	./ifj24debug < $(TEST_FILE) > $(OUTPUT_FILE)

#----RULES----#
%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

%-d.o : %.c
	$(CC) $(CFLAGS) -g -DIFJ24_DEBUG -c $< -o $@

#----STAGE FOR COMMIT----#
stage:
	git add . && cd ../ && git add .

#----CLEAN----#
clean:
	rm -f $(TARGETS) *.o && clear
