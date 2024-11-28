#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "expression_parser.h"
#include "stack.h"
#include "core_parser.h"
#include "symtable.h"
#include "vector.h"
#include "error.h"
#include "scanner.h"
#include "codegen.h"
#include "shared.h"

const Ptable precedence_table = {
    /*ID*/  {INVALID, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, INVALID, REDUCE, REDUCE},
    /***/   {SHIFT, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, SHIFT, REDUCE, REDUCE},
    /*/*/   {SHIFT, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, SHIFT, REDUCE, REDUCE},
    /*+*/   {SHIFT, SHIFT, SHIFT, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, SHIFT, REDUCE, REDUCE},
    /*-*/   {SHIFT, SHIFT, SHIFT, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, SHIFT, REDUCE, REDUCE},
    /*==*/  {SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, SHIFT, REDUCE, REDUCE},
    /*!=*/  {SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, SHIFT, REDUCE, REDUCE},
    /*<*/   {SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, SHIFT, REDUCE, REDUCE},
    /*>*/   {SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, SHIFT, REDUCE, REDUCE},
    /*<=*/  {SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, SHIFT, REDUCE, REDUCE},
    /*>=*/  {SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, SHIFT, REDUCE, REDUCE},
    /*(*/   {SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, MATCH, INVALID},
    /*)*/   {INVALID, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, INVALID, REDUCE, REDUCE},
    /*$*/   {SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, INVALID, ACCEPT}
    };


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

bool IsNullable(DATA_TYPE type)
{
    return type == INT32_NULLABLE_TYPE ||
           type == DOUBLE64_NULLABLE_TYPE ||
           type == U8_ARRAY_NULLABLE_TYPE ||
           type == NULL_DATA_TYPE; // This specific one shouldn't ever happen i guess?
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
            if (token->token_type == KEYWORD && token->keyword_type == NULL_TYPE)
                AppendToken(postfix, token); // TODO semantic
            else
            {
                fprintf(stderr, RED "Error in syntax analysis: Line %d: Unexpected symbol '%s' in expression\n" RESET, parser->line_number, token->attribute);
                DestroyToken(token);
                DestroyStackAndVector(postfix, stack);
                exit(ERROR_SYNTACTIC);
            }
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
    if (postfix == NULL)
    {
        fprintf(stderr, "Recieved NULL TokenVector, printing nothing\n");
        return;
    }

    for (int i = 0; i < postfix->length; i++)
    {
        fprintf(stderr, "%s", postfix->token_string[i]->attribute);
    }

    fprintf(stderr, "\n");
}

void ReplaceConstants(TokenVector *postfix, Parser *parser)
{
    for (int i = 0; i < postfix->length; i++)
    {
        Token *token = postfix->token_string[i];
        if (token->token_type == IDENTIFIER_TOKEN)
        {
            VariableSymbol *var = SymtableStackFindVariable(parser->symtable_stack, token->attribute);
            if (var != NULL && !var->nullable && var->is_const && var->value != NULL)
            {
                var->was_used = true;
                Token *new = InitToken();
                new->attribute = strdup(var->value);
                new->line_number = token->line_number;
                switch (var->type)
                {
                case INT32_TYPE:
                    new->token_type = INTEGER_32;
                    break;

                case DOUBLE64_TYPE:
                    new->token_type = DOUBLE_64;
                    break;

                // Will never happen
                default:
                    break;
                }

                // Replace the old token
                postfix->token_string[i] = new;
                DestroyToken(token);
            }
        }
    }
}

bool HasZeroDecimalPlaces(char *float_value)
{
    double val = strtod(float_value, NULL);
    int int_part = (int)val;

    if (val - int_part < 0.0)
        return (val - int_part) > (0.0 - EPSILON);
    return (val - int_part) < EPSILON;
}

DATA_TYPE ArithmeticOperationTwoLiterals(Token *operand_left, Token *operand_right, Token *operator)
{
    // Get the resulting type of the operation
    DATA_TYPE result_type = operand_left->token_type == DOUBLE_64 ||
                                    operand_right->token_type == DOUBLE_64
                                ? DOUBLE64_TYPE
                                : INT32_TYPE;

    // If one operand is a double and the other is a float, we need to convert the int

    // In this case, the right operand is on top of the stack so stack conversion will suffice
    if (operand_right->token_type == INTEGER_32 && operand_left->token_type == DOUBLE_64)
        INT2FLOATS

    // In this case, the left operand is not on top of the stack so we need a temporary register for conversion
    else if (operand_left->token_type == INTEGER_32 && operand_right->token_type == DOUBLE_64)
    {
        // Since the right operand is an int, we can temporarily store it in R0
        PopToRegister(DOUBLE64_TYPE);

        // The second operand is now on top of the stack
        INT2FLOATS

        // Push the first operand back from F0
        fprintf(stdout, "PUSHS GF@$F0\n");
    }

    // Perform the operation based on the operator
    switch (operator->token_type)
    {
    case MULTIPLICATION_OPERATOR:
        MULS break;

    case DIVISION_OPERATOR:
        if (result_type == DOUBLE64_TYPE)
            DIVS else IDIVS break;

    case ADDITION_OPERATOR:
        ADDS break;

    case SUBSTRACTION_OPERATOR:
        SUBS break;

    // Will literally never ever happen
    default:
        ErrorExit(ERROR_INTERNAL, "Invalid operator in OperationTwoLiterals");
    }

    return result_type;
}

DATA_TYPE ArithmeticOperationLiteralId(Token *literal, VariableSymbol *id, Token *operator, bool literal_on_top)
{
    // Get the resulting type of the operation
    DATA_TYPE result_type = literal->token_type == DOUBLE_64 ||
                                    id->type == DOUBLE64_TYPE
                                ? DOUBLE64_TYPE
                                : INT32_TYPE;

    // Check if the literal isn't an int and the variable is a double, if yes convert the int
    if (literal->token_type == INTEGER_32 && id->type == DOUBLE64_TYPE)
    {
        // If the literal is on top of the stack, we can convert it directly
        if (literal_on_top)
            INT2FLOATS

        // The other case, we will need to use temporary variables, the variable is a double so we will temporarily store it in F0
        else
        {
            // Store the variable in F0
            PopToRegister(DOUBLE64_TYPE);

            // Convert the literal to a float
            INT2FLOATS

            // Push the variable back
            fprintf(stdout, "PUSHS GF@$F0\n");
        }
    }

    // Do the same for floats
    else if (literal->token_type == DOUBLE_64 && id->type == INT32_TYPE)
    {
        // If the literal is on top of the stack, we can convert it directly
        if (literal_on_top)
            FLOAT2INTS

        // The other case, we will need to use temporary variables, the variable is an int so we will temporarily store it in R0
        else
        {
            // Store the variable in R0
            PopToRegister(INT32_TYPE);

            // Convert the literal to an int
            FLOAT2INTS

            // Push the variable back
            fprintf(stdout, "PUSHS GF@$R0\n");
        }
    }

    // Perform the given operation
    switch (operator->token_type)
    {
    case MULTIPLICATION_OPERATOR:
        MULS break;

    case DIVISION_OPERATOR:
        if (result_type == DOUBLE64_TYPE)
            DIVS else IDIVS break;

    case ADDITION_OPERATOR:
        ADDS break;

    case SUBSTRACTION_OPERATOR:
        SUBS break;

    // Will literally never ever happen
    default:
        ErrorExit(ERROR_INTERNAL, "Invalid operator in OperationLiteralId");
    }

    return result_type;
}

int CheckLiteralVarCompatibilityArithmetic(Token *literal, Token *var, Parser *parser)
{
    // First check if var is defined
    VariableSymbol *var_symbol = SymtableStackFindVariable(parser->symtable_stack, var->attribute);
    if (var_symbol == NULL)
    {
        PrintError("Error in semantic analysis: Line %d: Undefined variable \"%s\"", parser->line_number, var->attribute);
        return ERROR_SEMANTIC_UNDEFINED;
    }

    // Check if the variable is nullable
    else if (IsNullable(var_symbol->type))
    {
        PrintError("Error in semantic analysis: Line %d: Variable \"%s\" is nullable", parser->line_number, var->attribute);
        return ERROR_SEMANTIC_TYPE_COMPATIBILITY;
    }

    /*  The types are compatible if:
        1. Their types are equal
        2. The literal is an integer and the variable is a double (implicit conversion)
        3. The literal is a double with zero decimal places and the variable is an integer (implicit conversion) TODO
    */
    if (literal->token_type == INTEGER_32 && var_symbol->type == INT32_TYPE)
        return 0; // case 1
    else if (literal->token_type == DOUBLE_64 && var_symbol->type == DOUBLE64_TYPE)
        return 0; // case 1
    else if (literal->token_type == INTEGER_32 && var_symbol->type == DOUBLE64_TYPE)
        return 0; // case 2, later convert to float
    else if (literal->token_type == DOUBLE_64 && HasZeroDecimalPlaces(literal->attribute) && var_symbol->type == INT32_TYPE)
        return 0; // case 3, later convert to int

    // If none of the cases are met, the types are incompatible
    PrintError("Error in semantic analysis: Line %d: Incompatible types in expression", parser->line_number);
    return ERROR_SEMANTIC_TYPE_COMPATIBILITY;
}

int CheckTwoVariablesCompatibilityArithmetic(Token *var_lhs, Token *var_rhs, Parser *parser)
{
    // Get the corresponding variable symbols
    VariableSymbol *lhs = SymtableStackFindVariable(parser->symtable_stack, var_lhs->attribute);
    VariableSymbol *rhs = SymtableStackFindVariable(parser->symtable_stack, var_rhs->attribute);

    // Check if the variables are defined
    if (lhs == NULL)
    {
        PrintError("Error in semantic analysis: Line %d: Undefined variable \"%s\"", parser->line_number, var_lhs->attribute);
        return ERROR_SEMANTIC_UNDEFINED;
    }

    if (rhs == NULL)
    {
        PrintError("Error in semantic analysis: Line %d: Undefined variable \"%s\"", parser->line_number, var_rhs->attribute);
        return ERROR_SEMANTIC_UNDEFINED;
    }

    // Check if the variables are nullable
    if (IsNullable(lhs->type))
    {
        PrintError("Error in semantic analysis: Line %d: Variable \"%s\" is nullable", parser->line_number, var_lhs->attribute);
        return ERROR_SEMANTIC_TYPE_COMPATIBILITY;
    }

    if (IsNullable(rhs->type))
    {
        PrintError("Error in semantic analysis: Line %d: Variable \"%s\" is nullable", parser->line_number, var_rhs->attribute);
        return ERROR_SEMANTIC_TYPE_COMPATIBILITY;
    }

    // Now we can check type compatibility (the types have to be equal)
    if (lhs->type != rhs->type)
    {
        PrintError("Error in semantic analysis: Line %d: Incompatible types in expression", parser->line_number);
        return ERROR_SEMANTIC_TYPE_COMPATIBILITY;
    }

    return 0;
}

DATA_TYPE ArithmeticOperationTwoIds(VariableSymbol *operand_left, VariableSymbol *operand_right, Token *operator)
{
    // Get the resulting type (it doesn't matter which operand we check, we assured their types are equal)
    DATA_TYPE result_type = operand_left->type == DOUBLE64_TYPE || operand_right->type == DOUBLE64_TYPE ? DOUBLE64_TYPE : INT32_TYPE;

    // Perform the operation based on the operator
    switch (operator->token_type)
    {
    case MULTIPLICATION_OPERATOR:
        MULS break;

    case DIVISION_OPERATOR:
        if (result_type == DOUBLE64_TYPE)
            DIVS else IDIVS break;

    case ADDITION_OPERATOR:
        ADDS break;

    case SUBSTRACTION_OPERATOR:
        SUBS break;

    // Will literally never ever happen
    default:
        ErrorExit(ERROR_INTERNAL, "Invalid operator in OperationTwoIds");
    }

    return result_type;
}

void BooleanOperationTwoLiterals(Token *operand_left, Token *operand_right, Token *operator)
{
    // If one of the operands is a double, we need to convert the int

    // In this case, the right operand is on top of the stack so stack conversion will suffice
    if (operand_left->token_type == DOUBLE_64 && operand_right->token_type == INTEGER_32)
        INT2FLOATS

    // In this case, the left operand is not on top of the stack so we need a temporary register for conversion
    else if (operand_left->token_type == INTEGER_32 && operand_right->token_type == DOUBLE_64)
    {
        // Since the right operand is an int, we can temporarily store it in R0
        PopToRegister(DOUBLE64_TYPE);

        // The second operand is now on top of the stack
        INT2FLOATS

        // Push the first operand back from R0
        fprintf(stdout, "PUSHS GF@$F0\n");
    }

    // Perform the operation based on the operator
    switch (operator->token_type)
    {
    case EQUAL_OPERATOR:
        EQS break;

    case NOT_EQUAL_OPERATOR:
        EQS
            NOTS break;

    case LESS_THAN_OPERATOR:
        LTS break;

    case LARGER_THAN_OPERATOR:
        GTS break;

    case LESSER_EQUAL_OPERATOR:
        GTS
            NOTS break;

    case LARGER_EQUAL_OPERATOR:
        LTS
            NOTS break;

    // Will literally never ever happen
    default:
        ErrorExit(ERROR_INTERNAL, "Invalid operator in BooleanExpressionTwoLiterals");
    }
}

int CheckLiteralVarCompatibilityBoolean(Token *literal, Token *var, Parser *parser)
{
    // First check if var is defined
    VariableSymbol *var_symbol = SymtableStackFindVariable(parser->symtable_stack, var->attribute);
    if (var_symbol == NULL)
    {
        PrintError("Error in semantic analysis: Line %d: Undefined variable \"%s\"", parser->line_number, var->attribute);
        return ERROR_SEMANTIC_UNDEFINED;
    }

    // Nullable check (== and != accept nullables, but only if both operands are nullable)
    if (IsNullable(var_symbol->type))
    {
        PrintError("Error in semantic analysis: Line %d: Comparing nullable variable \"%s\" with a constant", parser->line_number, var->attribute);
        return ERROR_SEMANTIC_TYPE_COMPATIBILITY;
    }

    // Double literal and int variable
    else if (literal->token_type == DOUBLE_64 && var_symbol->type == INT32_TYPE && !HasZeroDecimalPlaces(literal->attribute))
    {
        PrintError("Error in semantic analysis: Line %d: Incompatible types in expression", parser->line_number);
        return ERROR_SEMANTIC_TYPE_COMPATIBILITY;
    }

    return 0;
}

void BooleanOperationLiteralId(Token *literal, VariableSymbol *id, Token *operator, bool literal_top_stack)
{
    // Check if the literal isn't an int and the variable is a double, if yes convert the int
    if (literal->token_type == INTEGER_32 && id->type == DOUBLE64_TYPE)
    {
        // If the literal is on top of the stack, we can convert it directly
        if (literal_top_stack)
            INT2FLOATS

        // The other case, we will need to use temporary variables, the variable is a double so we will temporarily store it in F0
        else
        {
            // Store the variable in F0
            PopToRegister(DOUBLE64_TYPE);

            // Convert the literal to a float
            INT2FLOATS

            // Push the variable back
            fprintf(stdout, "PUSHS GF@$F0\n");
        }
    }

    // Do the same for float to int conversion
    else if (literal->token_type == DOUBLE_64 && id->type == INT32_TYPE)
    {
        // If the literal is on top of the stack, we can convert it directly
        if (literal_top_stack)
            FLOAT2INTS

        // The other case, we will need to use temporary variables, the variable is an int so we will temporarily store it in R0
        else
        {
            // Store the variable in R0
            PopToRegister(INT32_TYPE);

            // Convert the literal to an int
            FLOAT2INTS

            // Push the variable back
            fprintf(stdout, "PUSHS GF@$R0\n");
        }
    }

    // Perform the given operation
    switch (operator->token_type)
    {
    case EQUAL_OPERATOR:
        EQS break;

    case NOT_EQUAL_OPERATOR:
        EQS
            NOTS break;

    case LESS_THAN_OPERATOR:
        LTS break;

    case LARGER_THAN_OPERATOR:
        GTS break;

    case LESSER_EQUAL_OPERATOR:
        GTS
            NOTS break;

    case LARGER_EQUAL_OPERATOR:
        LTS
            NOTS break;

    // Will literally never ever happen
    default:
        ErrorExit(ERROR_INTERNAL, "Invalid operator in BooleanExpressionLiteralId");
    }
}

int CheckTwoVariablesCompatibilityBoolean(Token *var_lhs, Token *var_rhs, Token *operator, Parser * parser)
{
    // Get the corresponding variable symbols
    VariableSymbol *lhs = SymtableStackFindVariable(parser->symtable_stack, var_lhs->attribute);
    VariableSymbol *rhs = SymtableStackFindVariable(parser->symtable_stack, var_rhs->attribute);

    // Check if the variables are defined
    if (lhs == NULL)
    {
        PrintError("Error in semantic analysis: Line %d: Undefined variable \"%s\"", parser->line_number, var_lhs->attribute);
        return ERROR_SEMANTIC_UNDEFINED;
    }

    if (rhs == NULL)
    {
        PrintError("Error in semantic analysis: Line %d: Undefined variable \"%s\"", parser->line_number, var_rhs->attribute);
        return ERROR_SEMANTIC_UNDEFINED;
    }

    // Different semantic for these two bad boys, operands can be nullable BUT both have to (only difference)
    // Also, they have to match so not ?f64 == ?i32
    if (operator->token_type == EQUAL_OPERATOR || operator->token_type == NOT_EQUAL_OPERATOR)
    {
        if (lhs->type != rhs->type)
        {
            PrintError("Error in semantic analysis: Line %d: Incompatible types in expression", parser->line_number);
            return ERROR_SEMANTIC_TYPE_COMPATIBILITY;
        }

        return 0;
    }

    // With other operators we can't have nullable operands
    else if (IsNullable(lhs->type) || IsNullable(rhs->type))
    {
        PrintError("Error in semantic analysis: Line %d: Comparing nullable variables with a non-nullable one", parser->line_number);
        return ERROR_SEMANTIC_TYPE_COMPATIBILITY;
    }

    // Their types have to match
    if (lhs->type != rhs->type)
    {
        PrintError("Error in semantic analysis: Line %d: Incompatible types in expression", parser->line_number);
        return ERROR_SEMANTIC_TYPE_COMPATIBILITY;
    }

    return 0;
}

void BooleanOperationTwoIds(Token *operator)
{
    // Perform the operation based on the operator
    switch (operator->token_type)
    {
    case EQUAL_OPERATOR:
        EQS break;

    case NOT_EQUAL_OPERATOR:
        EQS
            NOTS break;

    case LESS_THAN_OPERATOR:
        LTS break;

    case LARGER_THAN_OPERATOR:
        GTS break;

    case LESSER_EQUAL_OPERATOR:
        GTS
            NOTS break;

    case LARGER_EQUAL_OPERATOR:
        LTS
            NOTS break;

    // Will literally never ever happen
    default:
        ErrorExit(ERROR_INTERNAL, "Invalid operator in BooleanExpressionTwoIds");
    }
}

DATA_TYPE ParseExpression(TokenVector *postfix, Parser *parser)
{
    // Variables used throughout the parsing
    Token *token = NULL;                               // Input token
    Token *operand_left = NULL, *operand_right = NULL; // Operand (stack top) tokens
    DATA_TYPE result_type, return_type = INT32_TYPE;   // Intermediate and final data types
    ExpressionStack *stack = ExpressionStackInit();    // Stack for operands
    VariableSymbol *id_input = NULL;                   // Variable symbols for ID inputs

    // Replace the constants for easier generation
    ReplaceConstants(postfix, parser);

    // Traverse the postfix string from left to right
    for (int i = 0; i < postfix->length; i++)
    {
        token = postfix->token_string[i];

        switch (token->token_type)
        {
        // If the token is an operand, push it onto the stack
        case INTEGER_32:
        case DOUBLE_64:
        case IDENTIFIER_TOKEN:
            // Switch up the resulting type
            if (token->token_type == DOUBLE_64)
                return_type = DOUBLE64_TYPE;

            // Check if the identifier isn't either undefined, or nullable
            if (token->token_type == IDENTIFIER_TOKEN)
            {
                // Look throughout the stack
                id_input = SymtableStackFindVariable(parser->symtable_stack, token->attribute);

                // Variable is undefined
                if (id_input == NULL)
                {
                    PrintError("Error in semantic analysis: Line %d: Undefined variable \"%s\"", parser->line_number, token->attribute);
                    DestroyStackAndVector(postfix, stack);
                    CLEANUP
                    exit(ERROR_SEMANTIC_UNDEFINED);
                }

                // Mark off the variable as used
                id_input->was_used = true;

                // Change the return type if the variable is a double
                if (id_input->type == DOUBLE64_TYPE)
                    return_type = DOUBLE64_TYPE;
            }

            // Push the operand and process the next token
            ExpressionStackPush(stack, token);
            PUSHS(token->attribute, token->token_type, LOCAL_FRAME);
            break;

        // Arithmetic operators
        case MULTIPLICATION_OPERATOR:
        case DIVISION_OPERATOR:
        case ADDITION_OPERATOR:
        case SUBSTRACTION_OPERATOR:
            // All of these are binary, so pop two operands from the stack
            operand_right = ExpressionStackPop(stack);
            operand_left = ExpressionStackPop(stack);

            /* Four possible options:
                1. Neither operand is an ID
                2. Operand 1 is an ID
                3. Operand 2 is an ID
                4. Both operands are IDs
            */

            // Case 1
            if (operand_left->token_type != IDENTIFIER_TOKEN && operand_right->token_type != IDENTIFIER_TOKEN)
                result_type = ArithmeticOperationTwoLiterals(operand_left, operand_right, token);

            // Case 2a (the left operand is an ID)
            else if (operand_left->token_type == IDENTIFIER_TOKEN && operand_right->token_type != IDENTIFIER_TOKEN)
            {
                // Check the validity of the operation
                int error_code = CheckLiteralVarCompatibilityArithmetic(operand_right, operand_left, parser);

                // The error message was already printed
                if (error_code != 0)
                {
                    DestroyStackAndVector(postfix, stack);
                    CLEANUP
                    exit(error_code);
                }

                VariableSymbol *id = SymtableStackFindVariable(parser->symtable_stack, operand_left->attribute);

                result_type = ArithmeticOperationLiteralId(operand_right, id, token, true);
            }

            // Case 2b (the right operand is an ID)
            else if (operand_left->token_type != IDENTIFIER_TOKEN && operand_right->token_type == IDENTIFIER_TOKEN)
            {
                // Check the validity of the operation
                int error_code = CheckLiteralVarCompatibilityArithmetic(operand_left, operand_right, parser);

                // The error message was already printed
                if (error_code != 0)
                {
                    DestroyStackAndVector(postfix, stack);
                    CLEANUP
                    exit(error_code);
                }

                VariableSymbol *id = SymtableStackFindVariable(parser->symtable_stack, operand_right->attribute);

                result_type = ArithmeticOperationLiteralId(operand_left, id, token, false);
            }

            // Case 3
            else
            {
                // Check the validity of the operation
                int error_code = CheckTwoVariablesCompatibilityArithmetic(operand_left, operand_right, parser);

                // The error message was already printed
                if (error_code != 0)
                {
                    DestroyStackAndVector(postfix, stack);
                    CLEANUP
                    exit(error_code);
                }

                VariableSymbol *lhs = SymtableStackFindVariable(parser->symtable_stack, operand_left->attribute);
                VariableSymbol *rhs = SymtableStackFindVariable(parser->symtable_stack, operand_right->attribute);

                // Get the resulting type
                result_type = ArithmeticOperationTwoIds(lhs, rhs, token);
            }

            /* Now we can push the result back onto the stack
                - We have to create an artificial token for the result
            */
            Token *result = InitToken();
            result->token_type = result_type == INT32_TYPE ? INTEGER_32 : DOUBLE_64;
            ExpressionStackPush(stack, result);

            return_type = result_type;

            break;

        // Boolean operators
        case EQUAL_OPERATOR:
        case NOT_EQUAL_OPERATOR:
        case LESS_THAN_OPERATOR:
        case LARGER_THAN_OPERATOR:
        case LESSER_EQUAL_OPERATOR:
        case LARGER_EQUAL_OPERATOR:
            // Again, all of these are binary, so pop two operands from the stack
            operand_right = ExpressionStackPop(stack);
            operand_left = ExpressionStackPop(stack);

            /* Once again, we have 3 cases
                1. Neither operand is an ID
                2. 1 operand is an ID
                3. Both operands are IDs
            */

            // Case 1
            if (operand_left->token_type != IDENTIFIER_TOKEN && operand_right->token_type != IDENTIFIER_TOKEN)
                BooleanOperationTwoLiterals(operand_left, operand_right, token);

            // Case 2a (the left operand is an ID)
            else if (operand_left->token_type == IDENTIFIER_TOKEN && operand_right->token_type != IDENTIFIER_TOKEN)
            {
                // Check the validity of the operation
                int error_code = CheckLiteralVarCompatibilityBoolean(operand_right, operand_left, parser);

                // The error message was already printed
                if (error_code != 0)
                {
                    DestroyStackAndVector(postfix, stack);
                    CLEANUP
                    exit(error_code);
                }

                VariableSymbol *id = SymtableStackFindVariable(parser->symtable_stack, operand_left->attribute);

                BooleanOperationLiteralId(operand_right, id, token, true);
            }

            // Case 2b (the right operand is an ID)
            else if (operand_left->token_type != IDENTIFIER_TOKEN && operand_right->token_type == IDENTIFIER_TOKEN)
            {
                // Check the validity of the operation
                int error_code = CheckLiteralVarCompatibilityBoolean(operand_left, operand_right, parser);

                // The error message was already printed
                if (error_code != 0)
                {
                    DestroyStackAndVector(postfix, stack);
                    CLEANUP
                    exit(error_code);
                }

                VariableSymbol *id = SymtableStackFindVariable(parser->symtable_stack, operand_right->attribute);

                BooleanOperationLiteralId(operand_left, id, token, false);
            }

            // Case 3
            else
            {
                // Check the validity of the operation
                int error_code = CheckTwoVariablesCompatibilityBoolean(operand_left, operand_right, token, parser);

                // The error message was already printed
                if (error_code != 0)
                {
                    DestroyStackAndVector(postfix, stack);
                    CLEANUP
                    exit(error_code);
                }

                BooleanOperationTwoIds(token);
            }

            return_type = BOOLEAN;

            // Check if the next token is a expression separator
            if (postfix->token_string[i + 1]->token_type != SEMICOLON && postfix->token_string[i + 1]->token_type != R_ROUND_BRACKET)
            {
                PrintError("Error in semantic analysis: Line %d: Unexpected token after boolean expression", parser->line_number);
                DestroyStackAndVector(postfix, stack);
                CLEANUP
                exit(ERROR_SYNTACTIC);
            }

            break;

        // End of expression, result is on top of the stack
        case SEMICOLON:
        case R_ROUND_BRACKET:
            break;

        // Will never happen, but throw an error just in case
        default:
            PrintError("Error in syntactic analysis: Line %d: Unexpected token in expression", parser->line_number);
            DestroyStackAndVector(postfix, stack);
            CLEANUP
            exit(ERROR_SYNTACTIC);
        }
    }

    DestroyStackAndVector(postfix, stack);
    return return_type;
}
