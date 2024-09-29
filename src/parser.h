#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include "scanner.h"
#include "error.h"
#include "symtable.h"

// Parser structure
typedef struct
{
    int line_number;
    bool has_main;
    bool in_function;
    Symtable *symtable;
} Parser;

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
 * @brief Checks if the next token matches the expected token type and returns the token.
 *
 * @param line_number Pointer to the current line number.
 * @param type The expected token type.
 * @return Token* The checked token.
 */
Token *CheckAndReturnToken(int *line_number, TOKEN_TYPE type);

/**
 * @brief Parses the program header.
 *
 * @param parser Pointer to the parser structure.
 */
void Header(Parser *parser);

/**
 * @brief Parses an expression.
 *
 * @param parser Pointer to the parser structure.
 */
void Expression(Parser *parser);

/**
 * @brief Parses all function parameters.
 *
 * @param parser Pointer to the parser structure.
 */
void Parameters(Parser *parser);

/**
 * @brief Parses a function declaration and body.
 *
 * @param parser Pointer to the parser structure.
 */
void Function(Parser *parser);

/**
 * @brief Parses an if-else block.
 *
 * @param parser Pointer to the parser structure.
 */
void IfElse(Parser *parser);

/**
 * @brief Parses a while loop block.
 *
 * @param parser Pointer to the parser structure.
 */
void WhileLoop(Parser *parser);

/**
 * @brief Parses a constant declaration.
 *
 * @param parser Pointer to the parser structure.
 */
void ConstDeclaration(Parser *parser);

/**
 * @brief Parses the program body (main parsing loop).
 *
 * @param parser Pointer to the parser structure.
 */
void ProgramBody(Parser *parser);

#endif
