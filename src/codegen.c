#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shared.h"
#include "codegen.h"
#include "stack.h"
#include "error.h"
#include "scanner.h"
#include "vector.h"
#include "expression_parser.h"

void InitRegisters()
{
    // Result registers
    fprintf(stdout, "DEFVAR GF@$R0\n");
    fprintf(stdout, "DEFVAR GF@$F0\n");
    fprintf(stdout, "DEFVAR GF@$B0\n");
    fprintf(stdout, "DEFVAR GF@$S0\n");

    // Operand registers
    fprintf(stdout, "DEFVAR GF@$R1\n");
    fprintf(stdout, "DEFVAR GF@$R2\n");
    fprintf(stdout, "DEFVAR GF@$F1\n");
    fprintf(stdout, "DEFVAR GF@$F2\n");
    fprintf(stdout, "DEFVAR GF@$B1\n");
    fprintf(stdout, "DEFVAR GF@$B2\n");
    fprintf(stdout, "DEFVAR GF@$S1\n");
    fprintf(stdout, "DEFVAR GF@$S2\n");
}

void DefineVariable(const char *name, FRAME frame)
{
    switch(frame)
    {
        case GLOBAL_FRAME:
            fprintf(stdout, "DEFVAR GF@%s\n", name);
            break;

        case LOCAL_FRAME:
            fprintf(stdout, "DEFVAR LF@%s\n", name);
            break;

        case TEMPORARY_FRAME:
            fprintf(stdout, "DEFVAR TF@%s\n", name);
            break;
    }
}

void IntExpression(TOKEN_TYPE operator)
{
    // Initialize the result register to zero
    // At the start, operand_1 is always in the register R1, and operand_2 is in the register R2
    switch(operator)
    {
        case MULTIPLICATION_OPERATOR:
            fprintf(stdout, "MUL GF@$R0 GF@$R1 GF@$R2\n");
            break;

        case DIVISION_OPERATOR:
            fprintf(stdout, "IDIV GF@$R0 GF@$R2 GF@$R1\n");
            break;

        case ADDITION_OPERATOR:
            fprintf(stdout, "ADD GF@$R0 GF@$R1 GF@$R2\n");
            break;

        case SUBSTRACTION_OPERATOR:
            fprintf(stdout, "SUB GF@$R0 GF@$R2 GF@$R1\n");
            break;

        default:
            // This will never happen, but it also has to be here
            break;
    }

    fprintf(stdout, "PUSHS GF@$R0\n");
}

void FloatExpression(Parser *parser, Token *operand_1, Token *operand_2, TOKEN_TYPE operator, bool *are_incompatible)
{
    /*The operands are incompatible in 2 cases:
    1. Both are variables, but one is an int and the other is a float
    2. One is an int variable and the other is a float constant with a non-zero floating-point value
    NOTE: In the other case where their types are different (e.g float variable/int constant) the int constant has to be converted*/

    // Case 1
    if(operand_1->token_type == IDENTIFIER_TOKEN && operand_2->token_type == IDENTIFIER_TOKEN)
    {
        if(SymtableStackFindVariable(parser->symtable_stack, operand_1->attribute)->type != SymtableStackFindVariable(parser->symtable_stack, operand_2->attribute)->type)
        {
            *are_incompatible = true;
            return;
        }
    }

    // Case 2, operand_1 is the int variable
    else if(operand_1->token_type == IDENTIFIER_TOKEN)
    {
        if(SymtableStackFindVariable(parser->symtable_stack, operand_1->attribute)->type == INT32_TYPE)
        {
            *are_incompatible = true;
            return;
        }
    }

    // Case 2, operand 2 is the int variable
    else if(operand_2->token_type == IDENTIFIER_TOKEN)
    {
        if(SymtableStackFindVariable(parser->symtable_stack, operand_2->attribute)->type == INT32_TYPE)
        {
            *are_incompatible = true;
            return;
        }
    }

    // See if one of the operands isn't an int literal and if yes, convert it
    if(operand_1->token_type == INTEGER_32) fprintf(stdout, "INT2FLOAT GF@$F1 GF@$R1\n");
    if(operand_2->token_type == INTEGER_32) fprintf(stdout, "INT2FLOAT GF@$F2 GF@$R2\n");

    // Perform the given operation
    // At the start, operand 1 is in F1, operand 2 is in F2
    switch(operator)
    {
        case MULTIPLICATION_OPERATOR:
            fprintf(stdout, "MUL GF@$F0 GF@$F1 GF@$F2\n");
            break;

        case DIVISION_OPERATOR:
            fprintf(stdout, "DIV GF@$F0 GF@$F2 GF@$F1\n");
            break;

        case ADDITION_OPERATOR:
            fprintf(stdout, "ADD GF@$F0 GF@$F1 GF@$F2\n");
            break;

        case SUBSTRACTION_OPERATOR:
            fprintf(stdout, "SUB GF@$F0 GF@$F2 GF@$F1\n");
            break;

        default:
            // This will never happen, but it also has to be here
            break;
    }

    fprintf(stdout, "PUSHS GF@$F0\n");
}

void BoolExpression(Parser *parser, Token *operand_1, Token *operand_2, TOKEN_TYPE operator, bool *are_incompatible)
{
    // So that we knpow which registers to use
    bool has_floats = false;

    // For quicker access to types
    VariableSymbol *symb1 = SymtableStackFindVariable(parser->symtable_stack, operand_1->attribute),
    *symb2 = SymtableStackFindVariable(parser->symtable_stack, operand_2->attribute);

    // For quicker printing of the source registers (Either R1/R2 or F1/F2)
    char op1_reg[8]; char op2_reg[8];


    // Same edge/error cases as with FloatExpression()
    if(operand_1->token_type == IDENTIFIER_TOKEN && operand_2->token_type == IDENTIFIER_TOKEN)
    {
        if(symb1->type != symb2->type)
        {
            *are_incompatible = true;
            return;
        }

        else if(symb1->type == DOUBLE64_TYPE || symb2->type == DOUBLE64_TYPE)
        {
            has_floats = true;
        }
    }

    else if(symb1 != NULL)
    {
        if(symb1->type == INT32_TYPE && operand_2->token_type == DOUBLE_64)
        {
            *are_incompatible = true;
            return;
        }

        else if(symb1->type == DOUBLE64_TYPE)
        {
            has_floats = true;
        }
    }

    else if(symb2 != NULL)
    {
        if(operand_1->token_type == DOUBLE_64 && symb2->type == INT32_TYPE)
        {
            *are_incompatible = true;
            return;
        }

        else if(symb2->type == DOUBLE64_TYPE)
        {
            has_floats = true;
        }
    }


    // Get the src registers
    if(has_floats)
    {
        strcpy(op1_reg, "GF@$F1");
        strcpy(op2_reg, "GF@$F2");
    }

    else
    {
        strcpy(op1_reg, "GF@$R1");
        strcpy(op2_reg, "GF@$R2");
    }

    // Convert to floats if needed
    if(operand_1->token_type == INTEGER_32 && has_floats) fprintf(stdout, "INT2FLOAT GF@$F1 GF@$R1\n");
    if(operand_2->token_type == INTEGER_32 && has_floats) fprintf(stdout, "INT2FLOAT GF@$F2 GF@$R2\n");

    // Perform the given operation
    switch(operator)
    {
        case EQUAL_OPERATOR:
            fprintf(stdout, "EQ GF@$B0 %s %s\n", op1_reg, op2_reg);
            break;

        case NOT_EQUAL_OPERATOR:
            fprintf(stdout, "EQ GF@$B1 %s %s\n", op1_reg, op2_reg);
            fprintf(stdout, "NOT GF@$B0 GF@$B1\n");
            break;

        case LARGER_THAN_OPERATOR:
            fprintf(stdout, "GT GF@$B0 %s %s\n", op2_reg, op1_reg);
            break;

        case LESS_THAN_OPERATOR:
            fprintf(stdout, "LT GF@$B0 %s %s\n", op2_reg, op1_reg);
            break;

        case LARGER_EQUAL_OPERATOR:
            fprintf(stdout, "GT GF@$B1 %s %s\n", op2_reg, op1_reg);
            fprintf(stdout, "EQ GF@$B2 %s %s\n", op1_reg, op2_reg);
            fprintf(stdout, "OR GF@$B0 GF@$B1 GF@$B2\n");
            break;

        case LESSER_EQUAL_OPERATOR:
            fprintf(stdout, "LT GF@$B1 %s %s\n", op2_reg, op1_reg);
            fprintf(stdout, "EQ GF@$B2 %s %s\n", op1_reg, op2_reg);
            fprintf(stdout, "OR GF@$B0 GF@$B1 GF@$B2\n");
            break;

        default:
            // This will never happen, because the codegen calls this only when a boolean operator is encountered
            // Regardless, it has to be here
            fprintf(stderr, "Error in codegen: Invalid boolean operator\n");
            break;
    }

    fprintf(stdout, "PUSHS GF@$B0\n");
}

DATA_TYPE GeneratePostfixExpression(Parser *parser, TokenVector *postfix, VariableSymbol *var)
{
    // either we use the original variable's name, or define a new one
    char *varname;

    // Variables to store symbols if needed (so if we encounter an id token for example)
    VariableSymbol *sym1 = NULL, *sym2 = NULL;

    // Variables to store tokens
    Token *op1 = NULL, *op2 = NULL;

    // Error flag if we will need to check
    bool are_incompatible = false;

    // Flags to indicate if the operands are floats (if they are id's)
    bool op1_float = false, op2_float = false;

    // To later return
    DATA_TYPE return_type = INT32_TYPE; // Default

    if(var == NULL) varname = strdup("tempvar");

    else // we were given a variable
    {
        if((varname = malloc((strlen(var->name) + 1) * sizeof(char))) == NULL)
        {
            SymtableStackDestroy(parser->symtable_stack);
            DestroyTokenVector(postfix);
            free(varname);
            ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
        }

        // copy the variable's name to varname
        strcpy(varname, var->name);
    }

    // create a stack to evaluate the expression
    ExpressionStack *stack = ExpressionStackInit();
    // go through the entire postfix string
    for(int i = 0; i < postfix->length; i++)
    {
        // evaluate it using the postfix evaluation algorithm
        Token *token = postfix->token_string[i];
        switch(token->token_type)
        {
            case INTEGER_32: case DOUBLE_64: // operand tokens, push them to the stack
                if(token->token_type == DOUBLE_64) return_type = DOUBLE64_TYPE;
                char *type_string = GetTypeStringToken(token->token_type);
                ExpressionStackPush(stack, token);
                PUSHS(token->attribute, token->token_type, LOCAL_FRAME);
                free(type_string);
                break;

            case IDENTIFIER_TOKEN: // identifier token, the same as for operands just check type/and if are defined
                if((sym1 = SymtableStackFindVariable(parser->symtable_stack, token->attribute)) == NULL)
                {
                    fprintf(stderr, RED"Error in semantic analysis: Line %d: Undefined variable '%s'\n"RESET, parser->line_number, token->attribute);
                    SymtableStackDestroy(parser->symtable_stack);
                    DestroyStackAndVector(postfix, stack);
                    free(varname);
                    exit(ERROR_SEMANTIC_UNDEFINED);
                }

                else
                {
                    if(sym1->type == DOUBLE64_TYPE) return_type = DOUBLE64_TYPE;
                    PUSHS(token->attribute, token->token_type, LOCAL_FRAME);
                    ExpressionStackPush(stack, token);
                    break;
                }

            // arithmetic operators
            case MULTIPLICATION_OPERATOR: case DIVISION_OPERATOR:
            case ADDITION_OPERATOR: case SUBSTRACTION_OPERATOR:
                // pop 2 operands from the stack
                if(((op1 = ExpressionStackPop(stack)) == NULL) || ((op2 = ExpressionStackPop(stack)) == NULL))
                {
                    // invalid expression
                    if(op1 != NULL && IsTokenInString(postfix, op1)) DestroyToken(op1);
                    if(op2 != NULL && IsTokenInString(postfix, op2)) DestroyToken(op2);

                    SymtableStackDestroy(parser->symtable_stack);
                    DestroyStackAndVector(postfix, stack);
                    free(varname);
                    ErrorExit(ERROR_SYNTACTIC, "Line %d: Invalid expression");
                }


                // check if they are identifiers
                if(op1->token_type == IDENTIFIER_TOKEN)
                {
                    if(!(sym1 = SymtableStackFindVariable(parser->symtable_stack, op1->attribute)))
                    {
                        fprintf(stderr, RED"Error in syntax analysis: Line %d: Undefined variable '%s'\n"RESET, parser->line_number, op1->attribute);
                        if(!IsTokenInString(postfix, op1)) DestroyToken(op1);
                        if(!IsTokenInString(postfix, op2)) DestroyToken(op2);

                        SymtableStackDestroy(parser->symtable_stack);
                        DestroyStackAndVector(postfix, stack);
                        exit(ERROR_SEMANTIC_UNDEFINED);
                    }

                    else if(sym1->type == DOUBLE64_TYPE || sym1->type == DOUBLE64_NULLABLE_TYPE)
                    {
                        return_type = DOUBLE64_TYPE;
                        op1_float = true;
                    }
                }

                if(op2->token_type == IDENTIFIER_TOKEN)
                {
                    if(!(sym2 = SymtableStackFindVariable(parser->symtable_stack, op2->attribute)))
                    {
                        fprintf(stderr, RED"Error in syntax analysis: Line %d: Undefined variable '%s'\n"RESET, parser->line_number, op2->attribute);
                        if(!IsTokenInString(postfix, op1)) DestroyToken(op1);
                        if(!IsTokenInString(postfix, op2)) DestroyToken(op2);

                        SymtableStackDestroy(parser->symtable_stack);
                        DestroyStackAndVector(postfix, stack);
                        free(varname);
                        exit(ERROR_SEMANTIC_UNDEFINED);
                    }

                    else if(sym2->type == DOUBLE64_TYPE || sym2->type == DOUBLE64_NULLABLE_TYPE)
                    {
                        return_type = DOUBLE64_TYPE;
                        op2_float = true;
                    }
                }

                (op1->token_type != DOUBLE_64 && !op1_float) ? fprintf(stdout, "POPS GF@$R1\n") : fprintf(stdout, "POPS GF@$F1\n"); // R1/F1 = Second operand (popped from the stack first)
                (op2->token_type != DOUBLE_64 && !op2_float) ? fprintf(stdout, "POPS GF@$R2\n") : fprintf(stdout, "POPS GF@$F2\n"); // R2/F2 = First operand

                // call a function depending on the data types
                if(op1->token_type == DOUBLE_64 || op2->token_type == DOUBLE_64)
                {
                    // Change the return type
                    return_type = DOUBLE64_TYPE;

                    // To push the result
                    Token *f_token = InitToken();
                    f_token->token_type = DOUBLE_64;
                    FloatExpression(parser, op1, op2, token->token_type, &are_incompatible);
                    ExpressionStackPush(stack, f_token);
                    return_type = DOUBLE64_TYPE;
                    if(are_incompatible)
                    {
                        fprintf(stderr, RED"Error in semantic analysis: Line %d: Incompatible operands '%s' and '%s'\n"RESET, parser->line_number, op1->attribute, op2->attribute);
                        if(!IsTokenInString(postfix, op1)) DestroyToken(op1);
                        if(!IsTokenInString(postfix, op2)) DestroyToken(op2);

                        SymtableStackDestroy(parser->symtable_stack);
                        DestroyStackAndVector(postfix, stack);
                        free(varname);
                        exit(ERROR_SEMANTIC_TYPE_COMPATIBILITY);
                    }
                }

                else if(sym1 != NULL)
                {
                    if(sym1->type == DOUBLE64_TYPE){
                        Token *f_token = InitToken();
                        f_token->token_type = DOUBLE_64;
                        FloatExpression(parser, op1, op2, token->token_type, &are_incompatible);
                        ExpressionStackPush(stack, f_token);
                        return_type = DOUBLE64_TYPE;
                        
                        if(are_incompatible)
                        {
                            fprintf(stderr, RED"Error in semantic analysis: Line %d: Incompatible operands '%s' and '%s'\n"RESET, parser->line_number, op1->attribute, op2->attribute);
                            if(!IsTokenInString(postfix, op1)) DestroyToken(op1);
                            if(!IsTokenInString(postfix, op2)) DestroyToken(op2);

                            SymtableStackDestroy(parser->symtable_stack);
                            DestroyStackAndVector(postfix, stack);
                            free(varname);
                            exit(ERROR_SEMANTIC_TYPE_COMPATIBILITY);
                        }
                    }

                    else
                    {
                        Token *i_token = InitToken();
                        i_token->token_type = INTEGER_32;
                        IntExpression(token->token_type);
                        ExpressionStackPush(stack, i_token);
                    }
                }

                else if(sym2 != NULL)
                {
                    if(sym2->type == DOUBLE64_TYPE){
                        Token *f_token = InitToken();
                        f_token->token_type = DOUBLE_64;
                        FloatExpression(parser, op1, op2, token->token_type, &are_incompatible);
                        ExpressionStackPush(stack, f_token);
                        return_type = DOUBLE64_TYPE;

                        if(are_incompatible)
                        {
                            fprintf(stderr, RED"Error in semantic analysis: Line %d: Incompatible operands '%s' and '%s'\n"RESET, parser->line_number, op1->attribute, op2->attribute);
                            if(!IsTokenInString(postfix, op1)) DestroyToken(op1);
                            if(!IsTokenInString(postfix, op2)) DestroyToken(op2);

                            SymtableStackDestroy(parser->symtable_stack);
                            DestroyStackAndVector(postfix, stack);
                            free(varname);
                            exit(ERROR_SEMANTIC_TYPE_COMPATIBILITY);
                        }
                    }

                    else
                    {
                        Token *i_token = InitToken();
                        i_token->token_type = INTEGER_32;
                        IntExpression(token->token_type);
                        ExpressionStackPush(stack, i_token);
                    }
                }

                else
                {
                    Token *i_token = InitToken();
                    i_token->token_type = INTEGER_32;
                    IntExpression(token->token_type);
                    ExpressionStackPush(stack, i_token);
                }

                // Dispose of the old operands
                if(!IsTokenInString(postfix, op1)) DestroyToken(op1);
                if(!IsTokenInString(postfix, op2)) DestroyToken(op2);

                // Reset the float flags
                op1_float = false; op2_float = false;
                break;

            // Boolean operators
            case EQUAL_OPERATOR:                case NOT_EQUAL_OPERATOR:
            case LESS_THAN_OPERATOR:            case LARGER_THAN_OPERATOR:
            case LESSER_EQUAL_OPERATOR:         case LARGER_EQUAL_OPERATOR:
                // Has to be the end of a expression
                if(postfix->token_string[i + 1]->token_type != SEMICOLON && postfix->token_string[i + 1]->token_type != R_ROUND_BRACKET)
                {
                    SymtableStackDestroy(parser->symtable_stack);
                    DestroyStackAndVector(postfix, stack);
                    free(varname);
                    ErrorExit(ERROR_SYNTACTIC, "Line %d: Invalid expression");
                }

                // Return a boolean expression
                return_type = BOOLEAN;

                // pop 2 operands from the stack
                op1 = ExpressionStackPop(stack);
                op2 = ExpressionStackPop(stack);

                // Invalid expression check
                if(op1 == NULL || op2 == NULL)
                {
                    if(op1 != NULL && IsTokenInString(postfix, op1)) DestroyToken(op1);
                    if(op2 != NULL && IsTokenInString(postfix, op2)) DestroyToken(op2);

                    SymtableStackDestroy(parser->symtable_stack);
                    DestroyStackAndVector(postfix, stack);
                    free(varname);
                    ErrorExit(ERROR_SYNTACTIC, "Line %d: Invalid expression");
                }

                // Undefined checks
                else if(op1->token_type == IDENTIFIER_TOKEN)
                {
                    if(!(sym1 = SymtableStackFindVariable(parser->symtable_stack, op1->attribute)))
                    {
                        fprintf(stderr, RED"Error in syntax analysis: Line %d: Undefined variable '%s'\n"RESET, parser->line_number, op1->attribute);
                        if(!IsTokenInString(postfix, op1)) DestroyToken(op1);
                        if(!IsTokenInString(postfix, op2)) DestroyToken(op2);

                        SymtableStackDestroy(parser->symtable_stack);
                        DestroyStackAndVector(postfix, stack);
                        free(varname);
                        exit(ERROR_SEMANTIC_UNDEFINED);
                    }

                    else if(sym1->type == DOUBLE64_TYPE) op1_float = true;
                }

                else if(op2->token_type == IDENTIFIER_TOKEN)
                {
                    if(!(sym2 = SymtableStackFindVariable(parser->symtable_stack, op2->attribute)))
                    {
                        fprintf(stderr, RED"Error in syntax analysis: Line %d: Undefined variable '%s'\n"RESET, parser->line_number, op2->attribute);
                        if(!IsTokenInString(postfix, op1)) DestroyToken(op1);
                        if(!IsTokenInString(postfix, op2)) DestroyToken(op2);

                        SymtableStackDestroy(parser->symtable_stack);
                        DestroyStackAndVector(postfix, stack);
                        free(varname);
                        exit(ERROR_SEMANTIC_UNDEFINED);
                    }

                    else if(sym2->type == DOUBLE64_TYPE) op2_float = true;
                }

                (op1->token_type != DOUBLE_64 && !op1_float) ? fprintf(stdout, "POPS GF@$R1\n") : fprintf(stdout, "POPS GF@$F1\n"); // R1/F1 = Second operand (popped from the stack first)
                (op2->token_type != DOUBLE_64 && !op2_float) ? fprintf(stdout, "POPS GF@$R2\n") : fprintf(stdout, "POPS GF@$F2\n"); // R2/F2 = First operand

                op1_float = false; op2_float = false;

                Token *b_token = InitToken();
                b_token->token_type = BOOLEAN_TOKEN;
                BoolExpression(parser, op1, op2, token->token_type, &are_incompatible);
                ExpressionStackPush(stack, b_token);

                // Dispose of the old operands
                if(!IsTokenInString(postfix, op1)) DestroyToken(op1);
                if(!IsTokenInString(postfix, op2)) DestroyToken(op2);

                break;

            case SEMICOLON: case R_ROUND_BRACKET:
                // The result of the expression is on top of the stack

                // If the result of a expression isn't in a variable, it' a boolean expression
                if(strcmp(varname, "tempvar"))
                    fprintf(stdout, "POPS LF@%s\n", varname);

                else PopToRegister(return_type);

                // Clear registers
                CLEARS

                // Free resources that are not needed anymore
                free(varname);
                DestroyStackAndVector(postfix, stack);
                return return_type;

            default:
                // This case should never happen, since the Expression parser checks for invalid symbols, but it has to be here anyway
                SymtableStackDestroy(parser->symtable_stack);
                DestroyStackAndVector(postfix, stack);
                free(varname);
                ErrorExit(ERROR_SYNTACTIC, "Line %d: Invalid symbol in expression");
                break;
        }
    }

    // The function will get here if the expression isn't ended by a semicolon (for example while(expression))
    // If the result of a expression isn't in a variable, it' a boolean expression or a return value from a function, so it has to stay on top of the stack
    if(strcmp(varname, "tempvar"))
        fprintf(stdout, "POPS LF@%s\n", varname);

    else PopToRegister(return_type);

    // Clear registers
    CLEARS

    // Free resources that are not needed anymore
    free(varname);
    DestroyStackAndVector(postfix, stack);
    return return_type;
}

void IfLabel(int count)
{
    fprintf(stdout, "LABEL $if%d\n", count);
}

void ElseLabel(int count)
{
    fprintf(stdout, "LABEL $else%d\n", count);
}

void EndIfLabel(int count)
{
    fprintf(stdout, "LABEL $endif%d\n", count);
}

void WhileLabel(int count)
{
    fprintf(stdout, "LABEL $while%d\n", count);
}

void EndWhileLabel(int count)
{
    fprintf(stdout, "LABEL $endwhile%d\n", count);
}

void PUSHS(const char *attribute, TOKEN_TYPE type, FRAME frame)
{
    // If the token is an identifier, push it from the correct frame
    if(type == IDENTIFIER_TOKEN)
    {
        char *frame_string = GetFrameString(frame);
        fprintf(stdout, "PUSHS %s%s\n", frame_string, attribute);
        free(frame_string);
        return;
    }

    char *type_string = GetTypeStringToken(type);
    fprintf(stdout, "PUSHS %s", type_string);

    // White space handling for string literals
    if(type == LITERAL_TOKEN) WriteStringLiteral(attribute);
    else fprintf(stdout, "%s", attribute);

    // Print the exponent in case of a float token
    type == DOUBLE_64 ? fprintf(stdout, "p+0\n") : fprintf(stdout, "\n");
    free(type_string);
}

void MOVE(const char *dst, const char *src, bool is_literal, FRAME dst_frame)
{
    char *frame_string = GetFrameString(dst_frame);
    fprintf(stdout, "MOVE %s%s ", frame_string, dst);
    if(is_literal) WriteStringLiteral(src);
    else fprintf(stdout, "%s\n", src);
}

void SETPARAM(int order, const char *value, TOKEN_TYPE type, FRAME frame)
{
    // Initial print of the target parameter variable
    fprintf(stdout, "MOVE TF@PARAM%d ", order);

    // Prefix is either GF@/LF@/TF@ or the type of the token (int@, float@0x, string@, bool@)
    char *prefix = type == IDENTIFIER_TOKEN ? GetFrameString(frame) : GetTypeStringToken(type);
    fprintf(stdout, "%s", prefix);

    // If the token is a string literal, call the WriteStringLiteral function to handle whitespaces accordingly
    if(type == LITERAL_TOKEN) WriteStringLiteral(value); // Why can't i use the ternary operator here :(((
    else fprintf(stdout, "%s", value);

    // If the token is a float, print the exponent, other than that just a newline
    type == DOUBLE_64 ? fprintf(stdout, "p+0\n") : fprintf(stdout, "\n");

    free(prefix);
}

char *GetFrameString(FRAME frame)
{
    switch(frame)
    {
        case GLOBAL_FRAME:
            return strdup("GF@");

        case LOCAL_FRAME:
            return strdup("LF@");

        case TEMPORARY_FRAME:
            return strdup("TF@");
    }

    return NULL; // Shut up GCC please
}

char *GetTypeStringToken(TOKEN_TYPE type)
{
    switch(type)
    {
        case INTEGER_32:
            return strdup("int@");

        case DOUBLE_64:
            return strdup("float@0x");

        case LITERAL_TOKEN:
            return strdup("string@");

        case BOOLEAN_TOKEN:
            return strdup("bool@");

        default:
            return NULL;
    }
}

char *GetTypeStringSymbol(DATA_TYPE type)
{
    switch(type)
    {
        case INT32_TYPE: case INT32_NULLABLE_TYPE:
            return strdup("int@");

        case DOUBLE64_TYPE: case DOUBLE64_NULLABLE_TYPE:
            return strdup("float@0x");

        case U8_ARRAY_TYPE: case U8_ARRAY_NULLABLE_TYPE:
            return strdup("string@");

        case BOOLEAN:
            return strdup("bool@");

        default:
            return NULL;
    }
}

// In case the variable has term_type, change it after
void READ(VariableSymbol *var, FRAME frame, DATA_TYPE read_type)
{
    char *type;
    switch(var->type)
    {
        case U8_ARRAY_TYPE: case U8_ARRAY_NULLABLE_TYPE:
            type = strdup("string");
            break;

        case INT32_TYPE: case INT32_NULLABLE_TYPE:
            type = strdup("int");
            break;

        case DOUBLE64_TYPE: case DOUBLE64_NULLABLE_TYPE:
            type = strdup("float");
            break;

        case BOOLEAN:
            type = strdup("bool");
            break;

        case VOID_TYPE:
            type = GetTypeStringSymbol(read_type);
            break;

        default:
            ErrorExit(ERROR_INTERNAL, "Calling read on a variable with wrong type. Fix your code!!!");
    }


    switch(frame)
    {
        case GLOBAL_FRAME:
            fprintf(stdout, "READ GF@%s %s\n", var->name, type);
            break;

        case LOCAL_FRAME:
            fprintf(stdout, "READ LF@%s %s\n", var->name, type);
            break;

        case TEMPORARY_FRAME:
            fprintf(stdout, "READ TF@%s %s\n", var->name, type);
            break;
    }

    free(type);
}

void WRITEINSTRUCTION(Token *token, FRAME frame)
{
    // Get the prefix and print it
    char *prefix = token->token_type == IDENTIFIER_TOKEN ? GetFrameString(frame) : GetTypeStringToken(token->token_type);
    fprintf(stdout, "WRITE %s", prefix);

    // Write the token attribute depenting on the type
    if(token->token_type == LITERAL_TOKEN) WriteStringLiteral(token->attribute);
    else fprintf(stdout, "%s", token->attribute);

    // If the token is a float, print the exponent, other than that just a newline
    token->token_type == DOUBLE_64 ? fprintf(stdout, "p+0\n") : fprintf(stdout, "\n");

    free(prefix);
}

void INT2FLOAT(VariableSymbol *dst, Token *value, FRAME dst_frame, FRAME src_frame)
{
    char *src_prefix = value->token_type == IDENTIFIER_TOKEN ? GetFrameString(src_frame) : strdup("int@");
    char *dst_prefix = GetFrameString(dst_frame);

    fprintf(stdout, "INT2FLOAT %s%s %s%s\n", dst_prefix, dst->name, src_prefix, value->attribute);

    free(src_prefix);
    free(dst_prefix);
}

void FLOAT2INT(VariableSymbol *dst, Token *value, FRAME dst_frame, FRAME src_frame)
{
    char *src_prefix = value->token_type == IDENTIFIER_TOKEN ? GetFrameString(src_frame) : strdup("float@0x");
    char *dst_prefix = GetFrameString(dst_frame);

    fprintf(stdout, "FLOAT2INT %s%s %s%s\n", dst_prefix, dst->name, src_prefix, value->attribute);

    free(src_prefix);
    free(dst_prefix);
}

void STRLEN(VariableSymbol *var, Token *src, FRAME dst_frame, FRAME src_frame)
{
    // Get the prefixes
    char *dst_prefix = GetFrameString(dst_frame);
    char *src_prefix = src->token_type == IDENTIFIER_TOKEN ? GetFrameString(src_frame) : strdup("string@");
    fprintf(stdout, "STRLEN %s%s %s%s\n", dst_prefix, var->name, src_prefix, src->attribute);

    // Free the prefixes
    free(dst_prefix);
    free(src_prefix);
}

void CONCAT(VariableSymbol *dst, Token *prefix, Token *postfix, FRAME dst_frame, FRAME prefix_frame, FRAME postfix_frame)
{
    // Get the prefixes (maybe choose different variable names)
    char *dst_prefix = GetFrameString(dst_frame);
    char *prefix_prefix = prefix->token_type == IDENTIFIER_TOKEN ? GetFrameString(prefix_frame) : strdup("string@");
    char *postfix_prefix = postfix->token_type == IDENTIFIER_TOKEN ? GetFrameString(postfix_frame) : strdup("string@");
    
    // Print the instruction
    fprintf(stdout, "CONCAT %s%s %s%s %s%s\n", dst_prefix, dst->name, prefix_prefix, prefix->attribute, postfix_prefix, postfix->attribute);
    
    // Free the prefixes
    free(dst_prefix);
    free(prefix_prefix);
    free(postfix_prefix);
}

void STRI2INT(VariableSymbol *var, Token *src, Token *position, FRAME dst_frame, FRAME src_frame, FRAME position_frame)
{
    // Get the prefixes first
    char *dst_prefix = GetFrameString(dst_frame);
    char *src_prefix = src->token_type == IDENTIFIER_TOKEN ? GetFrameString(src_frame) : strdup("string@");
    char *position_prefix = position->token_type == IDENTIFIER_TOKEN ? GetFrameString(position_frame) : strdup("int@");

    // We assume that the type-checking has already been done, so error 58 won't occur
    fprintf(stdout, "STRI2INT %s%s %s%s %s%s\n", dst_prefix, var->name, src_prefix, src->attribute, position_prefix, position->attribute);

    // Deallocate the prefixes
    free(dst_prefix);
    free(src_prefix);
    free(position_prefix);
}

void INT2CHAR(VariableSymbol *dst, Token *value, FRAME dst_frame, FRAME src_frame)
{
    // Get prefixes
    char *dst_prefix = GetFrameString(dst_frame);
    char *src_prefix = value->token_type == IDENTIFIER_TOKEN ? GetFrameString(src_frame) : strdup("int@");
    fprintf(stdout, "INT2CHAR %s%s %s%s\n", dst_prefix, dst->name, src_prefix, value->attribute);

    // Free the prefixes
    free(dst_prefix);
    free(src_prefix);
}

void STRCMP(VariableSymbol *var, Token *str1, Token *str2, FRAME dst_frame, FRAME str1_frame, FRAME str2_frame)
{
    // If this is being called, we assume that the needed type-checking has already been done so it won't be done here
    char *dst_frame_str = GetFrameString(dst_frame);
    (void) dst_frame_str;
    char *str1_prefix = str1->token_type == IDENTIFIER_TOKEN ? GetFrameString(str1_frame) : GetTypeStringToken(str1->token_type);
    char *str2_prefix = str2->token_type == IDENTIFIER_TOKEN ? GetFrameString(str2_frame) : GetTypeStringToken(str2->token_type);

    // Compare the strings with IFJcode24 instructions
    // B1 will store the strings s1 > s2, B2 will store s2 > s1, if neither of those is true, the strings are equal
    fprintf(stdout, "GT GF@$B1 %s", str1_prefix);
    if(str1->token_type == LITERAL_TOKEN) WriteStringLiteral(str1->attribute);
    else fprintf(stdout, "%s", str1->attribute);

    // second operand
    fprintf(stdout, " %s", str2_prefix);
    if(str2->token_type == LITERAL_TOKEN) WriteStringLiteral(str2->attribute);
    else fprintf(stdout, "%s", str2->attribute);

    fprintf(stdout, "\n");

    // Do the same for B2
    fprintf(stdout, "GT GF@$B2 %s", str2_prefix);
    if(str2->token_type == LITERAL_TOKEN) WriteStringLiteral(str2->attribute);
    else fprintf(stdout, "%s", str2->attribute);

    // second operand
    fprintf(stdout, " %s", str1_prefix);
    if(str1->token_type == LITERAL_TOKEN) WriteStringLiteral(str1->attribute);
    else fprintf(stdout, "%s", str1->attribute);

    fprintf(stdout, "\n");

    // Jump to the corresponding labels for each situations
    /*
    B1 = str1 > str2
    B2 = str2 > str1
    */

    JUMPIFEQ("FIRSTGREATER", "GF@$B1", "bool@true", strcmp_count)
    JUMPIFEQ("SECONDGREATER", "GF@$B2", "bool@true", strcmp_count)
    fprintf(stdout, "JUMP AREEQUAL%d\n", strcmp_count);

    // LABEL FIRSTGREATER
    fprintf(stdout, "LABEL FIRSTGREATER%d\n", strcmp_count);
    MOVE(var->name, "int@-1", false, dst_frame);
    fprintf(stdout, "JUMP ENDSTRCMP%d\n", strcmp_count);

    // LABEL SECONDGREATER
    fprintf(stdout, "LABEL SECONDGREATER%d\n", strcmp_count);
    MOVE(var->name, "int@1", false, dst_frame);
    fprintf(stdout, "JUMP ENDSTRCMP%d\n", strcmp_count);

    // LABEL AREEQUAL
    fprintf(stdout, "LABEL AREEQUAL%d\n", strcmp_count);
    MOVE(var->name, "int@0", false, dst_frame);

    // LABEL ENDSTRCMP
    fprintf(stdout, "LABEL ENDSTRCMP%d\n", strcmp_count);

    // Increment the counter at the end of the function
    strcmp_count++;
}

void STRING(VariableSymbol *var, Token *src, FRAME dst_frame, FRAME src_frame)
{
    char *dst_prefix = GetFrameString(dst_frame);
    char *src_prefix = src->token_type == IDENTIFIER_TOKEN ? GetFrameString(src_frame) : strdup("string@");
    fprintf(stdout, "MOVE %s%s %s", dst_prefix, var->name, src_prefix);
    if(src->token_type == LITERAL_TOKEN)
    {
        WriteStringLiteral(src->attribute);
        fprintf(stdout, "\n");
    }
    else fprintf(stdout, "%s\n", src->attribute);
    free(dst_prefix);
    free(src_prefix);
}

void ORD(VariableSymbol *var, Token *string, Token *position, FRAME dst_frame, FRAME string_frame, FRAME position_frame)
{
    // Get the prefixes first
    char *dst_prefix = GetFrameString(dst_frame);
    char *string_prefix = string->token_type == IDENTIFIER_TOKEN ? GetFrameString(string_frame) : strdup("string@");
    char *position_prefix = position->token_type == IDENTIFIER_TOKEN ? GetFrameString(position_frame) : strdup("int@");

    // We assume that the type-checking has already been done
    /*
        - We will use the STRLEN instruction to get the length of the string and store it in R0
        - Then we will either insert 0 into dst, or we will use STRI2INT to get the ASCII value of the character at the given position
        - STRLEN <dst> <src>
        - STRI2INT <dst> <str> <position>
    */

    // Don't call STRLEN since it R0 is not represented by a token
    fprintf(stdout, "STRLEN GF@$R0 %s%s\n", string_prefix, string->attribute);

    /* Pseudocode for how that might look like
        if R0 == 0 jump RETURN0ORD
        B0 = position > R0
        if B0 jump RETURN0ORD
        STRI2INT ...
        jump ENDORD
    */

    // Initial conditionals
    JUMPIFEQ("ORDRETURN0", "GF@$R0", "int@0", ord_count)
    fprintf(stdout, "GT GF@$B0 %s%s GF@R0\n", position_prefix, string->attribute);
    JUMPIFEQ("ORDRETURN0", "GF@$B0", "bool@true", ord_count)

    // Call STRI2INT and skip the 0 assignment
    STRI2INT(var, string, position, dst_frame, string_frame, position_frame);
    fprintf(stdout, "JUMP ENDORD%d\n", ord_count);

    fprintf(stdout, "LABEL ORDRETURN0%d\n", ord_count);
    MOVE(var->name, "int@0", false, dst_frame);
    fprintf(stdout, "LABEL ENDORD%d\n", ord_count);

    // Deallocate the resources
    free(dst_prefix);
    free(string_prefix);
    free(position_prefix);

    // Increment the ord counter
    ord_count++;
}

void WriteStringLiteral(const char *str)
{
    for(int i = 0; i < (int)strlen(str); i++)
    {
        switch(str[i])
        {
            case '\n':
                fprintf(stdout, "\\010");
                break;

            case '\t':
                fprintf(stdout, "\\009");
                break;

            case '\v':
                fprintf(stdout, "\\011");
                break;

            case '\b':
                fprintf(stdout, "\\008");
                break;

            case '\r':
                fprintf(stdout, "\\013");
                break;

            case '\f':
                fprintf(stdout, "\\012");
                break;

            case '\\':
                fprintf(stdout, "\\092");
                break;

            case '\'':
                fprintf(stdout, "\\039");
                break;

            case '\"':
                fprintf(stdout, "\\034");
                break;

            case ' ':
                fprintf(stdout, "\\032");
                break;

            default:
                fprintf(stdout, "%c", str[i]);
                break;
        }
    }
} // hello rudko was here

void PopToRegister(DATA_TYPE type)
{
    switch (type)
    {
        case INT32_TYPE:
            fprintf(stdout, "POPS GF@$R0\n");
            break;

        case DOUBLE64_TYPE:
            fprintf(stdout, "POPS GF@$F0\n");
            break;

        case BOOLEAN:
            fprintf(stdout, "POPS GF@$B0\n");
            break;

        // This will never happen
        default:
            break;  
    }
}

void PushRetValue(DATA_TYPE type)
{
    switch (type)
    {
        case INT32_TYPE: case INT32_NULLABLE_TYPE:
            fprintf(stdout, "PUSHS GF@$R0\n");
            break;

        case DOUBLE64_TYPE: case DOUBLE64_NULLABLE_TYPE:
            fprintf(stdout, "PUSHS GF@$F0\n");
            break;

        // Should never happen?
        case BOOLEAN: 
            fprintf(stdout, "PUSHS GF@$B0\n");
            break;

        // This will never happen
        default:
            break;  
    }
}