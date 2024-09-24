#ifndef VECTOR_H
#define VECTOR_H

#define ALLOC_CHUNK(size) size == 0 ? 1 : size * 2

// Dynamic vector structure
typedef struct
{
    int length;
    int max_length;
    char *value;
} Vector;

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

#endif
