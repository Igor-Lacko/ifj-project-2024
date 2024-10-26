#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen.h"
#include "expression_parser.h"

// Extern label counters
int if_label_count = 0;
int while_label_count = 0;

void InitRegisters()
{
    // Result registers
    fprintf(stdout, "DEFVAR GF@R0\n");
    fprintf(stdout, "DEFVAR GF@F0\n");
    fprintf(stdout, "DEFVAR GF@B0\n");
    fprintf(stdout, "DEFVAR GF@S0\n");

    // Operand registers
    fprintf(stdout, "DEFVAR GF@R1\n");
    fprintf(stdout, "DEFVAR GF@R2\n");
    fprintf(stdout, "DEFVAR GF@F1\n");
    fprintf(stdout, "DEFVAR GF@F2\n");
    fprintf(stdout, "DEFVAR GF@B1\n");
    fprintf(stdout, "DEFVAR GF@B2\n");
    fprintf(stdout, "DEFVAR GF@S1\n");
    fprintf(stdout, "DEFVAR GF@S2\n");
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
            fprintf(stdout, "MUL GF@R0 GF@R1 GF@R2\n");
            break;

        case DIVISION_OPERATOR:
            fprintf(stdout, "IDIV GF@R0 GF@R2 GF@R1\n");
            break;

        case ADDITION_OPERATOR:
            fprintf(stdout, "ADD GF@R0 GF@R1 GF@R2\n");
            break;

        case SUBSTRACTION_OPERATOR:
            fprintf(stdout, "SUB GF@R0 GF@R2 GF@R1\n");
            break;

        default:
            // This will never happen, but it also has to be here
            break;
    }

    fprintf(stdout, "PUSHS GF@R0\n");
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
    if(operand_1->token_type == INTEGER_32) fprintf(stdout, "INT2FLOAT GF@F1 GF@R1\n");
    if(operand_2->token_type == INTEGER_32) fprintf(stdout, "INT2FLOAT GF@F2 GF@R2\n");

    // Perform the given operation
    // At the start, operand 1 is in F1, operand 2 is in F2
    switch(operator)
    {
        case MULTIPLICATION_OPERATOR:
            fprintf(stdout, "MUL GF@F0 GF@F1 GF@F2\n");
            break;

        case DIVISION_OPERATOR:
            fprintf(stdout, "DIV GF@F0 GF@F2 GF@F1\n");
            break;

        case ADDITION_OPERATOR:
            fprintf(stdout, "ADD GF@F0 GF@F1 GF@F2\n");
            break;

        case SUBSTRACTION_OPERATOR:
            fprintf(stdout, "SUB GF@F0 GF@F2 GF@F1\n");
            break;

        default:
            // This will never happen, but it also has to be here
            break;
    }

    fprintf(stdout, "PUSHS GF@F0\n");
}

void BoolExpression(Parser *parser, Token *operand_1, Token *operand_2, TOKEN_TYPE operator, bool *are_incompatible)
{
    // So that we knpow which registers to use
    bool has_floats = false;

    // For quicker access to types
    VariableSymbol *symb1 = NULL, *symb2 = NULL;

    // For quicker printing of the source registers (Either R1/R2 or F1/F2)
    char op1_reg[8]; char op2_reg[8];

    // Same edge/error cases as with FloatExpression()
    if(operand_1->token_type == IDENTIFIER_TOKEN && operand_2->token_type == IDENTIFIER_TOKEN)
    {
        if((symb1 = SymtableStackFindVariable(parser->symtable_stack, operand_1->attribute))->type != (symb2 = SymtableStackFindVariable(parser->symtable_stack, operand_2->attribute))->type)
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
    }

    else if(symb2 != NULL)
    {
        if(operand_1->token_type == INTEGER_32 && symb2->type == INT32_TYPE)
        {
            *are_incompatible = true;
            return;
        }
    }

    // Get the src registers
    if(has_floats)
    {
        strcpy(op1_reg, "GF@F1");
        strcpy(op2_reg, "GF@F2");
    }

    else
    {
        strcpy(op1_reg, "GF@R1");
        strcpy(op2_reg, "GF@R2");
    }

    // Convert to floats if needed
    if(operand_1->token_type == INTEGER_32 && has_floats) fprintf(stdout, "INT2FLOAT GF@F1 GF@R1\n");
    if(operand_2->token_type == INTEGER_32 && has_floats) fprintf(stdout, "INT2FLOAT GF@F2 GF@R2\n");

    // Perform the given operation
    switch(operator)
    {
        case EQUAL_OPERATOR:
            fprintf(stdout, "EQ GF@B0 %s %s\n", op1_reg, op2_reg);
            break;

        case NOT_EQUAL_OPERATOR:
            fprintf(stdout, "EQ GF@B1 %s %s\n", op1_reg, op2_reg);
            fprintf(stdout, "NOT GF@B0 GF@B1\n");
            break;

        case LARGER_THAN_OPERATOR:
            fprintf(stdout, "GT GF@B0 %s %s\n", op2_reg, op1_reg);
            break;

        case LESS_THAN_OPERATOR:
            fprintf(stdout, "LT GF@B0 %s %s\n", op2_reg, op1_reg);
            break;

        case LARGER_EQUAL_OPERATOR:
            fprintf(stdout, "GT GF@B1 %s %s\n", op2_reg, op1_reg);
            fprintf(stdout, "EQ GF@B2 %s %s\n", op1_reg, op2_reg);
            fprintf(stdout, "OR GF@B0 GF@B1 GF@B2\n");
            break;

        case LESSER_EQUAL_OPERATOR:
            fprintf(stdout, "LT GF@B1 %s %s\n", op2_reg, op1_reg);
            fprintf(stdout, "EQ GF@B2 %s %s\n", op1_reg, op2_reg);
            fprintf(stdout, "OR GF@B0 GF@B1 GF@B2\n");
            break;

        default:
            // This will never happen, because the codegen calls this only when a boolean operator is encountered
            // Regardless, it has to be here
            break;
    }

    fprintf(stdout, "PUSHS GF@B0\n");
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
        fprintf(stdout, "DEFVAR LF@tempvar\n");
        strcpy(varname, "tempvar");
    }

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
                    ExpressionStackPush(stack, token);
                    fprintf(stdout, "PUSHS LF@%s\n",token->attribute);
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

                    else if(sym2->type == DOUBLE64_TYPE || sym2->type == DOUBLE64_NULLABLE_TYPE)
                    {
                        return_type = DOUBLE64_TYPE;
                        op2_float = true;
                    }
                }

                (op1->token_type != DOUBLE_64 || op1_float) ? fprintf(stdout, "POPS GF@R1\n") : fprintf(stdout, "POPS GF@F1\n"); // R1/F1 = Second operand (popped from the stack first)
                (op2->token_type != DOUBLE_64 || op2_float) ? fprintf(stdout, "POPS GF@R2\n") : fprintf(stdout, "POPS GF@F2\n"); // R2/F2 = First operand

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
                if(postfix->token_string[i + 1]->token_type != SEMICOLON)
                {
                    SymtableStackDestroy(parser->symtable_stack);
                    DestroyStackAndVector(postfix, stack);
                    free(varname);
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

                if(op1_float || op2_float) return_type = DOUBLE64_TYPE;

                (op1->token_type != DOUBLE_64 || op1_float) ? fprintf(stdout, "POPS GF@R1\n") : fprintf(stdout, "POPS GF@F1\n"); // R1/F1 = Second operand (popped from the stack first)
                (op2->token_type != DOUBLE_64 || op2_float) ? fprintf(stdout, "POPS GF@R2\n") : fprintf(stdout, "POPS GF@F2\n"); // R2/F2 = First operand

                op1_float = false; op2_float = false;

                Token *b_token = InitToken();
                b_token->token_type = BOOLEAN_TOKEN;
                BoolExpression(parser, op1, op2, token->token_type, &are_incompatible);
                ExpressionStackPush(stack, b_token);

                // Dispose of the old operands
                if(!IsTokenInString(postfix, op1)) DestroyToken(op1);
                if(!IsTokenInString(postfix, op2)) DestroyToken(op2);

                break;

            case SEMICOLON:
                // The result of the expression is on top of the stack
                fprintf(stdout, "POPS LF@%s\n", varname);

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
                free(varname);
                ErrorExit(ERROR_SYNTACTIC, "Line %d: Invalid symbol in expression");
                break;
        }
    }

    // The function shouldn't ever get here, but this has to be here anyway
    return return_type;
}

void IfLabel(FRAME frame)
{
    switch (frame)
    {
    case GLOBAL_FRAME:
        fprintf(stdout, "LABEL GF@if%d\n", ++if_label_count);
        break;
    
    case LOCAL_FRAME:
        fprintf(stdout, "LABEL LF@if%d\n", ++if_label_count);
        break;

    case TEMPORARY_FRAME:
        fprintf(stdout, "LABEL TF@if%d\n", ++if_label_count);
        break;
    }
}

void ElseLabel(FRAME frame)
{
    switch(frame)
    {
        case GLOBAL_FRAME:
            fprintf(stdout, "LABEL GF@else%d\n", if_label_count);
            break;

        case LOCAL_FRAME:
            fprintf(stdout, "LABEL LF@else%d\n", if_label_count);
            break;

        case TEMPORARY_FRAME:
            fprintf(stdout, "LABEL TF@else%d\n", if_label_count);
            break;
    }
}

void EndIfLabel(FRAME frame)
{
    switch(frame)
    {
        case GLOBAL_FRAME:
            fprintf(stdout, "LABEL GF@endif%d\n", if_label_count);
            break;

        case LOCAL_FRAME:
            fprintf(stdout, "LABEL LF@endif%d\n", if_label_count);
            break;

        case TEMPORARY_FRAME:
            fprintf(stdout, "LABEL TF@endif%d\n", if_label_count);
            break;
    }
}

void WhileLabel(FRAME frame)
{
    switch(frame)
    {
        case GLOBAL_FRAME:
            fprintf(stdout, "LABEL GF@while%d\n", ++while_label_count);
            break;

        case LOCAL_FRAME:
            fprintf(stdout, "LABEL LF@while%d\n", ++while_label_count);
            break;

        case TEMPORARY_FRAME:
            fprintf(stdout, "LABEL TF@while%d\n", ++while_label_count);
            break;
    }
}

void EndWhileLabel(FRAME frame)
{
    switch(frame)
    {
        case GLOBAL_FRAME:
            fprintf(stdout, "LABEL GF@endwhile%d\n", while_label_count);
            break;

        case LOCAL_FRAME:
            fprintf(stdout, "LABEL LF@endwhile%d\n", while_label_count);
            break;

        case TEMPORARY_FRAME:
            fprintf(stdout, "LABEL TF@endwhile%d\n", while_label_count);
            break;
    }
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

void MOVE(const char *dst, const char *src, FRAME dst_frame)
{
    char *frame_str = GetFrameString(dst_frame);
    fprintf(stdout, "MOVE %s%s %s\n", frame_str, dst, src);
    free(frame_str);
}

void SETPARAM(int order, const char *value, TOKEN_TYPE type, FRAME frame)
{
    // Initial print of the target parameter variable
    fprintf(stdout, "MOVE TF@param%d ", order);

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
void READ(VariableSymbol *var, FRAME frame)
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


void INT2FLOAT(VariableSymbol *dst, const char *value, FRAME frame)
{
    switch(frame)
    {
        case GLOBAL_FRAME:
            fprintf(stdout, "INT2FLOAT GF@%s %s", dst->name, value);
            break;

        case LOCAL_FRAME:
            fprintf(stdout, "INT2FLOAT LF@%s %s", dst->name, value);
            break;

        case TEMPORARY_FRAME:
            fprintf(stdout, "INT2FLOAT TF@%s %s", dst->name, value);
            break;
    }
}

void FLOAT2INT(VariableSymbol *dst, const char *value, FRAME frame)
{
    switch(frame)
    {
        case GLOBAL_FRAME:
            fprintf(stdout, "FLOAT2INT GF@%s %s", dst->name, value);
            break;

        case LOCAL_FRAME:
            fprintf(stdout, "FLOAT2INT LF@%s %s", dst->name, value);
            break;

        case TEMPORARY_FRAME:
            fprintf(stdout, "FLOAT2INT TF@%s %s", dst->name, value);
            break;
    }
}

void STRLEN(VariableSymbol *dst, const char *str, FRAME frame)
{
    switch(frame)
    {
        case GLOBAL_FRAME:
            fprintf(stdout, "STRLEN GF@%s %s\n", dst->name, str);
            break;

        case LOCAL_FRAME:
            fprintf(stdout, "STRLEN LF@%s %s\n", dst->name, str);
            break;

        case TEMPORARY_FRAME:
            fprintf(stdout, "STRLEN TF@%s %s\n", dst->name, str);
            break;
    }
}

void CONCAT(VariableSymbol *dst, const char *str1, const char *str2, FRAME frame)
{
    switch(frame)
    {
        case GLOBAL_FRAME:
            fprintf(stdout, "CONCAT GF@%s %s %s\n", dst->name, str1, str2);
            break;

        case LOCAL_FRAME:
            fprintf(stdout, "CONCAT LF@%s %s %s\n", dst->name, str1, str2);
            break;

        case TEMPORARY_FRAME:
            fprintf(stdout, "CONCAT TF@%s %s %s\n", dst->name, str1, str2);
            break;
    }
}

void STRI2INT(VariableSymbol *dst, const char *src, int position, FRAME frame)
{
    char *frame_str = GetFrameString(frame);
    fprintf(stdout, "STRI2INT %s%s %s %d", frame_str, dst->name, src, position);
    free(frame_str);
}

void INT2CHAR(VariableSymbol *dst, int ascii_value, FRAME frame)
{
    char *frame_str = GetFrameString(frame);
    fprintf(stdout, "INT2CHAR %s%s %d\n", frame_str, dst->name, ascii_value);
    free(frame_str);
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
}