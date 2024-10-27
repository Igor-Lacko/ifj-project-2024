#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "embedded_functions.h"
#include "scanner.h"
#include "error.h"
#include "symtable.h"
#include "stack.h"

// ifj.function(params)
FunctionSymbol *IsEmbeddedFunction(Parser *parser)
{
    Token *token = GetNextToken(&parser->line_number);

    // If the next token is not '.' it's an error: Checking if 'ifj' is a variable is done before this function
    if (token->token_type != DOT_TOKEN)
    {
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        DestroyToken(token);
        ErrorExit(ERROR_SEMANTIC_UNDEFINED, "Line %d: Undefined variable \"ifj\"");
    }

    DestroyToken(token); // Get rid of the previous token

    // The next token has to be a identifier
    if ((token = GetNextToken(&parser->line_number))->token_type != IDENTIFIER_TOKEN)
    {
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        DestroyToken(token);
        ErrorExit(ERROR_SYNTACTIC, "Line %d: Expected a embedded function name following \"ifj.\"");
    }

    // Check if it matches a IFJ function first, the user can also type in ifj.myFoo which would be an error
    for (int i = 0; i < EMBEDDED_FUNCTION_COUNT; i++)
    {
        if (!strcmp(embedded_names[i], token->attribute)) // Match found
        {
            FunctionSymbol *func = FindFunctionSymbol(parser->global_symtable, token->attribute);
            DestroyToken(token);
            return func;
        }
    }

    // The identifier was not an embedded function
    PrintError("Error in syntax analysis: Line %d: Invalid embedded function name \"%s\"", parser->line_number, token->attribute);
    DestroyToken(token);
    SymtableStackDestroy(parser->symtable_stack);
    DestroySymtable(parser->global_symtable);
    exit(ERROR_SYNTACTIC);
}

void InsertEmbeddedFunctions(Parser *parser)
{
    for (int i = 0; i < EMBEDDED_FUNCTION_COUNT; i++)
    {
        // Create a new function symbol
        FunctionSymbol *func = FunctionSymbolInit();
        func->name = strdup(embedded_names[i]);
        func->return_type = embedded_return_types[i];
        func->was_called = false; // This doesn't really matter with embedded functions

        // Create the function's parameters
        for (int j = 0; j < MAXPARAM_EMBEDDED_FUNCTION && embedded_parameters[i][j] != VOID_TYPE; j++)
        {
            // Create a parameter according to the table
            VariableSymbol *param = VariableSymbolInit();
            param->type = embedded_parameters[i][j];

            // Insert the new parameter into the function
            if ((func->parameters = realloc(func->parameters, sizeof(VariableSymbol *) * (func->num_of_parameters + 1))) == NULL)
            {
                SymtableStackDestroy(parser->symtable_stack);
                DestroySymtable(parser->global_symtable);
                ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
            }

            func->parameters[func->num_of_parameters++] = param;
        }

        // Insert the function symbol into the parser's global symtable
        if (!InsertFunctionSymbol(parser->global_symtable, func))
        {
            // This condition should never be false, but just in case put this here
            SymtableStackDestroy(parser->symtable_stack);
            DestroySymtable(parser->global_symtable);
            ErrorExit(ERROR_INTERNAL, "Multiple instances of embedded function in the global symtable. Fix your code!!!");
        }
    }
}