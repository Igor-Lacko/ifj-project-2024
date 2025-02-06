CC= gcc
CFLAGS= -Wall -Wextra -pedantic -Werror

HEADERS = types.h shared.h scanner.h vector.h error.h core_parser.h symtable.h stack.h expression_parser.h codegen.h embedded_functions.h function_parser.h loop.h conditionals.h

MODULES = shared.o scanner.o vector.o error.o core_parser.o symtable.o stack.o expression_parser.o codegen.o embedded_functions.o function_parser.o loop.o conditionals.o
DEBUG_MODULES = shared-d.o scanner-d.o vector-d.o error-d.o core_parser-d.o symtable-d.o stack-d.o expression_parser-d.o codegen-d.o embedded_functions-d.o function_parser-d.o loop-d.o conditionals-d.o

TEST_FOLDER = ../tests_github/in
EXAMPLE_FOLDER = ../ifj24_examples

#----VEDUCI LOGIN----#
VEDUCI = xpogad00

#----CURRENT DEBUG FILE----#
INPUT_FILE = $(EXAMPLE_FOLDER)/underscore.ifj24

#----CURRENT OUTPUT FILE----#
OUTPUT_FILE = ../tests/test_outputs/testifj24.ifjcode

TARGETS = ifj24 ifj24debug

#----MAIN IFJ24 EXECUTABLE----$
all: $(MODULES) $(HEADERS)
	$(CC) $(MODULES) -o ifj24

#----EXECUTABLE WITH DEBUG ADDITIONS----#
debug: $(DEBUG_MODULES) $(HEADERS)
	$(CC) $(DEBUG_MODULES) -o ifj24debug

#----RUN THE NORMAL/DEBUG EXECUTABLE----#
run: all
	./ifj24 < $(INPUT_FILE) > $(OUTPUT_FILE)

run-debug: debug
	./ifj24debug < $(INPUT_FILE) > $(OUTPUT_FILE)

#----RUN TESTS----#
test: all
	./../tests_github/test.sh ../tests_github ifj24 ../interpreter/ic24int

#----RULES----#
%.o : %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

%-d.o : %.c $(HEADERS)
	$(CC) $(CFLAGS) -g -DIFJ24_DEBUG -c $< -o $@

#----STAGE FOR COMMIT----#
stage:
	git add . && cd ../ && git add .

#----CLEAN----#
clean:
	rm -f $(TARGETS) *.o && clear

#----PACK----#
pack:
	zip -j $(VEDUCI).zip *.c *.h Makefile ../rozdeleni ../doc/dokumentace.pdf && mv $(VEDUCI).zip ../submit_folder
