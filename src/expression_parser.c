/**
 * @file expression_parser.c
 * @brief Implements functions for parsing expressions and generating code for arithmetic and boolean operations.
 *
 * This file contains the implementation of functions for infix-to-postfix conversion, expression parsing,
 * and handling operand compatibility. It also provides code generation for operations involving literals,
 * variables, and identifiers.
 *
 * Authors:
 * - Igor Lacko [xlackoi00]
 */

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

const PrecedenceTable precedence_table = {
    /*ID*/ {INVALID, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, INVALID, REDUCE, REDUCE},
    /***/ {SHIFT, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, SHIFT, REDUCE, REDUCE},
    /*/*/ {SHIFT, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, SHIFT, REDUCE, REDUCE},
    /*+*/ {SHIFT, SHIFT, SHIFT, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, SHIFT, REDUCE, REDUCE},
    /*-*/ {SHIFT, SHIFT, SHIFT, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, SHIFT, REDUCE, REDUCE},
    /*==*/{SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, SHIFT, REDUCE, REDUCE},
    /*!=*/{SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, SHIFT, REDUCE, REDUCE},
    /*<*/ {SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, SHIFT, REDUCE, REDUCE},
    /*>*/ {SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, SHIFT, REDUCE, REDUCE},
    /*<=*/{SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, SHIFT, REDUCE, REDUCE},
    /*>=*/{SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, SHIFT, REDUCE, REDUCE},
    /*(*/ {SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, MATCH, INVALID},
    /*)*/ {INVALID, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, REDUCE, INVALID, REDUCE, REDUCE},
    /*$*/ {SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, SHIFT, INVALID, ACCEPT}};

PtableKey GetPtableKey(Token *token, int bracket_count)
{
    // The only time this will happen is with the initial terminal dollar
    if (token == NULL)
        return PTABLE_DOLLAR;

    switch (token->token_type)
    {
    case IDENTIFIER_TOKEN:
    case INTEGER_32:
    case DOUBLE_64:
    case KEYWORD:
        if (token->token_type == KEYWORD)
        {
            return token->keyword_type == NULL_TYPE ? PTABLE_ID : PTABLE_ERROR;
        }

        else
            return PTABLE_ID;

    case MULTIPLICATION_OPERATOR:
        return PTABLE_MULTIPLICATION;

    case DIVISION_OPERATOR:
        return PTABLE_DIVISION;

    case ADDITION_OPERATOR:
        return PTABLE_ADDITION;

    case SUBSTRACTION_OPERATOR:
        return PTABLE_SUBSTRACTION;

    case EQUAL_OPERATOR:
        return PTABLE_EQUAL;

    case NOT_EQUAL_OPERATOR:
        return PTABLE_NOT_EQUAL;

    case LESS_THAN_OPERATOR:
        return PTABLE_LESS_THAN;

    case LARGER_THAN_OPERATOR:
        return PTABLE_LARGER_THAN;

    case LESSER_EQUAL_OPERATOR:
        return PTABLE_LESSER_EQUAL;

    case LARGER_EQUAL_OPERATOR:
        return PTABLE_LARGER_EQUAL;

    case L_ROUND_BRACKET:
        return PTABLE_LEFT_BRACKET;

    case R_ROUND_BRACKET:
        return bracket_count == -1 ? PTABLE_DOLLAR : PTABLE_RIGHT_BRACKET;

    case SEMICOLON:
        return PTABLE_DOLLAR;

    default:
        return PTABLE_ERROR;
    }
}

bool IsOperand(Token *token)
{
    return token->token_type == IDENTIFIER_TOKEN || token->token_type == INTEGER_32 || token->token_type == DOUBLE_64 || (token->token_type == KEYWORD && token->keyword_type == NULL_TYPE);
}

bool IsNullable(DATA_TYPE type)
{
    return type == INT32_NULLABLE_TYPE ||
           type == DOUBLE64_NULLABLE_TYPE ||
           type == U8_ARRAY_NULLABLE_TYPE ||
           type == NULL_DATA_TYPE; // This specific one shouldn't ever happen i guess?
}

EXPRESSION_RULE FindRule(ExpressionStack *stack, int distance)
{
    // Get the stack top
    ExpressionStackNode *top = ExpressionStackTop(stack);

    // No possible rule can be applied
    if (distance != 1 && distance != 3)
        return NOT_FOUND_RULE;

    // Only E -> id can be applied here
    else if (distance == 1)
    {
        if (top->node_type == TERMINAL && IsOperand(top->token))
            return IDENTIFIER_RULE;

        return NOT_FOUND_RULE;
    }

    // Check if the stack size can actually contain a rule
    if (stack->size < 4)
        return NOT_FOUND_RULE; // 4 since we need 3 nodes + the handle

    // Special case, brackets rule (E -> (E)) --> only other type when a terminal can be on the top of the stack
    if (top->node_type == TERMINAL)
    {
        if (top->token->token_type != R_ROUND_BRACKET)
            return NOT_FOUND_RULE;
        if (top->next->node_type != NONTERMINAL)
            return NOT_FOUND_RULE;
        if (top->next->next->node_type != TERMINAL && top->next->next->token->token_type != L_ROUND_BRACKET)
            return NOT_FOUND_RULE;

        return BRACKET_RULE;
    }

    /********** ALL OTHER RULES, IN THE FORM (E OPERATOR E) (WITHOUT BRACKETS) **********/
    EXPRESSION_RULE rule;

    // E
    if (top->node_type != NONTERMINAL)
        return NOT_FOUND_RULE;

    // OPERATOR
    if (top->next->node_type != TERMINAL)
        return NOT_FOUND_RULE;
    switch (top->next->token->token_type)
    {
    case MULTIPLICATION_OPERATOR:
        rule = MULTIPLICATION_RULE;
        break;

    case DIVISION_OPERATOR:
        rule = DIVISION_RULE;
        break;

    case ADDITION_OPERATOR:
        rule = ADDITION_RULE;
        break;

    case SUBSTRACTION_OPERATOR:
        rule = SUBSTRACTION_RULE;
        break;

    case EQUAL_OPERATOR:
        rule = EQUAL_RULE;
        break;

    case NOT_EQUAL_OPERATOR:
        rule = NOT_EQUAL_RULE;
        break;

    case LESS_THAN_OPERATOR:
        rule = LESS_THAN_RULE;
        break;

    case LARGER_THAN_OPERATOR:
        rule = LARGER_THAN_RULE;
        break;

    case LESSER_EQUAL_OPERATOR:
        rule = LESSER_EQUAL_RULE;
        break;

    case LARGER_EQUAL_OPERATOR:
        rule = LARGER_EQUAL_RULE;
        break;

    default:
        return NOT_FOUND_RULE;
    }

    // E
    if (top->next->next->node_type != NONTERMINAL)
        return NOT_FOUND_RULE;

    return rule;
}

void ReduceToNonterminal(ExpressionStack *stack, TokenVector *postfix, EXPRESSION_RULE rule)
{
    /* To convert the input expression to a postfix (while validating it) in reductions:
        - Append the rhs terminals (other than brackers) to the output string
        - At the end, append the "$" symbol to the output string
    */

    // Decide based on the rule
    switch (rule)
    {
    case IDENTIFIER_RULE:
        // E -> id
        AppendToken(postfix, CopyToken(ExpressionStackTop(stack)->token));
        ExpressionStackRemoveTop(stack);

        // Remove the handle from the stack
        ExpressionStackRemoveTop(stack);

        break;

    // E -> (E)
    case BRACKET_RULE:
        // Just remove 4 nodes from the stack, we aren't appending anything to the postfix string
        ExpressionStackRemoveTop(stack);
        ExpressionStackRemoveTop(stack);
        ExpressionStackRemoveTop(stack);
        ExpressionStackRemoveTop(stack);

        break;

    // E -> E OPERATOR E (all of these have the same semantics for reduction)
    case MULTIPLICATION_RULE:
    case DIVISION_RULE:
    case ADDITION_RULE:
    case SUBSTRACTION_RULE:
    case EQUAL_RULE:
    case NOT_EQUAL_RULE:
    case LESS_THAN_RULE:
    case LARGER_THAN_RULE:
    case LESSER_EQUAL_RULE:
    case LARGER_EQUAL_RULE:
        // We only need the operator to append
        ExpressionStackRemoveTop(stack);                          // First nonterminal
        ExpressionStackNode *operator= ExpressionStackPop(stack); // Operator
        ExpressionStackRemoveTop(stack);                          // Second nonterminal
        ExpressionStackRemoveTop(stack);                          // Handle

        AppendToken(postfix, CopyToken(operator->token)); // Append the operator to the postfix string

        free(operator); // Destroy excess nodes

        break;

    // Will literally never happen
    default:
        return;
    }

    // Push the nonterminal to the stack
    ExpressionStackNode *nonterminal = ExpressionStackNodeInit(NULL, NONTERMINAL, PTABLE_NOKEY);
    ExpressionStackPush(stack, nonterminal);

    return;
}

TokenVector *InfixToPostfix(Parser *parser)
{
    // needed structures/varibles
    ExpressionStack *stack = ExpressionStackInit(); // Stack for precedence analysis
    TokenVector *postfix = InitTokenVector();       // Output postfix vector
    ExpressionStackNode *node;                      // Temporary node for pushing to the stack
    Token *token;                                   // Input token
    EXPRESSION_RULE rule = NOT_FOUND_RULE;          // Rule to be applied
    PtableKey key = PTABLE_NOKEY;                   // Key for indexing into the precedence table
    int bracket_count = 0;                          // In case the expression ends
    int distance = -1;                              // Distance to the handle
    bool expression_over = false;                   // If the expression is over, set to true after accepting the last token

    // Check if the expression isn't empty
    Token *first = GetNextToken(parser);
    if (first->token_type == SEMICOLON || first->token_type == R_ROUND_BRACKET)
    {
        PrintError("Error in syntactic analysis: Line %d: Empty expression", parser->line_number);
        CLEANUP
        exit(ERROR_SYNTACTIC);
    }

    --(stream_index); // Move the stream back to the beginning of the expression

    // Initial dollar sign
    ExpressionStackNode *beginning = ExpressionStackNodeInit(NULL, TERMINAL, PTABLE_DOLLAR);
    ExpressionStackPush(stack, beginning);

    while (true)
    {
        // Get the next token from the input if we aren't at the end of the expression, else it stays the same
        if (!expression_over)
            token = CopyToken(GetNextToken(parser));

        // Update the bracket count if needed
        if (token->token_type == L_ROUND_BRACKET && !expression_over)
            bracket_count++;
        else if (token->token_type == R_ROUND_BRACKET && !expression_over)
            bracket_count--;

        // Get the topmost terminal symbol from the stack and the token's key to index
        ExpressionStackNode *topmost = TopmostTerminal(stack);
        key = GetPtableKey(token, bracket_count);

        // Check for a invalid token
        if (key == PTABLE_ERROR)
        {
            PrintError("Error in syntactic analysis: Line %d: Unexpected token \"%s\" in expression", parser->line_number, token->attribute);
            DestroyExpressionStackAndVector(postfix, stack);
            DestroyToken(token);
            CLEANUP
            exit(ERROR_SYNTACTIC);
        }

        // Check if the expression is over
        else if (key == PTABLE_DOLLAR)
            expression_over = true;

        // Get the next action from the precedence table
        PtableValue action = precedence_table[topmost->key_type][key];
        switch (action)
        {
        // Push the input token to the stack and move on to the next iteration
        case MATCH:
            node = ExpressionStackNodeInit(token, TERMINAL, GetPtableKey(token, 1)); // TODO: check if this hard-coded 1 is correct
            ExpressionStackPush(stack, node);
            break;

        // Put a handle after the topmost terminal and push the input token to the stack
        case SHIFT:
            PushHandleAfterTopmost(stack);
            node = ExpressionStackNodeInit(token, TERMINAL, GetPtableKey(token, 1));
            ExpressionStackPush(stack, node);
            break;

        // Reduce the stack (so find a rule)
        case REDUCE:
            // Revert some actions depending on the input token since we will read it again anyway
            if (!expression_over)
                (--stream_index);
            if (token->token_type == R_ROUND_BRACKET && !expression_over)
                bracket_count++;
            if (token->token_type == L_ROUND_BRACKET && !expression_over)
                bracket_count--;
            distance = TopmostHandleDistance(stack);

            // Destroy the copy
            if (!expression_over)
                DestroyToken(token);

            // We have to reduce the nodes between the handle and the stack top
            rule = FindRule(stack, distance);

            // If no rule can be applied, throw an error
            if (rule == NOT_FOUND_RULE)
            {
                fprintf(stderr, "Invalid expression, rule not found\n");
                PrintError("Error in syntactic analysis: Line %d: Invalid expression", parser->line_number);
                DestroyExpressionStackAndVector(postfix, stack);
                DestroyToken(token);
                CLEANUP
                exit(ERROR_SYNTACTIC);
            }

            else
                ReduceToNonterminal(stack, postfix, rule);

            break;

        // Invalid expression
        case INVALID:
            fprintf(stderr, "Invalid expression\n");
            PrintError("Error in syntactic analysis: Line %d: Invalid expression", parser->line_number);
            DestroyExpressionStackAndVector(postfix, stack);
            DestroyToken(token);
            CLEANUP
            exit(ERROR_SYNTACTIC);

        // End of the expression
        case ACCEPT:
            AppendToken(postfix, CopyToken(token));
            ExpressionStackDestroy(stack);
            return postfix;
        }
    }
}

bool IsTokenInString(TokenVector *postfix, Token *token)
{
    if (token == NULL)
        return false;
    for (int i = 0; i < postfix->length; i++)
    {
        if (token == postfix->token_string[i])
            return true;
    }

    return false;
}

void DestroyExpressionStackAndVector(TokenVector *postfix, ExpressionStack *stack)
{
    while (stack->size != 0)
    {
        ExpressionStackNode *top = ExpressionStackPop(stack);
        if (top->token != NULL && !IsTokenInString(postfix, top->token))
        {
            DestroyToken(top->token);
            top->token = NULL;
            ExpressionStackRemoveTop(stack);
        }
    }

    DestroyTokenVector(postfix);
    ExpressionStackDestroy(stack);
}

void DestroyEvaluationStackAndVector(TokenVector *postfix, EvaluationStack *stack)
{
    while (stack->size != 0)
    {
        Token *top = EvaluationStackPop(stack);
        if (!IsTokenInString(postfix, top))
            DestroyToken(top);
    }

    DestroyTokenVector(postfix);
    EvaluationStackDestroy(stack);
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
            if (var != NULL && !var->nullable && var->is_const && var->value != NULL && var->type == DOUBLE64_TYPE && HasZeroDecimalPlaces(var->value))
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

int CheckLiteralsCompatibilityArithmetic(Token *var_lhs, Token *var_rhs, Token *operator, Parser * parser)
{
    // Neither can be NULL
    if (var_lhs->token_type == KEYWORD || var_rhs->token_type == KEYWORD)
    {
        PrintError("Error in semantic analysis: Line %d: Unexpected keyword in expression", parser->line_number);
        return ERROR_SYNTACTIC;
    }

    // Incompatibility can only occur with different types and the division operator
    if (operator->token_type != DIVISION_OPERATOR)
        return 0;

    // If their types don't match, they are compatible only if the float has zero decimal places
    if (var_lhs->token_type != var_rhs->token_type)
    {
        Token *f_token = var_lhs->token_type == DOUBLE_64 ? var_lhs : var_rhs;
        if (!HasZeroDecimalPlaces(f_token->attribute))
        {
            PrintError("Error in semantic analysis: Line %d: Incompatible types in division operation", parser->line_number);
            return ERROR_SEMANTIC_TYPE_COMPATIBILITY;
        }
    }

    return 0;
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

int CheckLiteralVarCompatibilityArithmetic(Token *literal, Token *var, Token *operator, Parser * parser)
{
    // The literal can't be NULL
    if (literal->token_type == KEYWORD)
    {
        PrintError("Error in semantic analysis: Line %d: Unexpected keyword in expression", parser->line_number);
        return ERROR_SYNTACTIC;
    }

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
    else if (literal->token_type == INTEGER_32 && var_symbol->type == DOUBLE64_TYPE && operator->token_type != DIVISION_OPERATOR)
        return 0; // case 2, later convert to float
    else if (literal->token_type == DOUBLE_64 && HasZeroDecimalPlaces(literal->attribute) && var_symbol->type == INT32_TYPE)
        return 0; // case 3, later convert to int

    // If none of the cases are met, the types are incompatible
    PrintError("Error in semantic analysis: Line %d: Incompatible types in expression", parser->line_number);
    return ERROR_SEMANTIC_TYPE_COMPATIBILITY;
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

int CheckCompatibilityLiteralsBoolean(Token *literal_left, Token *literal_right, Token *operator, Parser * parser)
{
    /* The literals are incompatible if, and only if these conditions are met:
        1. Both of them are NULL AND the operator is not == or !=
        2. Only one of them is NULL
    */

    // Case 1
    if (literal_left->token_type == KEYWORD && literal_right->token_type == KEYWORD && operator->token_type != EQUAL_OPERATOR && operator->token_type != NOT_EQUAL_OPERATOR)
    {
        PrintError("Error in semantic analysis: Line %d: \"null\" can't be used in expressions with operators other than \"==\" and \"!=\"!", parser->line_number);
        return ERROR_SEMANTIC_TYPE_COMPATIBILITY;
    }

    // Case 2
    else if ((literal_left->token_type == KEYWORD && literal_right->token_type != KEYWORD) ||
             (literal_left->token_type != KEYWORD && literal_right->token_type == KEYWORD))
    {
        PrintError("Error in semantic analysis: Line %d: Incompatible types in expression", parser->line_number);
        return ERROR_SEMANTIC_TYPE_COMPATIBILITY;
    }

    // Valid relational expression
    return 0;
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

int CheckLiteralVarCompatibilityBoolean(Token *literal, Token *var, Token *operator, Parser * parser)
{
    // First check if var is defined
    VariableSymbol *var_symbol = SymtableStackFindVariable(parser->symtable_stack, var->attribute);
    if (var_symbol == NULL)
    {
        PrintError("Error in semantic analysis: Line %d: Undefined variable \"%s\"", parser->line_number, var->attribute);
        return ERROR_SEMANTIC_UNDEFINED;
    }

    // Case with null literal
    if (literal->token_type == KEYWORD)
    {
        /* An error here will occur if:
            1. The operator is not == or !=
            2. The variable is not nullable
        */
        if (operator->token_type != EQUAL_OPERATOR && operator->token_type != NOT_EQUAL_OPERATOR)
        {
            PrintError("Error in semantic analysis: Line %d: \"null\" can't be used in expressions with operators other than \"==\" and \"!=\"!", parser->line_number);
            return ERROR_SEMANTIC_TYPE_COMPATIBILITY;
        }

        if (!IsNullable(var_symbol->type))
        {
            PrintError("Error in semantic analysis: Line %d: Variable \"%s\" is not nullable", parser->line_number, var->attribute);
            return ERROR_SEMANTIC_TYPE_COMPATIBILITY;
        }
    }

    // Nullable check (== and != accept nullables, but only if both operands are nullable)
    if (IsNullable(var_symbol->type) && (operator->token_type != EQUAL_OPERATOR && operator->token_type != NOT_EQUAL_OPERATOR))
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

bool ConvertConstVarsBoolean(VariableSymbol *lhs, VariableSymbol *rhs)
{
    fprintf(stderr, "trying to convert vars %s and %s\n", lhs->name, rhs->name);
    if (!(lhs->is_const && lhs->value != NULL) && !(rhs->is_const && rhs->value != NULL))
        return false;

    else if (((lhs->nullable && rhs->nullable) || (!lhs->nullable && rhs->nullable) || (lhs->nullable && !rhs->nullable)) && (lhs->type != rhs->type))
        return false;

    // Also, rhs is on the top of the stack, lhs is not
    else if (lhs->is_const && lhs->value != NULL)
    {
        switch (lhs->type)
        {
        // Convert lhs to a float
        case INT32_TYPE:
        case INT32_NULLABLE_TYPE:
            PopToRegister(DOUBLE64_TYPE);
            INT2FLOATS
            fprintf(stdout, "PUSHS GF@$F0\n");
            break;

        // Convert lhs to an int
        case DOUBLE64_TYPE:
        case DOUBLE64_NULLABLE_TYPE:
            if (!HasZeroDecimalPlaces(lhs->value))
                return false;
            PopToRegister(INT32_TYPE);
            FLOAT2INTS
            fprintf(stdout, "PUSHS GF@$R0\n");
            break;

        // Will never happen
        default:
            return false;
        }
    }

    // rhs is on top of the stack
    else if (rhs->is_const && rhs->value != NULL)
    {
        switch (rhs->type)
        {
        // Convert rhs to a float
        case INT32_TYPE:
        case INT32_NULLABLE_TYPE:
            INT2FLOATS
            break;

        // Convert rhs to an int
        case DOUBLE64_TYPE:
        case DOUBLE64_NULLABLE_TYPE:
            if (!HasZeroDecimalPlaces(rhs->value))
                return false;
            FLOAT2INTS
            break;

        // Will never happen
        default:
            return false;
        }
    }

    return true;
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
        if (lhs->type != rhs->type && !ConvertConstVarsBoolean(lhs, rhs))
        {
            PrintError("Error in semantic analysis: Line %d: Incompatible types in expression", parser->line_number);
            return ERROR_SEMANTIC_TYPE_COMPATIBILITY;
        }

        return 0;
    }

    // With other operators we can't have nullable operands
    else if ((IsNullable(lhs->type) || IsNullable(rhs->type)) && (operator->token_type != EQUAL_OPERATOR && operator->token_type != NOT_EQUAL_OPERATOR))
    {
        PrintError("Error in semantic analysis: Line %d: Using nullable variables with operators other than \"==\" or \"!=\"", parser->line_number);
        return ERROR_SEMANTIC_TYPE_COMPATIBILITY;
    }

    // Their types have to match, or they both have to be constants with values known at compile time
    if (lhs->type != rhs->type && !ConvertConstVarsBoolean(lhs, rhs))
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
    EvaluationStack *stack = EvaluationStackInit();    // Stack for evaluation
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
        case KEYWORD:
            // Switch up the resulting type
            if (token->token_type == DOUBLE_64)
                return_type = DOUBLE64_TYPE;

            if (token->token_type == KEYWORD && token->keyword_type != NULL_TYPE)
            {
                PrintError("Error in semantic analysis: Line %d: Unexpected keyword \"%s\" in expression", parser->line_number, token->attribute);
                DestroyEvaluationStackAndVector(postfix, stack);
                CLEANUP
                exit(ERROR_SYNTACTIC);
            }

            // Check if the identifier isn't either undefined, or nullable
            if (token->token_type == IDENTIFIER_TOKEN)
            {
                // Look throughout the stack
                id_input = SymtableStackFindVariable(parser->symtable_stack, token->attribute);

                // Variable is undefined
                if (id_input == NULL)
                {
                    PrintError("Error in semantic analysis: Line %d: Undefined variable \"%s\"", parser->line_number, token->attribute);
                    DestroyEvaluationStackAndVector(postfix, stack);
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
            EvaluationStackPush(stack, token);
            PUSHS(token->attribute, token->token_type, LOCAL_FRAME);
            break;

        // Arithmetic operators
        case MULTIPLICATION_OPERATOR:
        case DIVISION_OPERATOR:
        case ADDITION_OPERATOR:
        case SUBSTRACTION_OPERATOR:
            // All of these are binary, so pop two operands from the stack
            operand_right = EvaluationStackPop(stack);
            operand_left = EvaluationStackPop(stack);

            /* Four possible options:
                1. Neither operand is an ID
                2. Operand 1 is an ID
                3. Operand 2 is an ID
                4. Both operands are IDs
            */

            // Case 1
            if (operand_left->token_type != IDENTIFIER_TOKEN && operand_right->token_type != IDENTIFIER_TOKEN)
            {
                // Check the validity of the operation
                int error_code = CheckLiteralsCompatibilityArithmetic(operand_left, operand_right, token, parser);

                // The error message was already printed
                if (error_code != 0)
                {
                    DestroyEvaluationStackAndVector(postfix, stack);
                    CLEANUP
                    exit(error_code);
                }

                result_type = ArithmeticOperationTwoLiterals(operand_left, operand_right, token);
            }

            // Case 2a (the left operand is an ID)
            else if (operand_left->token_type == IDENTIFIER_TOKEN && operand_right->token_type != IDENTIFIER_TOKEN)
            {
                // Check the validity of the operation
                int error_code = CheckLiteralVarCompatibilityArithmetic(operand_right, operand_left, token, parser);

                // The error message was already printed
                if (error_code != 0)
                {
                    DestroyEvaluationStackAndVector(postfix, stack);
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
                int error_code = CheckLiteralVarCompatibilityArithmetic(operand_left, operand_right, token, parser);

                // The error message was already printed
                if (error_code != 0)
                {
                    DestroyEvaluationStackAndVector(postfix, stack);
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
                    DestroyEvaluationStackAndVector(postfix, stack);
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
            EvaluationStackPush(stack, result);

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
            operand_right = EvaluationStackPop(stack);
            operand_left = EvaluationStackPop(stack);

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
                int error_code = CheckLiteralVarCompatibilityBoolean(operand_right, operand_left, token, parser);

                // The error message was already printed
                if (error_code != 0)
                {
                    DestroyEvaluationStackAndVector(postfix, stack);
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
                int error_code = CheckLiteralVarCompatibilityBoolean(operand_left, operand_right, token, parser);

                // The error message was already printed
                if (error_code != 0)
                {
                    DestroyEvaluationStackAndVector(postfix, stack);
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
                    DestroyEvaluationStackAndVector(postfix, stack);
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
                DestroyEvaluationStackAndVector(postfix, stack);
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
            DestroyEvaluationStackAndVector(postfix, stack);
            CLEANUP
            exit(ERROR_SYNTACTIC);
        }
    }

    DestroyEvaluationStackAndVector(postfix, stack);
    return return_type;
}
