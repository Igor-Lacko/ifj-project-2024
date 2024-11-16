#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "conditionals.h"
#include "core_parser.h"
#include "expression_parser.h"
#include "codegen.h"
#include "shared.h"
#include "error.h"
#include "symtable.h"
#include "vector.h"
#include "stack.h"

bool IsIfNullableType(Parser *parser)
{
    // Will help us reset to the current position later and check for errors
    int initial_index = stream_index;

    // Braces counter
    int counter = 1;

    // If(expression)
    CheckTokenTypeVector(parser, L_ROUND_BRACKET);

    // Skip all tokens until we find the corresponding R_ROUND_BRACKET token
    Token *token;
    while(counter)
    {
        token = GetNextToken(parser);
        // 3 possible cases which we don't skip: L_ROUND_BRACKET, R_ROUND_BRACKET, EOF
        switch(token->token_type)
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
                ErrorExit(ERROR_SYNTACTIC, "Line %d: Incorrectly ended if statement", parser->line_number);
                break; // gcc would complain about missing break, but it's not needed here

            // Skip the token
            default:
                break;
        }
    }

    // |id_without_null| (the null check is in a separate function)
    if((token = GetNextToken(parser))->token_type == VERTICAL_BAR_TOKEN)
    {
        CheckTokenTypeVector(parser, IDENTIFIER_TOKEN);
        CheckTokenTypeVector(parser, VERTICAL_BAR_TOKEN);

        // Reset the stream index
        stream_index = initial_index;
        return true;
    }

    stream_index = initial_index;
    return false;
}

void ParseIfStatement(Parser *parser)
{
    // Initial if label
    IfLabel();

    // Before the initial L_ROUND_BRACKET
    CheckTokenTypeVector(parser, L_ROUND_BRACKET);

    // Parse the inside expression
    TokenVector *postfix = InfixToPostfix(parser);
    DATA_TYPE expr_type = GeneratePostfixExpression(parser, postfix, NULL);

    // Check if the expression is a boolean
    if(expr_type != BOOLEAN)
    {
        DestroyTokenVector(stream);
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        ErrorExit(ERROR_SEMANTIC_TYPE_COMPATIBILITY, "Line %d: Expected boolean expression in conditional", parser->line_number);
    }

    /* If statement pseudocode
        LABEL if_order
        POPS B0
        if(B0 == false) JUMP(else_order)
        If body
        JUMP(endif_order)
        LABEL else_order
        Else body
        LABEL endif_order
    */

    // If(B0 == false) JUMP(else)
    JUMPIFEQ("else", "GF@B0", "bool@false", if_label_count);

    // If body
    ProgramBody(parser);

    // JUMP(endif_order)
    JUMP_WITH_ORDER("endif", if_label_count);

    // Else label
    ElseLabel();

    // Else body
    ProgramBody(parser);

    // Final if label
    EndIfLabel();
}

void ParseNullableIfStatement(Parser *parser)
{
    /* Just like with loops, this part is analogous to the normal if statement parsing */

    // Initial if label
    IfLabel();

    // Before the initial L_ROUND_BRACKET
    CheckTokenTypeVector(parser, L_ROUND_BRACKET);

    /* This part is not analogous to the normal if statement parsing */

    // Has to be a nullable variable
    Token *token = CheckAndReturnTokenVector(parser, IDENTIFIER_TOKEN);

    // Get the symbol representing the variable
    VariableSymbol *var = SymtableStackFindVariable(parser->symtable_stack, token->attribute);

    // Undefined case
    if(var == NULL)
    {
        PrintError("Error in semantic analysis: Line %d: Undefined variable \"%s\"", parser->line_number, token->attribute);
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        DestroyTokenVector(stream);
        exit(ERROR_SEMANTIC_UNDEFINED);
    }

    // Non-nullable case
    else if(!IsNullable(var->type))
    {
        PrintError("Error in semantic analysis: Line %d: Variable \"%s\" is not of a nullable type", var->name);
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        DestroyTokenVector(stream);
        exit(ERROR_SEMANTIC_TYPE_COMPATIBILITY);
    }

    /* Loop pseudocode
        LABEL if_order
        if(var == NULL) JUMP(else_order)
        DEFVAR id
        MOVE id var
        If body
        JUMP(endif_order)
        LABEL else_order
        Else body
        LABEL endif_order
    */

    // Closing ')
    CheckTokenTypeVector(parser, R_ROUND_BRACKET);

    // |id_without_null|
    CheckTokenTypeVector(parser, VERTICAL_BAR_TOKEN);

    // Get the "id_without_null" part
    token = CheckAndReturnTokenVector(parser, IDENTIFIER_TOKEN);

    // Create a new variable symbol representing it
    VariableSymbol *new = VariableSymbolInit();
    new->defined = true;
    new->is_const = false;
    new->name = strdup(token->attribute);
    new->type = NullableToNormal(var->type);

    // New entry in the symtable
    InsertVariableSymbol(parser->symtable, new);

    // If(var == NULL) JUMP(else_order)
    fprintf(stdout, "JUMPIFEQ else%d LF@%s nil@nil\n", if_label_count, var->name);

    // DEFVAR var
    DefineVariable(new->name, LOCAL_FRAME);

    // id = var
    fprintf(stdout, "MOVE LF@%s LF@%s\n", new->name, var->name);

    // If body
    ProgramBody(parser);

    // JUMP endif_order
    JUMP_WITH_ORDER("endif", if_label_count);

    // LABEL else_order
    ElseLabel();

    // Else body
    ProgramBody(parser);

    // LABEL endif_order
    EndIfLabel();
}