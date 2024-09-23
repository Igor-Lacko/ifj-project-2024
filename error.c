#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void ErrorExit(int error_code, const char *message,...)
{
    va_list args;
    va_start(args, message);
    vfprintf(stderr, message, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(error_code);
}
