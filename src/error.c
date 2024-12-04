/**
 * @file error.c
 * @brief Error handling functions.
 *
 * This file contains functions that handle error reporting in the IFJCode24 compiler.
 * The `ErrorExit` function is used to report an error and exit the program, while the
 * `PrintError` function is used to print errors without terminating the program.
 * These functions help in providing detailed error messages for various stages of
 * the compilation process, such as lexical analysis, syntax analysis, and internal errors.
 *
 * Authors:
 * - Boris Semanco [xseman06]
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "error.h"

void ErrorExit(int error_code, const char *message, ...)
{
    va_list args;
    va_start(args, message);
    switch (error_code)
    {
    case ERROR_LEXICAL:
        fprintf(stderr, RED "ERROR IN LEXICAL ANALYSIS: " RESET);
        break;

    case ERROR_SYNTACTIC:
        fprintf(stderr, RED "ERROR IN SYNTAX ANALYSIS: " RESET);
        break;

    case ERROR_INTERNAL:
        fprintf(stderr, RED "INTERNAL COMPILER ERROR: " RESET);
        break;

    default:
        fprintf(stderr, RED "ERROR: " RESET);
        break;
    }
    vfprintf(stderr, message, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(error_code);
}

void PrintError(const char *message, ...)
{
    fprintf(stderr, RED "ERROR: " RESET);
    va_list args;
    va_start(args, message);
    vfprintf(stderr, message, args);
    fprintf(stderr, "\n");
    va_end(args);
}
