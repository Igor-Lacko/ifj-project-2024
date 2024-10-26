#ifndef CODEGEN_H
#define CODEGEN_H

// Includes
#include "core_parser.h"
#include "expression_parser.h"
#include "symtable.h"

/*
-----------Macros for IFJCode24 instructions that don't need any frame args or have a predefined frame----------
*/

// Program header
#define IFJCODE24 fprintf(stdout, ".IFJcode24\n");

// Frame creation/destruction macros
#define CREATEFRAME fprintf(stdout, "CREATEFRAME\n");
#define PUSHFRAME fprintf(stdout, "PUSHFRAME\n");
#define POPFRAME fprintf(stdout, "POPFRAME\n");

// Macros for working with functions
#define FUNCTIONCALL(fun_label) fprintf(stdout, "CALL %s\n", fun_label);
#define FUNCTIONLABEL(fun_label) fprintf(stdout, "LABEL %s\n", fun_label);
#define FUNCTION_RETURN fprintf(stdout, "RETURN\n");
#define NEWPARAM(order) fprintf(stdout, "DEFVAR TF@PARAM%d\n", order); // Defines a new parameter on the temporary frame

// Macros for working with the data stack
#define CLEARS fprintf(stdout, "CLEARS\n");

// I/O instruction macros
#define WRITE(symb) fprintf(stdout, "WRITE %s\n", symb);

// Macros for IFJCode24 exits
#define IFJ24SUCCESS fprintf(stdout, "EXIT 0\n");

// Macros for working with labels
#define JUMP(label) fprintf(stdout, "JUMP %s\n", label);


/*
----------End of help macros-----------
*/

// Enum for IFJCode24 frames
typedef enum
{
    GLOBAL_FRAME,
    LOCAL_FRAME,
    TEMPORARY_FRAME
} FRAME;

// Extern variables to keep track of labels for easier jumping
extern int if_label_count;
extern int while_label_count;

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

// Generates a label if@ where @ is the index of the label
void IfLabel(FRAME frame);

// The same but for else
void ElseLabel(FRAME frame);

// The same but for the end of if statements
void EndIfLabel(FRAME frame);

// The same but for while loops
void WhileLabel(FRAME frame);

// The same but for end of whiles
void EndWhileLabel(FRAME frame);

/**
 * @brief Generates code for moving src to dst.
 * 
 * @param dst Destination variable.
 * @param src Source variable/symbol.
 * @param dst_frame Destination frame type.
 */
void MOVE(const char *dst, const char *src, FRAME dst_frame);

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

// Calls the READ instruction to read a symbol of type var->type to var at frame "frame"
void READ(VariableSymbol *var, FRAME frame);

// Conversion instructions
void INT2FLOAT(VariableSymbol *dst, const char *value, FRAME frame);
void FLOAT2INT(VariableSymbol *dst, const char *value, FRAME frame);

// Calls the STRLEN instruction
void STRLEN(VariableSymbol *dst, const char *str, FRAME frame);

// Calls the CONCAT instruction
void CONCAT(VariableSymbol *dst, const char *str1, const char *str2, FRAME frame);

// Calls the STR2INT instruction.
void STRI2INT(VariableSymbol *dst, const char *src, int position, FRAME frame);

// Calls the INT2CHAR instruction
void INT2CHAR(VariableSymbol *dst, int ascii_value, FRAME frame);

/**
 * @brief Writes the string literal passed in as a param in a IFJCode24 compatible way.
 */
void WriteStringLiteral(const char *str);

#endif