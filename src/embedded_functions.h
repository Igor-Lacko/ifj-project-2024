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
 * @brief Generates code for a embedded function call. Has to be done this way, since each embedded function has a equivalent IFJCode24 counterpart.
 * 
 * @param var: Not always needed. Used in case of assignments, etc.
 */
void EmbeddedFunctionCall(Parser *parser, FunctionSymbol *func, VariableSymbol *var);

#endif