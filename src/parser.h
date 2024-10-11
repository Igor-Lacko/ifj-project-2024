#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include "scanner.h"
#include "error.h"
#include "symtable.h"
#include "stack.h"

// Parser structure
typedef struct
{
    int nested_level;
    int line_number;
    bool has_main;
    bool in_function;
    Symtable *symtable; // Current symtable (top of the stack), local variables
    Symtable *global_symtable; // Only for functions
    SymtableStack *symtable_stack;
} Parser;

// Help macro to free resources in case of invalid param
#define INVALID_PARAM_TYPE \
{   PrintError("Error in semantic analysis: Line %d: Invalid parameter type for function call for function '%s'",\
    fun->name);\
    DestroyToken(token);\
    SymtableStackDestroy(parser->symtable_stack);\
    DestroySymtable(parser->global_symtable);\
    exit(ERROR_SEMANTIC_TYPECOUNT_FUNCTION);\
}

// Help macro to check if the parameter matches the previous function usage (definition/call) in case it already exists
#define CHECK_PARAM(type1, type2) do{\
    if(type1 != type2)\
    {\
        PrintError("Error in semantic analysis: Invalid parameters for function %s", func->name);\
        DestroyToken(token);\
        SymtableStackDestroy(parser->symtable_stack);\
        DestroySymtable(parser->global_symtable);\
        exit(ERROR_SEMANTIC_TYPECOUNT_FUNCTION);\
    }\
} while(0);

// Same but for return type
#define CHECK_RETURN_VALUE do{\
    if(func->was_called && func->return_type != return_type){\
        PrintError("Error in semantic analysis: Invalid return value for function %s", func->name);\
        DestroyToken(token);\
        SymtableStackDestroy(parser->symtable_stack);\
        DestroySymtable(parser->global_symtable);\
        exit(ERROR_SEMANTIC_TYPECOUNT_FUNCTION);\
    }\
    func->return_type = return_type;\
} while(0);

// Function declarations

/**
 * @brief Checks if the next token matches the expected token type.
 *
 * @param parser Pointer to the parser structure.
 * @param type The expected token type.
 */
void CheckTokenType(Parser *parser, TOKEN_TYPE type);

/**
 * @brief Checks if the next token matches the expected keyword type.
 *
 * @param parser Pointer to the parser structure.
 * @param type The expected keyword type.
 */
void CheckKeywordType(Parser *parser, KEYWORD_TYPE type);

/**
 * @brief Checks if the next token matches the expected token type and returns the token.
 *
 * @param parser Pointer to the parser structure.
 * @param type The expected token type.
 * @return Token* The checked token.
 */
Token *CheckAndReturnToken(Parser *parser, TOKEN_TYPE type);

// A similar variant that returns true if it finds a match, false if it doesn't
bool DoesNextTokenMatch(Parser *parser, TOKEN_TYPE type);

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
 * @param func Symbol representing the function for which the parameters are read.
 */
void ParametersDefinition(Parser *parser, FunctionSymbol *func);

/**
 * @brief Parses a function definition and body.
 *
 * @param parser Pointer to the parser structure.
 */
void FunctionDefinition(Parser *parser);

/**
 * @brief A function call, checks if the params fit and calls codegen on the fly.
 * 
 * @param parser Pointer to the parser structure.
 * @param fun The function to be called.
 * @param fun_name For cases where the function is not declared yet. NULL if fun != NULL
 * @note here we are assuming that fun is a valid function which is contained in the symtable
 */
void FunctionCall(Parser *parser, FunctionSymbol *fun, const char *fun_name);

/**
 * @brief Parses the parameters for a function call.
 * 
 * @param parser Pointer to the parser structure.
 * @param fun The function to be called.
 */
void ParametersOnCall(Parser *parser, FunctionSymbol *fun);

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
