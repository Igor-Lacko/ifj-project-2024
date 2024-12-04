/**
 * @file codegen.h
 * @brief Header file for code generation functions for IFJCode24.
 *
 * This header file declares macros and functions related to the generation of
 * code (IFJCode24) for various expressions, operations, control structures,
 * and I/O instructions. It includes support for operations such as arithmetic, logical
 * comparisons, function calls, and data manipulations.
 *
 * @authors
 * - Igor Lacko [xlackoi00]
 * - Rudolf Baumgartner [xbaumg01]
 */

#ifndef CODEGEN_H
#define CODEGEN_H

#include "types.h"

/*
-----------Macros for IFJCode24 instructions that don't need any frame args or have a predefined frame----------
*/

// Program header
#define IFJCODE24 fprintf(stdout, ".IFJcode24\n");

// Variable macros
#define DEFVAR(frame, name) fprintf(stdout, "DEFVAR %s@%s\n", frame, name);
#define DEFVAR_WITH_ORDER(frame, name, order) fprintf(stdout, "DEFVAR %s@%s%d\n", frame, name, order);

// Frame creation/destruction macros
#define CREATEFRAME fprintf(stdout, "CREATEFRAME\n");
#define PUSHFRAME fprintf(stdout, "PUSHFRAME\n");
#define POPFRAME fprintf(stdout, "POPFRAME\n");

// Macros for working with functions
#define FUNCTIONCALL(fun_label) fprintf(stdout, "CALL %s\n", fun_label);
#define FUNCTIONLABEL(fun_label) fprintf(stdout, "LABEL %s\n", fun_label);
#define FUNCTION_RETURN fprintf(stdout, "RETURN\n");
#define NEWPARAM(order) fprintf(stdout, "DEFVAR TF@PARAM%d\n", order); // Defines a new parameter on the temporary frame
#define DEFPARAM(order) fprintf(stdout, "DEFVAR LF@PARAM%d\n", order); // Defines a new parameter on the local frame (so after the func label)

/***** Macros for working with the data stack ******/
#define CLEARS fprintf(stdout, "CLEARS\n");

// Arithmetic instruction macros
#define ADDS fprintf(stdout, "ADDS\n");
#define SUBS fprintf(stdout, "SUBS\n");
#define MULS fprintf(stdout, "MULS\n");
#define DIVS fprintf(stdout, "DIVS\n");
#define IDIVS fprintf(stdout, "IDIVS\n");
#define FLOAT2INTS fprintf(stdout, "FLOAT2INTS\n");
#define INT2FLOATS fprintf(stdout, "INT2FLOATS\n");

// Comparison instruction macros
#define EQS fprintf(stdout, "EQS\n");
#define LTS fprintf(stdout, "LTS\n");
#define GTS fprintf(stdout, "GTS\n");

// Logical instruction macros
#define ANDS fprintf(stdout, "ANDS\n");
#define ORS fprintf(stdout, "ORS\n");
#define NOTS fprintf(stdout, "NOTS\n");

// I/O instruction macros
#define WRITE(symb) fprintf(stdout, "WRITE %s\n", symb);

// Macros for IFJCode24 exits
#define IFJ24SUCCESS fprintf(stdout, "EXIT int@0\n");

// Macros for working with labels
#define JUMP(label) fprintf(stdout, "JUMP %s\n", label);
#define JUMP_WITH_ORDER(label, order) fprintf(stdout, "JUMP %s%d\n", label, order);
#define LABEL(label) fprintf(stdout, "LABEL %s\n", label);

// Conditional macros
#define JUMPIFEQ(label, symb1, symb2, order) fprintf(stdout, "JUMPIFEQ %s%d %s %s\n", label, order, symb1, symb2);
#define JUMPIFNEQ(label, symb1, symb2, order) fprintf(stdout, "JUMPIFNEQ %s%d %s %s\n", label, order, symb1, symb2);

/*
----------End of help macros-----------
*/

/**
 * @brief Generates IFJ24 code for the given expression in postfix form
 *
 * @param parser Used to check for redefinition, line numbers in case of an error, etc.
 * @param postfix The postfix expression contained in a TokenVector
 * @param var The variable that the expression is being assigned to. If NULL, assume we are in a conditional statement
 *
 * @note Also to expand on the previous statement, if we are expecting a boolean expression, it will be put into B0
 *
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

// Initial codegen that prints the IFJCode24 for defining some global registers
void InitRegisters();

/**
 * @brief Defines a IFJCode24 variable, basically just a nicely named wrapper for fprintf
 *
 * @param name The variable name
 * @param frame The frame/scope to define the variable for
 */
void DefineVariable(const char *name, FRAME frame);

// Generates a label if@ where @ is the index of the label
void IfLabel(int count);

// The same but for else
void ElseLabel(int count);

// The same but for the end of if statements
void EndIfLabel(int count);

// The same but for while loops
void WhileLabel(int count);

// The same but for end of whiles
void EndWhileLabel(int count);

/**
 * @brief Generates code for moving src to dst.
 *
 * @param dst Destination variable.
 * @param src Source variable/symbol.
 * @param is_literal If the source is a literal.
 * @param dst_frame Destination frame type.
 */
void MOVE(Token *dst, Token *src, FRAME dst_frame);

/**
 * @brief Generates code for pushing a symbol to the data stack.
 *
 * @param attribute String representation of the token.
 * @param type Token type.
 * @param frame Frame type. Ignored if token type is not IDENTIFIER_TOKEN.
 */
void PUSHS(const char *attribute, TOKEN_TYPE type, FRAME frame);

/**
 * @brief Generates code for moving a value to a function's parameter
 *
 * @param order The number/order of the parameter, for example the first param is TF@param0... etc.
 * @param value The string representation of the value.
 * @param type Token type.
 * @param frame Frame type. Ignored if token type is not IDENTIFIER_TOKEN.
 */
void SETPARAM(int order, const char *value, TOKEN_TYPE type, FRAME frame);

// Makes the print instructions a bit less bloated
char *GetFrameString(FRAME frame);

// Gets a IFJ24Code data type from a token type
char *GetTypeStringToken(TOKEN_TYPE type);

// Gets a IFJ24Code data type from a data type
char *GetTypeStringSymbol(DATA_TYPE type);

// Generates pop into R0/F0/B0 depending on the expression type
void PopToRegister(DATA_TYPE type);

// Calls the READ instruction to read a symbol of type var->type to var at frame "frame"
// @param read_type: used if the type of the variable is NONE, in that case the type is dereived from the function's return value
void READ(VariableSymbol *var, FRAME frame, DATA_TYPE read_type);

// Calls the WRITE instruction to write a symbol to the output
void WRITEINSTRUCTION(Token *token, FRAME frame);

// Conversion instructions
void INT2FLOAT(VariableSymbol *dst, Token *value, FRAME dst_frame, FRAME src_frame);
void FLOAT2INT(VariableSymbol *dst, Token *value, FRAME dst_frame, FRAME src_frame);

// Calls the STRLEN instruction
void STRLEN(VariableSymbol *var, Token *src, FRAME dst_frame, FRAME src_frame);

// Calls the CONCAT instruction
// Comically large amount of arguments, but probably the most simple way to do it :(((
void CONCAT(VariableSymbol *dst, Token *prefix, Token *postfix, FRAME dst_frame, FRAME prefix_frame, FRAME postfix_frame);

// Calls the STR2INT instruction.
void STRI2INT(VariableSymbol *var, Token *string, Token *position, FRAME dst_frame, FRAME string_frame, FRAME position_frame);

// Calls the INT2CHAR instruction
void INT2CHAR(VariableSymbol *dst, Token *value, FRAME dst_frame, FRAME src_frame);

// Generates code for the ifj.strcmp embedded function (since it doesn't have an equivalent in IFJCode24)
void STRCMP(VariableSymbol *var, Token *str1, Token *str2, FRAME dst_frame, FRAME str1_frame, FRAME str2_frame);

// Generates code for the ifj.string embedded function (since it doesn't have an equivalent in IFJCode24)
void STRING(VariableSymbol *var, Token *src, FRAME dst_frame, FRAME src_frame);

// Generates code for the ifj.ord(string, position) embedded function using STRCMP and STRI2INT
void ORD(VariableSymbol *var, Token *string, Token *position, FRAME dst_frame, FRAME string_frame, FRAME position_frame);

// Generates code for the ifj.substring(str, beginning_index, end_index) embedded function
// Also comically large amount of arguments, but it's the most simple way to do it (even though FRAME is probably always local)
void SUBSTRING(VariableSymbol *var, Token *str, Token *beginning_index, Token *end_index, FRAME dst_frame, FRAME src_frame, FRAME beginning_frame, FRAME end_frame);

/**
 * @brief Writes the string literal passed in as a param in a IFJCode24 compatible way.
 */
void WriteStringLiteral(const char *str);

/**
 * @brief Pushes the return value of a function to the data stack, depending on the return type.
 *
 * @note The return value was in R0/F0/B0 before this function was called.
 *
 * @param type The return type of the function.
 */
void PushRetValue(DATA_TYPE type);

#endif