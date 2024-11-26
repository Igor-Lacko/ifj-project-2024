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
 * @brief Ran in preparation for expression codegen. Replaces all variables with values known at compile time with their values.
 * 
 * @param postfix Postfix representation of the expression.
 */
void ReplaceConstants(TokenVector *postfix, Parser *parser);

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
 * @brief Checks if a variable symbol is of a nullable type
 * 
 * @note returns are self explanatory i guess
 * 
 */
bool IsNullable(DATA_TYPE type);

/**
 * @brief Help function to avoid double frees, utilizes IsTokenInString
 * 
 * @param postfix The token vector to search in and destroy
 * @param stack The stack to search in and destroy
 */
void DestroyStackAndVector(TokenVector *postfix, ExpressionStack *stack);

/**
 * @brief Generates code for a arithmetic operation between two literals
 * 
 * @param operand_left Left operand literal
 * @param operand_right Right operand literal
 * @param operator Resulting type of the expression
 */
DATA_TYPE ArithmeticOperationTwoLiterals(Token *operand_left, Token *operand_right, Token *operator);

/**
 * @brief Checks compatibility between operands, where one is a variable and the other is a literal
 * 
 * @param token The literal operand
 * @param var The variable operand
 * @return int Exit code. If 0, the operands are compatible, else exit with the returned value
 */
int CheckLiteralVarCompatibilityArithmetic(Token *literal, Token *var, Parser *parser);

/**
 * @brief Generates code for an operation between a literal and an identifier
 * 
 * @param literal The literal operand
 * @param id The identifier operand
 * @param operator The operator token
 * @param literal_top_stack If the literal is on top of the stack, to know how to generate code
 * @return DATA_TYPE The type of the expression
 */
DATA_TYPE ArithmeticOperationLiteralId(Token *literal, VariableSymbol *id, Token *operator, bool literal_top_stack);

/**
 * @brief Checks compatibility between two variables
 * 
 * @param var_lhs left hand side variable
 * @param var_rhs right hand side variable, top of the stack
 * @param parser Pointer to the parser structure
 * @return int Error code in case of an error, 0 if the variables are compatible
 */
int CheckTwoVariablesCompatibilityArithmetic(Token *var_lhs, Token *var_rhs, Parser *parser);

/**
 * @brief Generates code for an operation between two identifiers
 * 
 * @param operand_left Lhs operand
 * @param operand_right Rhs operand
 * @param operator Operation to perform
 * @return DATA_TYPE Resulting type of the expression
 */
DATA_TYPE ArithmeticOperationTwoIds(VariableSymbol *operand_left, VariableSymbol *operand_right, Token *operator);


/**
 * @brief Generates code for an operation between two literals
 * 
 * @param operand_left Lhs operand
 * @param operand_right Rhs operand
 * @param operator Operation to perform
 * @return DATA_TYPE Resulting type of the expression
 */
void BooleanOperationTwoLiterals(Token *operand_left, Token *operand_right, Token *operator);

/**
 * @brief Checks compatibility between operands, where one is a variable and the other is a literal
 * 
 * @param token The literal operand
 * @param var The variable operand
 * @param parser Pointer to the parser structure
 * @return int Exit code. If 0, the operands are compatible, else exit with the returned value
 */
int CheckLiteralVarCompatibilityBoolean(Token *literal, Token *var, Parser *parser);

/**
 * @brief Generates code for an operation between a literal and an identifier
 * 
 * @param literal The literal operand
 * @param id The identifier operand
 * @param operator The operator token
 * @param literal_top_stack If the literal is on top of the stack, to know how to generate code
 */
void BooleanOperationLiteralId(Token *literal, VariableSymbol *id, Token *operator, bool literal_top_stack);

/**
 * @brief Checks compatibility between two variables
 * 
 * @param var_lhs left hand side variable
 * @param var_rhs right hand side variable, top of the stack
 * @param parser Pointer to the parser structure
 * @return int Error code in case of an error, 0 if the variables are compatible
 */
int CheckTwoVariablesCompatibilityBoolean(Token *var_lhs, Token *var_rhs, Token *operator, Parser *parser);

/**
 * @brief Generates code for an operation between two identifiers
 * 
 * @param operator Operation to perform
 */
void BooleanOperationTwoIds(Token *operator);

/**
 * @brief Parses the expression and returns the type of the expression
 * 
 * @param postfix The postfix string of tokens
 * @param parser The parser structure
 * 
 * @return DATA_TYPE The type of the expression
 */
DATA_TYPE ParseExpression(TokenVector *postfix, Parser *parser);


/*----------DEBUG FUNCTIONS----------*/
void PrintPostfix(TokenVector *postfix);

#endif