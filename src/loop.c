#include <stdio.h>
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
    int initial_index = stream_index, current_line = parser->line_number;

    // While(expression)
    CheckTokenTypeVector(parser, L_ROUND_BRACKET);

    // Skip all tokens until )
    Token *token;
    while((token = GetNextToken(parser))->token_type != R_ROUND_BRACKET)
    {
        // Check for incorrect expression
        if(current_line != parser->line_number || token->token_type == EOF_TOKEN)
        {
            SymtableStackDestroy(parser->symtable_stack);
            DestroySymtable(parser->global_symtable);
            DestroyTokenVector(stream);
            ErrorExit(ERROR_SYNTACTIC, "Line %d: Incorrectly ended while loop", parser->line_number);
        }
    }

    // |id_without_null| (the null check is in a separate function)
    if((token = GetNextToken(parser))->token_type == VERTICAL_BAR_TOKEN)
    {
        // Current line number, basically do the same as before
        current_line = parser->line_number;

        // Again, skip until the corresponding vertical bar
        while((token = GetNextToken(parser))->token_type != VERTICAL_BAR_TOKEN)
        {
            // Check for incorrect expression
            if(current_line != parser->line_number || token->token_type == EOF_TOKEN)
            {
                SymtableStackDestroy(parser->symtable_stack);
                DestroySymtable(parser->global_symtable);
                DestroyTokenVector(stream);
                ErrorExit(ERROR_SYNTACTIC, "Line %d: Incorrectly ended assignment in while loop", parser->line_number);
            }
        }

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

    // Initial while label
    WhileLabel();

    // We are before the L_ROUND_BRACKET
    CheckTokenTypeVector(parser, L_ROUND_BRACKET);

    // Parse the expression
    TokenVector *postfix = InfixToPostfix(parser);
    DATA_TYPE expr_type = GeneratePostfixExpression(parser, postfix, NULL);

    // Check if the expression wasn't of a incorrect type
    if(expr_type != BOOLEAN)
    {
        DestroyTokenVector(stream);
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        ErrorExit(ERROR_SEMANTIC_TYPE_COMPATIBILITY, "Line %d: Expected boolean expression in while loop", parser->line_number);
    }

    /* Loop pseudocode
        LABEL while_order
        If(B0 == false) JUMP(endwhile_order)
        Loop body
        JUMP(while_order)
        LABEL endwhile_order
    */

    JUMPIFNEQ("endwhile", "GF@B0", "bool@false", while_label_count);

    // Loop body
    ProgramBody(parser);

    // Jump back to the beginning of the while
    JUMP_WITH_ORDER("while", while_label_count);

    EndWhileLabel();
}

void ParseNullableWhileLoop(Parser *parser)
{
    // Initial while label
    WhileLabel();

    // Must be done :((
    CheckTokenTypeVector(parser, L_ROUND_BRACKET);
}