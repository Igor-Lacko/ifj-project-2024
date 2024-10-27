#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vector.h"
#include "error.h"
#include "scanner.h"

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
        if ((vector->value = realloc(vector->value, sizeof(char) * ALLOC_CHUNK(vector -> length))) == NULL)
        {
            ErrorExit(ERROR_INTERNAL, "Compiler internal error: Memory allocation failed");
        }
        vector->max_length = ALLOC_CHUNK(vector -> length);
    }

    // append new character
    (vector->value)[vector->length ++] = c;
}

TokenVector *InitTokenVector()
{
    TokenVector *vector;
    if ((vector = calloc(1, sizeof(TokenVector))) == NULL)
    {
        ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
    }

    return vector;
}

void AppendToken(TokenVector *vector, Token *input_token)
{
    if ((vector->length) + 1 > (vector->capacity))
    {
        int new_capacity = ALLOC_CHUNK(vector->length); // compute the new capacity

        if ((vector->token_string = realloc(vector->token_string, new_capacity * sizeof(Token *))) == NULL)
        {
            DestroyToken(input_token);
            ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
        }

        vector->capacity = new_capacity;

        // initalize all pointers in the interval (current, capacity) to NULL
        for (int i = vector->length; i < vector->capacity; i++)
        {
            vector->token_string[i] = NULL;
        }
    }

    (vector->token_string)[vector->length++] = input_token;
}

void DestroyTokenVector(TokenVector *vector)
{
    if (vector->length != 0)
    {
        for (int i = 0; i < vector->capacity; i++)
        {
            if (vector->token_string[i] != NULL)
            {
                DestroyToken(vector->token_string[i]);
            }
        }
    }

    free(vector->token_string);
    free(vector);
}
