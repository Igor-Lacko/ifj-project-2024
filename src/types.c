#include <stdlib.h>
#include "types.h"

int if_label_count = 0;
int while_label_count = 0;

const char embedded_names[EMBEDDED_FUNCTION_COUNT][MAXLENGTH_EMBEDDED_FUNCTION] = {
    "readstr",
    "readi32",
    "readf64",
    "write",
    "i2f",
    "f2i",
    "string",
    "length",
    "concat",
    "substring",
    "strcmp",
    "ord",
    "chr"};

DATA_TYPE embedded_return_types[EMBEDDED_FUNCTION_COUNT] = {
    U8_ARRAY_NULLABLE_TYPE,
    INT32_NULLABLE_TYPE,
    DOUBLE64_NULLABLE_TYPE,
    VOID_TYPE,
    DOUBLE64_TYPE,
    INT32_TYPE,
    U8_ARRAY_TYPE,
    INT32_TYPE,
    U8_ARRAY_TYPE,
    U8_ARRAY_NULLABLE_TYPE,
    INT32_TYPE,
    INT32_TYPE,
    U8_ARRAY_TYPE};

DATA_TYPE embedded_parameters[EMBEDDED_FUNCTION_COUNT][MAXPARAM_EMBEDDED_FUNCTION] = {
    {VOID_TYPE, VOID_TYPE, VOID_TYPE},
    {VOID_TYPE, VOID_TYPE, VOID_TYPE},
    {VOID_TYPE, VOID_TYPE, VOID_TYPE},
    {TERM_TYPE, VOID_TYPE, VOID_TYPE},
    {INT32_TYPE, VOID_TYPE, VOID_TYPE},
    {DOUBLE64_TYPE, VOID_TYPE, VOID_TYPE},
    {TERM_TYPE, VOID_TYPE, VOID_TYPE},
    {U8_ARRAY_TYPE, VOID_TYPE, VOID_TYPE},
    {U8_ARRAY_TYPE, U8_ARRAY_TYPE, VOID_TYPE},
    {U8_ARRAY_TYPE, INT32_TYPE, INT32_TYPE},
    {U8_ARRAY_TYPE, U8_ARRAY_TYPE, VOID_TYPE},
    {U8_ARRAY_TYPE, INT32_TYPE, VOID_TYPE},
    {INT32_TYPE, VOID_TYPE, VOID_TYPE}};

int first_token_line = 1;

TokenVector *stream = NULL;