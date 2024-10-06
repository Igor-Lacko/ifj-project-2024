#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen.h"

void InitRegisters()
{
    // Result registers
    printf("DEFVAR <GF@R0>\n");
    printf("DEFVAR <GF@F0>\n");

    // Operand registers
    printf("DEFVAR <GF@R1>\n");
    printf("DEFVAR <GF@R2>\n");
    printf("DEFVAR <GF@F1>\n");
    printf("DEFVAR <GF@F2>\n");
}

void IntExpression(Parser *parser, TOKEN_TYPE operator)
{
    // Initialize the result register to zero
    // At the start, operand_1 is always in the register R1, and operand_2 is in the register R2
    switch(operator)
    {
        case MULTIPLICATION_OPERATOR:
            printf("MUL <GF@R0> <GF@R1> <GF@R2>");
            break;

        case DIVISION_OPERATOR:
            printf("DIV <GF@R0> <GF@R2> <GF@R1>"); // R1 is second since it was popped from the stack first
            break;

        case ADDITION_OPERATOR:
            printf("ADD <GF@R0> <GF@R1> <GF@R2>");
            break;

        case SUBSTRACTION_OPERATOR:
            printf("SUB <GF@R0> <GF@R2> <GF@R1>"); // The same, substraction is not an commutative operation
            break;
    }

    printf("PUSHS <GF@R0>");
}

DATA_TYPE GeneratePostfixExpression(Parser *parser, TokenVector *postfix, VariableSymbol *var)
{
    // either we use the original variable's name, or define a new one
    char *varname;
    if(var == NULL)
    {
        if((varname = malloc((strlen("tempvar") + 1) * sizeof(char))) == NULL)
        {
            SymtableStackDestroy(parser->symtable_stack);
            DestroyTokenVector(postfix);
            ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
        }

        // define a new variable
        printf("DEFVAR <LF@tempvar>\n");
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
            case I32: case F64: // operand tokens, push them to the stack
                ExpressionStackPush(stack, token);
                printf("PUSHS <%s>\n",token->attribute);
                break;

            case IDENTIFIER_TOKEN: // identifier token, the same as for operands just check type/and if are defined
                VariableSymbol *symb;
                if((symb = SymtableStackFindVariable(parser->symtable_stack, token->attribute)) == NULL)
                {
                    fprintf(RED,"Error in semantic analysis: Line %d: Undefined variable '%s'\n"RESET, parser->line_number, token->attribute);
                    SymtableStackDestroy(parser->symtable_stack);
                    DestroyStackAndVector(postfix, stack);
                }

                else
                {
                    ExpressionStackPush(stack, token);
                    printf("PUSHS <LF@%s>\n",token->attribute);
                    break;
                }

            // arithmetic operators
            case MULTIPLICATION_OPERATOR: case DIVISION_OPERATOR:
            case ADDITION_OPERATOR: case SUBSTRACTION_OPERATOR:
                Token *op1, *op2;
                VariableSymbol *sym1 = NULL, *sym2 = NULL;
                // pop 2 operands from the stack
                if(((op1 = ExpressionStackPop(stack)) == NULL) || ((op2 = ExpressionStackTop(stack)) == NULL))
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
                        if(op1 != NULL && !IsTokenInString(postfix, op1)) DestroyToken(op1);
                        if(op2 != NULL && !IsTokenInString(postfix, op2)) DestroyToken(op2);

                        SymtableStackDestroy(parser->symtable_stack);
                        DestroyStackAndVector(postfix, stack);
                        exit(ERROR_SEMANTIC_UNDEFINED);
                    }
                }

                else if(op2->token_type == IDENTIFIER_TOKEN)
                {
                    if(!(sym2 = SymtableStackFindVariable(parser->symtable_stack, op2->attribute)))
                    {
                        fprintf(stderr, RED"Error in syntax analysis: Line %d: Undefined variable '%s'\n"RESET, parser->line_number, op2->attribute);
                        if(op1 != NULL && !IsTokenInString(postfix, op1)) DestroyToken(op1);
                        if(op2 != NULL && !IsTokenInString(postfix, op2)) DestroyToken(op2);

                        SymtableStackDestroy(parser->symtable_stack);
                        DestroyStackAndVector(postfix, stack);
                        exit(ERROR_SEMANTIC_UNDEFINED);
                    }
                }

                printf("POPS <GF@R1>\n"); // R1 = First operand
                printf("POPS <GF@R2>\n"); // R2 = Second operand

                // call a function depending on the data types
                if(op1->token_type == DOUBLE64_TYPE || op2->token_type == INT32_TYPE)
                {
                    FloatExpression(parser, op1, op2, token->token_type);
                }

                else
                {
                    IntExpression(parser, token->token_type);
                }

                // Dispose of the old operands
                if(!IsTokenInString(postfix, op1)) DestroyToken(op1);
                if(!IsTokenInString(postfix, op2)) DestroyToken(op2);
        }
    }
}