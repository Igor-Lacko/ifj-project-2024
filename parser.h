#ifndef PARSER_H
#define PARSER_H

#include "scanner.h"
#include "error.h"

// Function declarations

/**
 * @brief Checks if the next token matches the expected token type.
 *
 * @param line_number Pointer to the current line number.
 * @param type The expected token type.
 */
void CheckTokenType(int *line_number, TOKEN_TYPE type);

/**
 * @brief Checks if the next token matches the expected keyword type.
 *
 * @param line_number Pointer to the current line number.
 * @param type The expected keyword type.
 */
void CheckKeywordType(int *line_number, KEYWORD_TYPE type);

/**
 * @brief Parses and checks all function parameters.
 *
 * @param line_number Pointer to the current line number.
 */
void Parameters(int *line_number);

/**
 * @brief Parses a function declaration and body.
 *
 * @param line_number Pointer to the current line number.
 */
void Function(int *line_number);

/**
 * @brief Parses an if-else block.
 *
 * @param line_number Pointer to the current line number.
 */
void IfElse(int *line_number);

/**
 * @brief Parses the program body (main parsing loop).
 *
 * @param line_number Pointer to the current line number.
 */
void ProgramBody(int *line_number);

#endif
