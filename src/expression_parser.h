#ifndef EXPRESSION_PARSER_H
#define EXPRESSION_PARSER_H

#include "types.h"

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
 * @brief Checks if the token is a nullable type, used for checking relational expressions for type incompatibility
 * 
 * @param operand The operand to check 
 * @param symtable The symtable to search in
 * @return true It is nullable
 * @return false It is not nullable
 */
bool IsNullable(Token *operand, Symtable *symtable);

/**
 * @brief Help function to avoid double frees, utilizes IsTokenInString
 * 
 * @param postfix The token vector to search in and destroy
 * @param stack The stack to search in and destroy
 */
void DestroyStackAndVector(TokenVector *postfix, ExpressionStack *stack);


/*----------DEBUG FUNCTIONS----------*/
void PrintPostfix(TokenVector *postfix);

#endif