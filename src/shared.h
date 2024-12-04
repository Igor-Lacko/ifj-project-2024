/**
 * @file shared.h
 * @brief Contains shared constants, extern variables, and data structures used across the compiler.
 *        This includes information about token types, keywords, embedded functions, and the token stream.
 *
 * Authors:
 * - Igor Lacko [xlackoi00]
 */

#ifndef SHARED_H
#define SHARED_H

#include "types.h"

// For embedded functions
#define EMBEDDED_FUNCTION_COUNT 13
#define MAXLENGTH_EMBEDDED_FUNCTION 10
#define MAXPARAM_EMBEDDED_FUNCTION 3

// For token checking
#define TOKEN_COUNT 30
#define MAXLENGTH_TOKEN 11
#define KEYWORD_COUNT 13
#define MAXLENGTH_KEYWORD 10

// Extern variables to keep track of labels for easier jumping
extern int if_label_count;
extern int while_label_count;
extern int strcmp_count;
extern int ord_count;
extern int substring_count;

// Contains string representation of token types
extern const char token_types[TOKEN_COUNT][MAXLENGTH_TOKEN];

// Contains string representation of keyword types
extern const char keyword_types[KEYWORD_COUNT][MAXLENGTH_KEYWORD];

// Contains the names of all embedded function names
extern const char embedded_names[EMBEDDED_FUNCTION_COUNT][MAXLENGTH_EMBEDDED_FUNCTION];

// Contains the return values of all embedded functions
extern DATA_TYPE embedded_return_types[EMBEDDED_FUNCTION_COUNT];

// Contains the parameters for all embedded functions
extern DATA_TYPE embedded_parameters[EMBEDDED_FUNCTION_COUNT][MAXPARAM_EMBEDDED_FUNCTION];

// Contains the entire stream stored in a dynamic array of tokens
extern TokenVector *stream;

// Index to access the stream
extern int stream_index;

#endif