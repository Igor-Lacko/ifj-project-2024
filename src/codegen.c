#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen.h"
#include "expression_parser.h"

void InitRegisters()
{
    // Result registers
    fprintf(stdout, "DEFVAR <GF@R0>\n");
    fprintf(stdout, "DEFVAR <GF@F0>\n");
    fprintf(stdout, "DEFVAR <GF@B0>\n"); // Special boolean register mostly for conditional jumps

    // Operand registers
    fprintf(stdout, "DEFVAR <GF@R1>\n");
    fprintf(stdout, "DEFVAR <GF@R2>\n");
    fprintf(stdout, "DEFVAR <GF@F1>\n");
    fprintf(stdout, "DEFVAR <GF@F2>\n");
}

void DefineVariable(const char *name, FRAME frame)
{
    switch(frame)
    {
        case GLOBAL_FRAME:
            fprintf(stdout, "DEFVAR <GF@%s>\n", name);
            break;

        case LOCAL_FRAME:
            fprintf(stdout, "DEFVAR <LF@%s>\n", name);
            break;

        case TEMPORARY_FRAME:
            fprintf(stdout, "DEFVAR <TF@%s>\n", name);
    }
}

void IntExpression(TOKEN_TYPE operator)
{
    // Initialize the result register to zero
    // At the start, operand_1 is always in the register R1, and operand_2 is in the register R2
    switch(operator)
    {
        case MULTIPLICATION_OPERATOR:
            fprintf(stdout, "MUL <GF@R0> <GF@R1> <GF@R2>\n");
            break;

        case DIVISION_OPERATOR:
            fprintf(stdout, "IDIV <GF@R0> <GF@R2> <GF@R1>\n");
            break;

        case ADDITION_OPERATOR:
            fprintf(stdout, "ADD <GF@R0> <GF@R1> <GF@R2>\n");
            break;

        case SUBSTRACTION_OPERATOR:
            fprintf(stdout, "SUB <GF@R0> <GF@R2> <GF@R1>\n");
            break;

        default:
            // This will never happen, but it also has to be here
            break;
    }

    fprintf(stdout, "PUSHS <GF@R0>\n");
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
    if(operand_1->token_type == INTEGER_32) fprintf(stdout, "INT2FLOAT <GF@F1> <GF@R1>\n");
    if(operand_2->token_type == INTEGER_32) fprintf(stdout, "INT2FLOAT <GF@F2> <GF@R2>\n");

    // Perform the given operation
    // At the start, operand 1 is in F1, operand 2 is in F2
    switch(operator)
    {
        case MULTIPLICATION_OPERATOR:
            fprintf(stdout, "MUL <GF@F0> <GF@F1> <GF@F2>\n");
            break;

        case DIVISION_OPERATOR:
            fprintf(stdout, "DIV <GF@F0> <GF@F2> <GF@F1>\n");
            break;

        case ADDITION_OPERATOR:
            fprintf(stdout, "ADD <GF@F0> <GF@F1> <GF@F2>\n");
            break;

        case SUBSTRACTION_OPERATOR:
            fprintf(stdout, "SUB <GF@F0> <GF@F2> <GF@F1>\n");
            break;

        default:
            // This will never happen, but it also has to be here
            break;
    }

    fprintf(stdout, "PUSHS <GF@F0>\n");
}

void BoolExpression(Parser *parser, Token *operand_1, Token *operand_2, TOKEN_TYPE operator, bool *are_incompatible)
{
    //todo
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

    if(var == NULL)
    {
        if((varname = malloc((strlen("tempvar") + 1) * sizeof(char))) == NULL)
        {
            SymtableStackDestroy(parser->symtable_stack);
            DestroyTokenVector(postfix);
            ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
        }

        // define a new variable
        fprintf(stdout, "DEFVAR <LF@tempvar>\n");
        strcpy(varname, "tempvar");
    }

    else // we were given a variable
    {
        if((varname = malloc((strlen(var->name) + 1) * sizeof(char))) == NULL)
        {
            SymtableStackDestroy(parser->symtable_stack);
            DestroyTokenVector(postfix);
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
                ExpressionStackPush(stack, token);
                fprintf(stdout, "PUSHS <%s>\n",token->attribute);
                break;

            case IDENTIFIER_TOKEN: // identifier token, the same as for operands just check type/and if are defined
                if((sym1 = SymtableStackFindVariable(parser->symtable_stack, token->attribute)) == NULL)
                {
                    fprintf(stdout, RED"Error in semantic analysis: Line %d: Undefined variable '%s'\n"RESET, parser->line_number, token->attribute);
                    SymtableStackDestroy(parser->symtable_stack);
                    DestroyStackAndVector(postfix, stack);
                    exit(ERROR_SEMANTIC_UNDEFINED);
                }

                else
                {
                    ExpressionStackPush(stack, token);
                    fprintf(stdout, "PUSHS <LF@%s>\n",token->attribute);
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

                    else if(sym1->type == INT32_TYPE) op1_float = true;
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
                        exit(ERROR_SEMANTIC_UNDEFINED);
                    }

                    else if(sym2->type == DOUBLE64_TYPE) op2_float = true;
                }

                (op1->token_type != DOUBLE_64 || op1_float) ? fprintf(stdout, "POPS <GF@R1>\n") : fprintf(stdout, "POPS <GF@F1>\n"); // R1/F1 = Second operand (popped from the stack first)
                (op2->token_type != DOUBLE_64 || op2_float) ? fprintf(stdout, "POPS <GF@R2>\n") : fprintf(stdout, "POPS <GF@F2>\n"); // R2/F2 = First operand

                // call a function depending on the data types
                if(op1->token_type == DOUBLE_64 || op2->token_type == DOUBLE_64)
                {
                    // To push the result
                    Token *f_token = InitToken();
                    f_token->token_type = DOUBLE_64;

                    FloatExpression(parser, op1, op2, token->token_type, &are_incompatible);
                    AllocateAttribute(f_token, "testtest");
                    ExpressionStackPush(stack, f_token);
                    return_type = DOUBLE64_TYPE;
                    if(are_incompatible)
                    {
                        fprintf(stderr, RED"Error in semantic analysis: Line %d: Incompatible operands '%s' and '%s'\n"RESET, parser->line_number, op1->attribute, op2->attribute);
                        if(!IsTokenInString(postfix, op1)) DestroyToken(op1);
                        if(!IsTokenInString(postfix, op2)) DestroyToken(op2);

                        SymtableStackDestroy(parser->symtable_stack);
                        DestroyStackAndVector(postfix, stack);
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
                        exit(ERROR_SEMANTIC_TYPE_COMPATIBILITY);
                        }
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
                        exit(ERROR_SEMANTIC_TYPE_COMPATIBILITY);
                        }
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
            case EQUAL_OPERATOR: case NOT_EQUAL_OPERATOR:
            case LESS_THAN_OPERATOR: case LARGER_THAN_OPERATOR:
            case LESSER_EQUAL_OPERATOR: case LARGER_EQUAL_OPERATOR:
                // Has to be the end of a expression
                if(postfix->token_string[++i]->token_type != SEMICOLON)
                {
                    free(varname);
                    SymtableStackDestroy(parser->symtable_stack);
                    DestroyStackAndVector(postfix, stack);
                    ErrorExit(ERROR_SYNTACTIC, "Line %d: Invalid expression");
                }

                // pop 2 operands from the stack
                op1 = ExpressionStackPop(stack);
                op2 = ExpressionStackPop(stack);

                // Invalid expression check
                if(op1 == NULL || op2 == NULL)
                {
                    if(op1 != NULL && IsTokenInString(postfix, op1)) DestroyToken(op1);
                    if(op2 != NULL && IsTokenInString(postfix, op2)) DestroyToken(op2);

                    free(varname);
                    SymtableStackDestroy(parser->symtable_stack);
                    DestroyStackAndVector(postfix, stack);
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

                        free(varname);
                        SymtableStackDestroy(parser->symtable_stack);
                        DestroyStackAndVector(postfix, stack);
                        exit(ERROR_SEMANTIC_UNDEFINED);
                    }

                    else if(sym1->type == DOUBLE_64) op1_float = true;
                }

                else if(op2->token_type == IDENTIFIER_TOKEN)
                {
                    if(!(sym2 = SymtableStackFindVariable(parser->symtable_stack, op2->attribute)))
                    {
                        fprintf(stderr, RED"Error in syntax analysis: Line %d: Undefined variable '%s'\n"RESET, parser->line_number, op2->attribute);
                        if(!IsTokenInString(postfix, op1)) DestroyToken(op1);
                        if(!IsTokenInString(postfix, op2)) DestroyToken(op2);

                        free(varname);
                        SymtableStackDestroy(parser->symtable_stack);
                        DestroyStackAndVector(postfix, stack);
                        exit(ERROR_SEMANTIC_UNDEFINED);
                    }

                    else if(sym2->type == DOUBLE64_TYPE) op2_float = true;
                }

                (op1->token_type != DOUBLE_64 || op1_float) ? fprintf(stdout, "POPS <GF@R1>\n") : fprintf(stdout, "POPS <GF@F1>\n"); // R1/F1 = Second operand (popped from the stack first)
                (op2->token_type != DOUBLE_64 || op2_float) ? fprintf(stdout, "POPS <GF@R2>\n") : fprintf(stdout, "POPS <GF@F2>\n"); // R2/F2 = First operand

                op1_float = false; op2_float = false;

                Token *b_token = InitToken();
                b_token->token_type = BOOLEAN_TOKEN;
                BoolExpression(parser, op1, op2, token, &are_incompatible);
                return BOOLEAN; // todo 

            case SEMICOLON:
                // The result of the expression is on top of the stack
                fprintf(stdout, "POPS <LF@%s>\n", varname);

                // Clear the registers
                fprintf(stdout, "CLEARS\n");

                // Free resources that are not needed anymore
                free(varname);
                DestroyStackAndVector(postfix, stack);
                return return_type;

            default:
                // This case should never happen, since the Expression parser checks for invalid symbols, but it has to be here anyway
                PrintToken(token);
                SymtableStackDestroy(parser->symtable_stack);
                DestroyStackAndVector(postfix, stack);
                ErrorExit(ERROR_SYNTACTIC, "Line %d: Invalid symbol in expression");
                break;
        }
    }

    // The function shouldn't ever get here, but this has to be here anyway
    return return_type;
}