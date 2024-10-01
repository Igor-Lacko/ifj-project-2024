#ifndef EXPRESSION_PARSER_H
#define EXPRESSION_PARSER_H

#include "scanner.h" // tokens
#include "vector.h" // vectors for infix-to-postfix
#include "stack.h" // expression stack for infix-to-postfix
#include "parser.h" // for keeping track of line numbers mostly

typedef struct { // simple precedence table struct
    TOKEN_TYPE PRIORITY_HIGHEST[2]; // * and /
    TOKEN_TYPE PRIORITY_MIDDLE[2]; // + and -
    TOKEN_TYPE PRIORITY_LOWEST[6]; // ==, !=, >, <, >=, <=
} PrecedenceTable;

typedef enum { // helper enum for operator priority (probably only used once)
    HIGHEST = 3,
    MIDDLE = 2,
    LOWEST = 1
} OPERATOR_PRIORITY;

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
 * @return Vector* Vector containing a postfix string
 * 
 * @note This algorhitm was heavily inspired by the IAL course implementation
 */
Vector *InfixToPostfix(Parser *parser);

/**
 * @brief Evaluates a postfix expression and it's value
 * 
 * @param expression Vector whose value is the postfix form expression
 * @return Token* Token with an initialized value and type
 */
Token *EvaluatePosfixExpression(Vector *expression);



#endif