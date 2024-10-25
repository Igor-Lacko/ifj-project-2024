#ifndef EMBEDDED_FUNCTIONS_H
#define EMBEDDED_FUNCTIONS_H

#include "symtable.h"
#include "core_parser.h"
#include "codegen.h"

#define EMBEDDED_FUNCTION_COUNT 13
#define MAXLENGTH_EMBEDDED_FUNCTION 10
#define MAXPARAM_EMBEDDED_FUNCTION 3

// Contains the names of all embedded function names
extern const char embedded_names[EMBEDDED_FUNCTION_COUNT][MAXLENGTH_EMBEDDED_FUNCTION];

// Contains the return values of all embedded functions
extern DATA_TYPE embedded_return_types[EMBEDDED_FUNCTION_COUNT];

// Contains the parameters for all embedded functions
extern DATA_TYPE embedded_parameters[EMBEDDED_FUNCTION_COUNT][MAXPARAM_EMBEDDED_FUNCTION];

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