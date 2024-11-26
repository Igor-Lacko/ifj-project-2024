#ifndef FUNCTION_PARSER_H
#define FUNCTION_PARSER_H

#include "types.h"

/**
 * @brief Parses a function definition. DOES NOT generate code! That is the core parser's task
 *
 * @param parser Pointer to the parser structure.
 */
void ParseFunctionDefinition(Parser *parser);

/**
 * @brief Parses all function parameters.
 *
 * @param parser Pointer to the parser structure.
 * @param func Symbol representing the function for which the parameters are read.
 */
void ParseParameters(Parser *parser, FunctionSymbol *func);

/**
 * @brief Essentialy acts as a main function for the function parser. Parses all functions and adds them and their types/parameters to the symtable.
 * 
 * @param parser Parser instance. For access to symbol tables, etc.
 * 
 * @note The function definitions have to be at nested level 0: IFJ24 doesn't support function definitions inside blocks.
 */
void ParseFunctions(Parser *parser);

/**
 * @brief Stores variable into the current function's variables array.
 * 
 * @param parser 
 */
void ParseVariableDeclaration(Parser *parser);

/**
 * @brief In case of nullable if or while statement, stores the variable into an array
 * 
 * @param parser 
 */
void ParseIfWhile(Parser *parser);

#endif