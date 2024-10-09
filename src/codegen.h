#ifndef CODEGEN_H
#define CODEGEN_H

// Includes
#include "parser.h"
#include "expression_parser.h"
#include "symtable.h"

// Enum for IFJCode24 frames
typedef enum
{
    GLOBAL_FRAME,
    LOCAL_FRAME,
    TEMPORARY_FRAME
} FRAME;

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
 * @brief Generates code for a int expression given by R1 @ R2, where @ is the operator
 * 
 * @param operator Operation to be performed
 * @note We use a register structure here: The operands are in the registers R1 and R2
 */
void IntExpression(TOKEN_TYPE operator);

/**
 * @brief Similar to IntExpression, but throws an error in case of a compatibility error (so int variable @ float)
 * @note Here we actually need access to the operands to check for compatibility
 */
void FloatExpression(Parser *parser, Token *operand_1, Token *operand_2, TOKEN_TYPE operator, bool *are_incompatible);

/**
 * @brief Boolean version of FloatExpression, also can throw an error
 */
void BoolExpression(Parser *parser, Token *operand_1, Token *operand_2, TOKEN_TYPE operator, bool *are_incompatible);

// Initial codegen that prints the IFJCode24 for defining some global registers
void InitRegisters();

/**
 * @brief Defines a IFJCode24 variable, basically just a nicely named wrapper for fprintf
 * 
 * @param name The variable name
 * @param frame The frame/scope to define the variable for
 */
void DefineVariable(const char *name, FRAME frame);

/**
 * @brief Generates code for an if statement
 * 
 * @param label Name of the label to pass to the code generator
 */
void IfStatement(const char *label);

#endif