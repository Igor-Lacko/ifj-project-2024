#ifndef PARSER_H
#define PARSER_H

#include "error.h"
#include "types.h"

// Help macro to free resources in case of invalid param
#define INVALID_PARAM_TYPE \
do {   PrintError("Error in semantic analysis: Line %d: Invalid parameter type for function call for function '%s'",\
    parser->line_number, func->name);\
    DestroyTokenVector(stream);\
    SymtableStackDestroy(parser->symtable_stack);\
    DestroySymtable(parser->global_symtable);\
    exit(ERROR_SEMANTIC_TYPECOUNT_FUNCTION);\
} while(0);

// Prints error in case of invalid param count
#define INVALID_PARAM_COUNT do{\
    PrintError("Error in semantic analysis: Line %d: Invalid parameter count when calling function '%s': Expected %d, got %d",\
    func->name, func->num_of_parameters, loaded);\
    DestroyTokenVector(stream);\
    SymtableStackDestroy(parser->symtable_stack);\
    DestroySymtable(parser->global_symtable);\
    exit(ERROR_SEMANTIC_TYPECOUNT_FUNCTION);\
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
 * @brief Initializes a parser structure so that main isn't as bloated.
 */
Parser InitParser();

/**
 * @brief Gets the next token from the stream vector which is already loaded.
 * 
 * @return Token* 
 */
Token *GetNextToken(Parser *parser);

/**
 * @brief Also exists for the sole purpose of removing some bloat from main.
 */
void ProgramBegin();

// Debug function, calls print token on all tokens in the stream and exits the program
void PrintStreamTokens(Parser *parser);

/**
 * @brief Checks if the next token matches the expected token type.
 *
 * @param parser Pointer to the parser structure.
 * @param type The expected token type.
 */
void CheckTokenTypeStream(Parser *parser, TOKEN_TYPE type);

/**
 * @brief Checks if the next token matches the expected keyword type.
 *
 * @param parser Pointer to the parser structure.
 * @param type The expected keyword type.
 */
void CheckKeywordTypeStream(Parser *parser, KEYWORD_TYPE type);

/**
 * @brief Checks if the next token matches the expected token type and returns the token.
 *
 * @param parser Pointer to the parser structure.
 * @param type The expected token type.
 * @return Token* The checked token.
 */
Token *CheckAndReturnTokenStream(Parser *parser, TOKEN_TYPE type);

/**
 * @brief Checks if the next token matches the expected keyword and returns the token.
 * 
 * @param parser Pointer to the parser structure.
 * @param type The expected keyword type.
 * @return Token* The checked keyword token.
 */
Token *CheckAndReturnKeywordStream(Parser *parser, KEYWORD_TYPE type);

// Variants of the 4 functions above, but they check the loaded stream vector instead of the input stream.
void CheckTokenTypeVector(Parser *parser, TOKEN_TYPE type);
void CheckKeywordTypeVector(Parser *parser, KEYWORD_TYPE type);
Token *CheckAndReturnTokenVector(Parser *parser, TOKEN_TYPE type);
Token *CheckAndReturnKeywordVector(Parser *parser, KEYWORD_TYPE type);

/**
 * @brief Upon encountering an ID, the parser checks if it's a valid variable and '=' and returns true if yes
 */
VariableSymbol *IsVariableAssignment(Token *token, Parser *parser);

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
 * @brief A function call, checks if the params fit and calls codegen on the fly.
 * 
 * @param parser Pointer to the parser structure.
 * @param fun The function to be called.
 * @param fun_name For cases where the function is not declared yet. NULL if fun != NULL
 * @note here we are assuming that fun is a valid function which is contained in the symtable
 */
void FunctionCall(Parser *parser, FunctionSymbol *fun, const char *fun_name, DATA_TYPE expected_return);

/**
 * @brief Called upon encountering a return keyword, checks if the usage is valid or not
 * 
 * @param parser 
 */
void FunctionReturn(Parser *parser);

/**
 * @brief Generates code for a function definition.
 * 
 * @param parser Pointer to the parser structure.
 */
void FunctionDefinition(Parser *parser);

/**
 * @brief Parses the parameters for a function call.
 * 
 * @param parser Pointer to the parser structure.
 * @param fun The function to be called.
 */
void ParametersOnCall(Parser *parser, FunctionSymbol *func);

/**
 * @brief Checks if the parameter type is valid for the function call/variable assignment
 * 
 * @param param_expected Expected parameter type.
 * @param param_got Gotten parameter type. Can be valid also if it's an non-nullable type and the expected is nullable.
 * @return true The parameter type is valid.
 * @return false The parameter type is invalid.
 */
bool CheckParamType(DATA_TYPE param_expected, DATA_TYPE param_got);

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
void VarDeclaration(Parser *parser, bool const);

/**
 * @brief Generates the code for assignment to the variable passed in as a parameter
 * 
 * @param var Also check if it's not a const variable
 */
void VariableAssignment(Parser *parser, VariableSymbol *var);

/**
 * @brief Generates code for assigning a function return to variable, for example var a : i32 = Sum(10, 20, 30, 40);
 * 
 * @param parser Parser instance.
 * @param var Symbol representing the variable to be assigned to.
 * @param func Symbol representing the function whose return value will be assigned to the variable.
 */
void FunctionToVariable(Parser *parser, VariableSymbol *var, FunctionSymbol *func);

/**
 * @brief Returns a normal data type from a nullable one.
 * 
 * @note If a non-nullable type is passed, it will be returned as is.
 * 
 * @param type The data type to be converted.
 * @return DATA_TYPE 
 */
DATA_TYPE NullableToNormal(DATA_TYPE type);

/**
 * @brief Parses the program body (main parsing loop).
 *
 * @param parser Pointer to the parser structure.
 */
void ProgramBody(Parser *parser);


#endif
