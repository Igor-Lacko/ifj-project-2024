/**
 * @file error.h
 * @brief Error handling macros and function declarations.
 *
 * This header defines error codes for lexical, syntactic, semantic, and internal errors,
 * along with functions for printing error messages. The `ErrorExit` function terminates the
 * program with the given error code, while `PrintError` only prints the error message.
 *
 * Authors:
 * - Boris Semanco [xseman06]
 */

#ifndef ERROR_H
#define ERROR_H

// success return code
#define SUCCESS 0

// Return codes in case of a error
#define ERROR_LEXICAL 1
#define ERROR_SYNTACTIC 2

/*Semantic error exit codes*/
#define ERROR_SEMANTIC_UNDEFINED 3          // undefined variable/function
#define ERROR_SEMANTIC_TYPECOUNT_FUNCTION 4 // wrong count/type of function parameters, or wrong type of func return value (and nepovolene zahozeni?)
#define ERROR_SEMANTIC_REDEFINED 5          // redefinition of a variable/function, re-assignment of a constant
#define ERROR_SEMANTIC_MISSING_EXPR 6       // i don't know what this means
#define ERROR_SEMANTIC_TYPE_COMPATIBILITY 7 // wrong types in arithmetic/string/relational expressions or wrong expression type (e.g. in assignments)
#define ERROR_SEMANTIC_TYPE_DERIVATION 8    // variable type not given and can't be derived from the used expression
#define ERROR_SEMANTIC_UNUSED_VARIABLE 9    // other than unused variable, i don't know what this means
#define ERROR_SEMANTIC_OTHER 10             // other semantic errors
/*End of semantic error codes*/

// Debug macro
#define NOT_IMPLEMENTED                                             \
    fprintf(stderr, "Error: This feature is not implemented yet!"); \
    exit(ERROR_INTERNAL);

#define ERROR_INTERNAL 99 // internal compiler error, for example a failed malloc call... etc.

#define RED "\033[1m\033[31m"
#define GREEN "\033[1m\033[32m"
#define RESET "\033[0m"

/**
 * @brief Ends the compiler with the given error code and message
 *
 * @param error_code Type of the error/in which part of the compiler the error has ocurred
 * @param message Depending on error_code, the function pre_prints the error type (so if it's internal it prints "Internal compiler error: ") so pass only the rest here
 * @param ... The function accepts variable arguments for printing, like printf/fprintf using vfprintf
 */
void ErrorExit(int error_code, const char *message, ...);

// Similar to ErrorExit, but doesn't end the program
void PrintError(const char *message, ...);

#endif