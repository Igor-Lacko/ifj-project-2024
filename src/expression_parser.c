#include <stdio.h>
#include <stdlib.h>

#include "expression_parser.h"
#include "stack.h"
#include "core_parser.h"
#include "symtable.h"
#include "vector.h"
#include "error.h"
#include "scanner.h"

PrecedenceTable InitPrecedenceTable(void)
{
    PrecedenceTable table;

    // highest priority operators
    table.PRIORITY_HIGHEST[0] = MULTIPLICATION_OPERATOR;
    table.PRIORITY_HIGHEST[1] = DIVISION_OPERATOR;

    // middle priority operators
    table.PRIORITY_MIDDLE[0] = ADDITION_OPERATOR;
    table.PRIORITY_MIDDLE[1] = SUBSTRACTION_OPERATOR;

    // lowest priority operators
    table.PRIORITY_LOWEST[0] = EQUAL_OPERATOR;
    table.PRIORITY_LOWEST[1] = NOT_EQUAL_OPERATOR;
    table.PRIORITY_LOWEST[2] = LESS_THAN_OPERATOR;
    table.PRIORITY_LOWEST[3] = LARGER_THAN_OPERATOR;
    table.PRIORITY_LOWEST[4] = LESSER_EQUAL_OPERATOR;
    table.PRIORITY_LOWEST[5] = LARGER_EQUAL_OPERATOR;

    return table;
}

int ComparePriority(TOKEN_TYPE operator_1, TOKEN_TYPE operator_2)
{
    OPERATOR_PRIORITY priority_1 = LOWEST, priority_2 = LOWEST;

    // first operator check
    if (operator_1 == MULTIPLICATION_OPERATOR || operator_1 == DIVISION_OPERATOR)
        priority_1 = HIGHEST;

    else if (operator_1 == ADDITION_OPERATOR || operator_1 == SUBSTRACTION_OPERATOR)
        priority_1 = MIDDLE;

    // by default it's lowest

    // second operator check
    if (operator_2 == MULTIPLICATION_OPERATOR || operator_2 == DIVISION_OPERATOR)
        priority_2 = HIGHEST;

    else if (operator_2 == ADDITION_OPERATOR || operator_2 == SUBSTRACTION_OPERATOR)
        priority_2 = MIDDLE;

    // compute the return type
    if (priority_1 < priority_2)
        return -1;
    else if (priority_1 > priority_2)
        return 1;
    return 0;
}

bool IsNullable(VariableSymbol *var)
{
    return  var->type == INT32_NULLABLE_TYPE        ||
            var->type == DOUBLE64_NULLABLE_TYPE     ||
            var->type == U8_ARRAY_NULLABLE_TYPE     ||
            var->type == NULL_DATA_TYPE; // This specific one shouldn't ever happen i guess?
}

TokenVector *InfixToPostfix(Parser *parser)
{
    int line_start = parser->line_number; // In case we encounter a newline, multi-line expressions are (probably not supported)
    int bracket_count = 0;                // In case the expression ends
    int priority_difference;              // Token priority difference

    // needed structures
    Token *token;
    TokenVector *postfix = InitTokenVector();
    ExpressionStack *stack = ExpressionStackInit();

    while (((token = CopyToken(GetNextToken(parser)))->token_type) != SEMICOLON)
    {
        if (parser->line_number > line_start)
        {
            DestroyToken(token);
            DestroyTokenVector(postfix);
            ExpressionStackDestroy(stack);
            ErrorExit(ERROR_SYNTACTIC, "Line %d: Expression not ended correctly", line_start);
        }

        switch (token->token_type)
        {
        // operand tokens
        case (INTEGER_32):
        case (IDENTIFIER_TOKEN):
        case (DOUBLE_64):
            AppendToken(postfix, token);
            break;

        // left bracket
        case (L_ROUND_BRACKET):
            ExpressionStackPush(stack, token);
            bracket_count++;
            break;

        // operator tokens
        case (MULTIPLICATION_OPERATOR):
        case (DIVISION_OPERATOR):
        case (ADDITION_OPERATOR):
        case (SUBSTRACTION_OPERATOR):
        case (EQUAL_OPERATOR):
        case (NOT_EQUAL_OPERATOR):
        case (LESS_THAN_OPERATOR):
        case (LESSER_EQUAL_OPERATOR):
        case (LARGER_THAN_OPERATOR):
        case (LARGER_EQUAL_OPERATOR):

            if (ExpressionStackIsEmpty(stack) ||
                ExpressionStackTop(stack)->token_type == L_ROUND_BRACKET ||
                (priority_difference = ComparePriority(token->token_type, ExpressionStackTop(stack)->token_type)) == 1)
            {
                ExpressionStackPush(stack, token);
                break;
            }

            else if (priority_difference == 0 || priority_difference == 1 || !ExpressionStackIsEmpty(stack))
            {
                // pop the operators with higher/equal priority from the stack to the end of the postfix string
                // while((priority_difference = ComparePriority(token -> token_type, ExpressionStackTop(stack) -> token_type)) <= 0 && !ExpressionStackIsEmpty(stack) && ExpressionStackTop(stack) -> token_type != L_ROUND_BRACKET){
                while (true)
                {
                    if (!ExpressionStackIsEmpty(stack))
                    {
                        if (ExpressionStackTop(stack)->token_type != L_ROUND_BRACKET && (priority_difference = ComparePriority(token->token_type, ExpressionStackTop(stack)->token_type)) <= 0)
                        {
                            Token *top = ExpressionStackPop(stack);
                            AppendToken(postfix, top);
                        }

                        else
                        {
                            ExpressionStackPush(stack, token);
                            break;
                        }
                    }

                    else
                    {
                        ExpressionStackPush(stack, token);
                        break;
                    }
                }
            }

            else
            { // invalid symbol sequence
                fprintf(stderr, RED "Error in syntax analysis: Line %d: Unexpected symbol '%s' in expression\n" RESET, parser->line_number, token->attribute);
                DestroyToken(token);
                DestroyStackAndVector(postfix, stack);
                exit(ERROR_SYNTACTIC);

                // TODO: Possible memory leak: Other structures such as parser aren't freed
                // TODO 2: Add attributes for all symbols
            }

            break;

        // right bracket
        case (R_ROUND_BRACKET):
            // expression over case
            if (--bracket_count < 0)
            {
                while (!ExpressionStackIsEmpty(stack))
                {
                    Token *top = ExpressionStackPop(stack);
                    AppendToken(postfix, top);
                }
                ExpressionStackDestroy(stack);
                AppendToken(postfix, token);
                return postfix;
            }

            // pop the characters from the stack until we encounter a left bracket
            Token *top = ExpressionStackPop(stack);
            while (top->token_type != L_ROUND_BRACKET)
            {
                // invalid end of expression
                if (ExpressionStackIsEmpty(stack))
                {
                    DestroyToken(token);
                    DestroyToken(top);
                    DestroyTokenVector(postfix);
                    ExpressionStackDestroy(stack);
                    ErrorExit(ERROR_SYNTACTIC, "Line %d: Expression not ended correctly", line_start);
                }

                // append the token to the vector
                AppendToken(postfix, top);
                top = ExpressionStackPop(stack);
            }

            DestroyToken(token);
            DestroyToken(top);
            break;

        case EOF_TOKEN:
            fprintf(stderr, RED "Error in syntax analysis: Line %d: Expression not ended correctly\n" RESET, parser->line_number);
            DestroyToken(token);
            DestroyStackAndVector(postfix, stack);
            exit(ERROR_SYNTACTIC);

        default:
            fprintf(stderr, RED "Error in syntax analysis: Line %d: Unexpected symbol '%s' in expression\n" RESET, parser->line_number, token->attribute);
            DestroyToken(token);
            DestroyStackAndVector(postfix, stack);
            exit(ERROR_SYNTACTIC);
        }
    }

    while (!ExpressionStackIsEmpty(stack))
    {
        Token *top = ExpressionStackPop(stack);
        AppendToken(postfix, top);
    }

    AppendToken(postfix, token);
    ExpressionStackDestroy(stack);
    return postfix;
}

bool IsTokenInString(TokenVector *postfix, Token *token)
{
    for (int i = 0; i < postfix->length; i++)
    {
        if (token == postfix->token_string[i])
            return true;
    }

    return false;
}

void DestroyStackAndVector(TokenVector *postfix, ExpressionStack *stack)
{
    while (stack->size != 0)
    {
        Token *top = ExpressionStackPop(stack);
        if (!IsTokenInString(postfix, top))
            DestroyToken(top);
    }

    DestroyTokenVector(postfix);
    ExpressionStackDestroy(stack);
}

void PrintPostfix(TokenVector *postfix)
{
    if(postfix == NULL)
    {
        fprintf(stderr, "Recieved NULL TokenVector, printing nothing\n");
        return;
    }

    for(int i = 0; i < postfix->length; i++)
    {
        fprintf(stderr, "%s", postfix->token_string[i]->attribute);
    }

    fprintf(stderr, "\n");
}