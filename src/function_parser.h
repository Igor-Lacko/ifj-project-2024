#ifndef FUNCTION_PARSER_H
#define FUNCTION_PARSER_G

#include "parser.h"
#include "expression_parser.h"


extern TokenVector *tokens;


/**
 * @brief Parses a function definition.
 *
 * @param parser Pointer to the parser structure.
 */
void FunctionDefinition(Parser *parser);

/**
 * @brief Parses all function parameters.
 *
 * @param parser Pointer to the parser structure.
 * @param func Symbol representing the function for which the parameters are read.
 */
void ParametersDefinition(Parser *parser, FunctionSymbol *func);

/**
 * @brief Essentialy acts as a main function for the function parser. Parses all functions and adds them and their types/parameters to the symtable.
 * 
 * @param parser Parser instance. For access to symbol tables, etc.
 * 
 * @note The function definitions have to be at nested level 0: IFJ24 doesn't support function definitions inside blocks.
 */
void ParseFunctions(Parser *parser);

/**
 * @brief Rewinds the stdin stream to it's start (before the first token was read). Need to run this after ParseFunctions() is executed.
 */
void UngetStream(Parser *parser);

#endif