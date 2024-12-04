/**
 * @file expression_parser.h
 * @brief Functions and macros for parsing expressions and generating code for arithmetic and boolean operations.
 *
 * This header defines functions for infix-to-postfix conversion, expression parsing, checking operand compatibility,
 * and generating code for operations on literals, variables, and identifiers.
 * It includes utility functions for handling different types of expressions (arithmetic, boolean) and optimizing
 * variable handling (e.g., constant folding, compatibility checks).
 *
 * Authors:
 * - Igor Lacko [xlackoi00]
 */

#ifndef EXPRESSION_PARSER_H
#define EXPRESSION_PARSER_H

#include "types.h"

// Used to check if a float has zero decimal places
#define EPSILON 1e-9

/**
 * @brief For compatibility of tokens and the precedence table, converts TOKEN_TYPE to PtableKey to allow indexing
 *
 * @param bracket_count Since R_ROUND_BRACKET can be a dollar sign if bracket_count is below 0
 */
PtableKey GetPtableKey(Token *token, int bracket_count);

/**
 * @brief Checks if a token is an valid operand in an expression (so ID, or a non-string literal)
 */
bool IsOperand(Token *token);

/**
 * @brief Tries to find a rule that can be applied in precedential analysis.
 * @brief That is to say, tries to reduce the items in the stack between the top and the handle to a single item.
 * @brief If no reduction can be applied, returns NOT_FOUND_RULE
 *
 *
 * @param stack Stack instance
 * @param distance Distance from the top of the stack to the handle (limits the number of possible rules that can be applied)
 *
 * @return EXPRESSION_RULE Rule to be applied, or NOT_FOUND_RULE if no rule can be applied
 */
EXPRESSION_RULE FindRule(ExpressionStack *stack, int distance);

/**
 * @brief Performs reduction on the stack according to the given rule
 *
 * @param stack Stack instance
 * @param postfix Postfix representation of the expression, append the right side of the rule to this
 * @param rule Rule to be applied
 */
void ReduceToNonterminal(ExpressionStack *stack, TokenVector *postfix, EXPRESSION_RULE rule);

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
void DestroyExpressionStackAndVector(TokenVector *postfix, ExpressionStack *stack);

// The same as the previous function, but for the EvaluationStack
void DestroyEvaluationStackAndVector(TokenVector *postfix, EvaluationStack *stack);

/**
 * @brief Checks if a float value has zero decimal places
 *
 * @param float_value String representation of the float value
 */
bool HasZeroDecimalPlaces(char *float_value);

/**
 * @brief Checks if two literals used in a expression together are compatible. (Aren't only with different types and the '/' operator)
 *
 * @param literal_left The left operand literal
 * @param literal_right The right operand literal
 * @param operator The operator token
 * @param parser Pointer to the parser structure
 *
 * @return int Exit code. If 0, the literals are compatible, else exit with the returned value
 */
int CheckLiteralsCompatibilityArithmetic(Token *literal_left, Token *literal_right, Token *operator, Parser * parser);

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
 * @param operator The operator token
 * @param parser Pointer to the parser structure
 *
 * @return int Exit code. If 0, the operands are compatible, else exit with the returned value
 */
int CheckLiteralVarCompatibilityArithmetic(Token *literal, Token *var, Token *operator, Parser * parser);

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
 *
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
 * @brief Checks the validity of a boolean expression between two literals
 *
 * @param literal_left Lhs literal
 * @param literal_right Rhs literal
 * @param operator Operation to perform
 * @param parser Pointer to the parser structure
 *
 * @return int Exit code. If 0, the literals are compatible, else exit with the returned value
 */
int CheckCompatibilityLiteralsBoolean(Token *literal_left, Token *literal_right, Token *operator, Parser * parser);

/**
 * @brief Generates code for an operation between two literals
 *
 * @param operand_left Lhs operand
 * @param operand_right Rhs operand
 * @param operator Operation to perform
 *
 * @return DATA_TYPE Resulting type of the expression
 */
void BooleanOperationTwoLiterals(Token *operand_left, Token *operand_right, Token *operator);

/**
 * @brief Checks compatibility between operands, where one is a variable and the other is a literal
 *
 * @param token The literal operand
 * @param var The variable operand
 * @param operator The operator token
 * @param parser Pointer to the parser structure
 *
 * @return int Exit code. If 0, the operands are compatible, else exit with the returned value
 */
int CheckLiteralVarCompatibilityBoolean(Token *literal, Token *var, Token *operator, Parser * parser);

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
 * @brief Help function for CheckTwoVariablesCompatibilityBoolean, converts const vars with different types to the same type if posssible
 *
 * @param var_lhs Lhs variable
 * @param var_rhs Rhs variable
 *
 * @return false in case of an error, true if the variables are compatible
 */
bool ConvertConstVarsBoolean(VariableSymbol *var_lhs, VariableSymbol *var_rhs);

/**
 * @brief Checks compatibility between two variables
 *
 * @param var_lhs left hand side variable
 * @param var_rhs right hand side variable, top of the stack
 * @param parser Pointer to the parser structure
 * @return int Error code in case of an error, 0 if the variables are compatible
 */
int CheckTwoVariablesCompatibilityBoolean(Token *var_lhs, Token *var_rhs, Token *operator, Parser * parser);

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