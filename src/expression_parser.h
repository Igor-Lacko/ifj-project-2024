#ifndef EXPRESSION_PARSER_H
#define EXPRESSION_PARSER_H

#include "scanner.h" // tokens
#include "vector.h" // vectors for infix-to-postfix
#include "stack.h" // expression stack for infix-to-postfix
#include "parser.h" // for keeping track of line numbers mostly

#define EPSILON 1e-7 // rounding error with float results, used to check for division by zero


typedef struct { // simple precedence table struct
    TOKEN_TYPE PRIORITY_HIGHEST[2]; // * and /
    TOKEN_TYPE PRIORITY_MIDDLE[2]; // + and -
    TOKEN_TYPE PRIORITY_LOWEST[6]; // ==, !=, >, <, >=, <=
} PrecedenceTable;

typedef struct { // Same as vector from vector.h, but with tokens
    Token **token_string; // string of input tokens
    int length; // current
    int capacity; // max
} TokenVector;

typedef enum { // helper enum for operator priority (probably only used once)
    HIGHEST = 3,
    MIDDLE = 2,
    LOWEST = 1
} OPERATOR_PRIORITY;

typedef struct { // struct for expression results
    DATA_TYPE type;
    void *value;
} ExpressionReturn;

/**
 * @brief Initializes a precedence table
 * 
 * @return PrecedenceTable Initialized precedence table with it's array filled by operators
 */
PrecedenceTable InitPrecedenceTable(void);

/**
 * @brief Compares the priority of the two operators
 * 
 * @param operator_1 First operator
 * @param operator_2 Second operator
 * @return int -1 if priority of the first is lesser than priority of the second, 0 if equal, 1 if greater and 2 if one of the arguments wasn't an operator
 */
int ComparePriority(TOKEN_TYPE operator_1, TOKEN_TYPE operator_2);

/**
 * @brief Converts the next expression starting at getchar() to postfix notation
 * 
 * @param parser mostly to keep track in case of an error
 * 
 * @return TokenVector* Vector containing a postfix string
 * 
 * @note This algorhitm was heavily inspired by the IAL course implementation
 */
TokenVector *InfixToPostfix(Parser *parser);

// Expression return structure constructor
ExpressionReturn *InitExpressionReturn(void);

// Expression return structure destructor
void DestroyExpressionReturn(ExpressionReturn *return_value);

/**
 * @brief Used when freeing operand tokens in the EvaluatePostfixExpression function. Checks if the token can be freed
 * 
 * @param postfix The postfix string of tokens
 * @param token The token to search for
 * @return true If the token is included in the postfix vector
 * @return false If the token is not included in the postfix vector
 */
bool IsTokenInString(TokenVector *postfix, Token *token);

/**
 * @brief Evaluates a postfix expression and it's value
 * 
 * @param postfix Vector whose value is the postfix form expression
 * @param symtable To look up symbols
 * @param parser For line numbers, etc. 
 * @return Token* Token with an initialized value and type
 */
ExpressionReturn *EvaluatePosfixExpression(TokenVector *postfix, Symtable *symtable, Parser parser);

/**
 * @brief Returns an int expression result from an expression between 2 tokens
 * 
 * @param operand_1 First operand
 * @param operand_2 Second operand
 * @param operator The operation to be done
 * @param symtable If one of the operands is an identifier, searches the symtable for it's value (assuming the token is included in the symtable since it's checked before)
 * @param zero_division Error flag which the expression parser checks after the function finishes
 * @return int Evaluated result
 */
int GetIntResult(Token *operand_1, Token *operand_2, TOKEN_TYPE operator, Symtable *symtable, bool *zero_division);

// the same, but if one of the operands is a DOUBLE_64
double GetDoubleResult(Token *operand_1, Token *operand_2, TOKEN_TYPE operator, Symtable *symtable, bool *zero_division);

// Vector functions
TokenVector *InitTokenVector();

void AppendToken(TokenVector *vector, Token *input_token);

void DestroyTokenVector(TokenVector *vector);


#endif