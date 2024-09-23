#include "error.h"
#include <stdio.h>
#include <stdlib.h>

void ErrorExit(const char *message, int error_code)
{
    fprintf(stderr, "%s\n", message);
    exit(error_code);
}
