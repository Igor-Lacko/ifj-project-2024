/**
 * @file embedded_functions.h
 * @brief Header file for functions related to embedded function handling in the IFJCode24 interpreter.
 *
 * This header file declares functions that manage embedded functions in the IFJCode24 interpreter.
 * These functions include parsing embedded functions, their parameters, and handling function
 * declarations and variable declarations in the parser. Embedded functions like I/O operations,
 * type conversions, and string manipulations are supported.
 *
 * Functions include parsing the function's definition, parameters, and handling function calls
 * by verifying types and generating code. The symbol tables for these functions are maintained
 * and updated accordingly.
 *
 * Authors:
 * - Igor Lacko [xlackoi00]
 */

#ifndef EMBEDDED_FUNCTIONS_H
#define EMBEDDED_FUNCTIONS_H

#include "types.h"

/**
 * @brief Checks if we have a embedded function as the next set of tokens
 *
 * @note The 'ifj' identifier was already read and it is assumed to not be a variable
 * @note The previous note can cause problems if it IS a variable. Create UngetToken() maybe?
 *
 * @param var: For type checking in assignments, NULL if the function is not being assigned
 */
FunctionSymbol *IsEmbeddedFunction(Parser *parser);

/**
 * @brief Called at the start of a program, inserts all embedded functions into the global symtable
 */
void InsertEmbeddedFunctions(Parser *parser);

/**
 * @brief Special version of ParametersOnCall for embedded functions. We aren't genwrating code here, since they have equivalent IFJCode24 instructions.
 *
 * @param parser Pointer to the parser structure.
 * @param func The symbol representing the embedded function.
 */
TokenVector *ParseEmbeddedFunctionParams(Parser *parser, FunctionSymbol *func);

/**
 * @brief Generates code for a embedded function call. Has to be done this way, since each embedded function has a equivalent IFJCode24 counterpart.
 *
 * @param var: Not always needed. Used in case of assignments, etc.
 * @note Don't forget to check for a semicolon after the call
 * @note We are expecting '(' as the next token, so we need to load the parameters
 */
void EmbeddedFunctionCall(Parser *parser, FunctionSymbol *func, VariableSymbol *var);

#endif