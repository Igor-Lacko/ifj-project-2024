#ifndef CODEGEN_H
#define CODEGEN_H

// Includes
#include "parser.h"
#include "expression_parser.h"
#include "symtable.h"

/**
 * @brief Generates IFJ24 code for the given expression in postfix form
 * 
 * @param parser Used to check for redefinition, line numbers in case of an error, etc.
 * @param postfix The postfix expression contained in a TokenVector
 * @param var The variable that the expression is being assigned to. If NULL, assume we are in a conditional statement
 * @return DATA_TYPE The data type of the expression, later used to check for errors
 */
DATA_TYPE GeneratePostfixExpression(Parser *parser, TokenVector *postfix, VariableSymbol *var);

/**
 * @brief Generates code for a int expression given by operand_1 @ operand_2, where @ is the operator
 * 
 * @param parser For line numbers and symtable
 * @param operand_1 First operand
 * @param operand_2 Second operand
 * @param operator Operation to be performed
 * @note We use a register structure here: The operands are in the registers R1 and R2
 */
void IntExpression(Parser *parser, TOKEN_TYPE operator);

/**
 * @brief Similar to IntExpression, but throws an error in case of a compatibility error (so float variable @ int)
 */
void FloatExpression(Parser *parser, Token *operand_1, Token *operand_2, TOKEN_TYPE operator);

/**
 * @brief Boolean version of FloatExpression, also can throw an error
 */
void BoolExpression(Parser *parser, Token *operand_1, Token *operand_2, TOKEN_TYPE operator);

// Initial codegen that prints the IFJCode24 for defining some global registers
void InitRegisters();

#endif