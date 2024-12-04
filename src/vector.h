/**
 * @file vector.h
 * @brief Provides functionality for handling dynamic vectors, including operations to append characters and tokens,
 *        and to initialize and destroy vectors. This is used to manage collections of data structures in a memory-efficient
 *        way by reallocating space when necessary.
 *
 * @authors
 * - Igor Lacko [xlackoi00]
 */

#ifndef VECTOR_H
#define VECTOR_H

#define ALLOC_CHUNK(size) size == 0 ? 1 : size * 2

#include "types.h"

/**
 * @brief Appends a character to the vector. If (length + 1) > max_length, calls realloc
 *
 * @param vector Pointer to the vector
 * @param c Character to append
 */
void AppendChar(Vector *vector, char c);

/**
 * @brief Allocates a new vector pointer
 *
 * @return Vector*: Initialized vector pointer
 */
Vector *InitVector();

/**
 * @brief Vector destructor
 *
 * @param vector Vector pointer instance
 */
void DestroyVector(Vector *vector);

// Token vector functions
TokenVector *InitTokenVector();

void AppendToken(TokenVector *vector, Token *input_token);

void DestroyTokenVector(TokenVector *vector);

#endif
