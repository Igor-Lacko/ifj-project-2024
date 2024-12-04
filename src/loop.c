/**
 * @file loop.c
 * @brief Functions for parsing and handling while loops, including nullable loops, in the IFJ24 language.
 *
 * This file contains the implementation of parsing functions for standard and nullable while loops,
 * including expression validation, symbol table manipulation, and generating corresponding pseudocode.
 * The main loop parsing function `ParseWhileLoop` handles the parsing of the loop expression, checks for semantic errors,
 * and generates appropriate assembly-like pseudocode. The nullable loop variant `ParseNullableWhileLoop` handles the
 * special case of nullable variables in the loop condition.
 *
 * Authors:
 * - Igor Lacko [xlackoi00]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shared.h"
#include "loop.h"
#include "scanner.h"
#include "core_parser.h"
#include "expression_parser.h"
#include "codegen.h"
#include "stack.h"
#include "symtable.h"
#include "vector.h"

bool IsLoopNullableType(Parser *parser)
{
    // Will help us reset to the current position later and check for errors
    int initial_index = stream_index;

    // Braces counter to identify the end of the loop
    int counter = 1;

    // While(expression)
    CheckTokenTypeVector(parser, L_ROUND_BRACKET);

    // Skip all tokens until we find the corresponding R_ROUND_BRACKET token
    Token *token;
    while (counter)
    {
        token = GetNextToken(parser);
        // 3 possible cases which we don't skip: L_ROUND_BRACKET, R_ROUND_BRACKET, EOF
        switch (token->token_type)
        {
        // Nest deeper into the expression
        case L_ROUND_BRACKET:
            counter++;
            break;

        // Nest outer from the expression
        case R_ROUND_BRACKET:
            counter--;
            break;

        // Invalid expression
        case EOF_TOKEN:
            SymtableStackDestroy(parser->symtable_stack);
            DestroySymtable(parser->global_symtable);
            DestroyTokenVector(stream);
            ErrorExit(ERROR_SYNTACTIC, "Line %d: Incorrectly ended while loop", parser->line_number);
            break; // gcc would complain about missing break, but it's not needed here

        // Skip the token
        default:
            break;
        }
    }

    // |id_without_null| (the null check is in a separate function)
    if ((token = GetNextToken(parser))->token_type == VERTICAL_BAR_TOKEN)
    {
        CheckTokenTypeVector(parser, IDENTIFIER_TOKEN);
        CheckTokenTypeVector(parser, VERTICAL_BAR_TOKEN);

        // Reset the token strean to the position before and return true
        stream_index = initial_index;
        return true;
    }

    // Same but false
    stream_index = initial_index;
    return false;
}

void ParseWhileLoop(Parser *parser)
{
    // Label ID
    int while_id = (while_label_count)++;

    // Initial while label
    WhileLabel(while_id);

    // We are before the L_ROUND_BRACKET
    CheckTokenTypeVector(parser, L_ROUND_BRACKET);

    // Parse the expression
    TokenVector *postfix = InfixToPostfix(parser);
    DATA_TYPE expr_type = ParseExpression(postfix, parser);

    // Check if the expression wasn't of a incorrect type
    if (expr_type != BOOLEAN)
    {
        DestroyTokenVector(stream);
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        ErrorExit(ERROR_SEMANTIC_TYPE_COMPATIBILITY, "Line %d: Expected boolean expression in while loop", parser->line_number);
    }

    // Pop the expression result and clear the data stack
    PopToRegister(BOOLEAN);
    CLEARS

    /* Loop pseudocode
        LABEL while_order
        If(B0 == false) JUMP(endwhile_order)
        Loop body
        JUMP(while_order)
        LABEL endwhile_order
    */

    JUMPIFEQ("$endwhile", "GF@$B0", "bool@false", while_id);

    // Opening loop '{'
    CheckTokenTypeVector(parser, L_CURLY_BRACKET);

    // Loop body
    ProgramBody(parser);

    // Jump back to the beginning of the while
    JUMP_WITH_ORDER("$while", while_id);

    EndWhileLabel(while_id);
}

void ParseNullableWhileLoop(Parser *parser)
{
    // Label ID
    int while_id = (while_label_count)++;

    /* This part is analogous to the normal while loop parsing except for while label after DEFVAR */

    // Must be done :((
    CheckTokenTypeVector(parser, L_ROUND_BRACKET);

    /* This part is not analogous to the normal while loop parsing */

    // The "expression" has to be a variable which can contain null (so the token has to be an identifier)
    Token *token = CheckAndReturnTokenVector(parser, IDENTIFIER_TOKEN);

    // Check if it's a defined variable
    VariableSymbol *var = SymtableStackFindVariable(parser->symtable_stack, token->attribute);
    if (var == NULL)
    {
        PrintError("Error in semantic analysis: Line %d: Undefined variable \"%s\"", token->attribute);
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        DestroyTokenVector(stream);
        exit(ERROR_SEMANTIC_UNDEFINED);
    }

    else if (!IsNullable(var->type))
    {
        PrintError("Error in semantic analysis: Line %d: Variable \"%s\" is not of a nullable type", var->name);
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        DestroyTokenVector(stream);
        exit(ERROR_SEMANTIC_TYPE_COMPATIBILITY);
    }
    var->was_used = true;

    /* Loop pseudocode
        LABEL while_order
        DEFVAR id  // this has to be done before the loop to avoid redefinition, worst case it is never used
        if(var == NULL) JUMP(endwhile_order)
        MOVE var2 var
        Loop body
        JUMP(while_order)
        LABEL endwhile_order
    */

    // Closing ')'
    CheckTokenTypeVector(parser, R_ROUND_BRACKET);

    // Opening '|'
    CheckTokenTypeVector(parser, VERTICAL_BAR_TOKEN);

    // new variable
    token = CheckAndReturnTokenVector(parser, IDENTIFIER_TOKEN);
    VariableSymbol *var2 = VariableSymbolInit();
    var2->defined = true;
    var2->is_const = false;
    var2->name = strdup(token->attribute);
    var2->type = NullableToNormal(var->type);

    // Closing '|'
    CheckTokenTypeVector(parser, VERTICAL_BAR_TOKEN);

    // Make a new entry in the symtable
    InsertVariableSymbol(parser, var2);
    // DefineVariable(var2->name, LOCAL_FRAME);

    // Initial while label
    WhileLabel(while_id);

    // if(var == NULL) JUMP(endwhile_order)
    fprintf(stdout, "JUMPIFEQ $endwhile%d LF@%s nil@nil\n", while_id, var->name);

    // var2 = var
    fprintf(stdout, "MOVE LF@%s LF@%s\n", var2->name, var->name);

    // Opening loop '{'
    CheckTokenTypeVector(parser, L_CURLY_BRACKET);

    // Loop body
    ProgramBody(parser);

    // JUMP while_order
    JUMP_WITH_ORDER("$while", while_id)

    EndWhileLabel(while_id);
}