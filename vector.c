#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vector.h"
#include "error.h"

Vector *InitVector()
{
    return calloc(1, sizeof(Vector));
}

void DestroyVector(Vector *vector)
{
    if (vector->value != NULL)
        free(vector->value);
    free(vector);
}

void AppendChar(Vector *vector, char c)
{
    // check if length + 1 > capacity, if yes allocate more memory
    if (vector->length + 1 > vector->max_length)
    {
        if ((vector->value = realloc(vector->value, (vector->max_length) + ALLOC_CHUNK)) == NULL)
        {
            ErrorExit(ERROR_INTERNAL, "Compiler internal error: Memory allocation failed");
        }
        vector->max_length += ALLOC_CHUNK;
    }

    // append new character
    (vector->value)[vector->length] = c;
    vector->length++;
}
