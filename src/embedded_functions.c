/**
 * @file embedded_functions.c
 * @brief Functions for handling embedded functions in the IFJCode24 interpreter.
 *
 * This file implements functions for identifying, inserting, and calling
 * embedded functions in the IFJCode24 interpreter. These functions include
 * checking whether a function is embedded, inserting the embedded functions
 * into the symbol table, parsing their parameters, and generating the corresponding
 * intermediate code. The supported embedded functions include I/O operations
 * (e.g., read/write), type conversions, string manipulation, and more.
 *
 * Authors:
 * - Igor Lacko [xlackoi00]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "embedded_functions.h"
#include "shared.h"
#include "error.h"
#include "symtable.h"
#include "stack.h"
#include "core_parser.h"
#include "vector.h"
#include "codegen.h"
#include "scanner.h"

// ifj.function(params)
FunctionSymbol *IsEmbeddedFunction(Parser *parser)
{
    Token *token = GetNextToken(parser);

    // If the next token is not '.' it's an error: Checking if 'ifj' is a variable is done before this function
    if (token->token_type != DOT_TOKEN)
    {
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        DestroyTokenVector(stream);
        ErrorExit(ERROR_SEMANTIC_UNDEFINED, "Line %d: Undefined variable \"ifj\"");
    }

    // The next token has to be a identifier
    if ((token = GetNextToken(parser))->token_type != IDENTIFIER_TOKEN)
    {
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        DestroyTokenVector(stream);
        ErrorExit(ERROR_SYNTACTIC, "Line %d: Expected a embedded function name following \"ifj.\"");
    }

    // Check if it matches a IFJ function first, the user can also type in ifj.myFoo which would be an error
    for (int i = 0; i < EMBEDDED_FUNCTION_COUNT; i++)
    {
        if (!strcmp(embedded_names[i], token->attribute)) // Match found
        {
            FunctionSymbol *func = FindFunctionSymbol(parser->global_symtable, token->attribute);
            stream_index -= 2; // Move the stream back to the beginning of the embedded function
            return func;
        }
    }

    // The identifier was not an embedded function
    PrintError("Error in semantic analysis: Line %d: Invalid embedded function name \"%s\"", parser->line_number, token->attribute);
    DestroyTokenVector(stream);
    SymtableStackDestroy(parser->symtable_stack);
    DestroySymtable(parser->global_symtable);
    exit(ERROR_SEMANTIC_UNDEFINED);
}

void InsertEmbeddedFunctions(Parser *parser)
{
    for (int i = 0; i < EMBEDDED_FUNCTION_COUNT; i++)
    {
        // Create a new function symbol
        FunctionSymbol *func = FunctionSymbolInit();
        func->name = strdup(embedded_names[i]);
        func->return_type = embedded_return_types[i];

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
                DestroyTokenVector(stream);
                ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
            }

            func->parameters[func->num_of_parameters++] = param;
        }

        // Insert the function symbol into the parser's global symtable
        if (!InsertFunctionSymbol(parser, func))
        {
            // This condition should never be false, but just in case put this here
            SymtableStackDestroy(parser->symtable_stack);
            DestroySymtable(parser->global_symtable);
            DestroyTokenVector(stream);
            ErrorExit(ERROR_INTERNAL, "Multiple instances of embedded function in the global symtable. Fix your code!!!");
        }
    }
}

TokenVector *ParseEmbeddedFunctionParams(Parser *parser, FunctionSymbol *func)
{
    // The array we will use to generate code later on
    TokenVector *operands = InitTokenVector();

    // Generic token variable
    Token *token;

    // Generic VariableSymbol variable since it can't be defined right after a switch case label
    VariableSymbol *var;

    // Parameter counter -- we could go by the length of the operands TokenVector, but this is more readable
    int loaded = 0;

    while ((token = GetNextToken(parser))->token_type != R_ROUND_BRACKET)
    {
        switch (token->token_type)
        {
        // ID token --> we have to check if it's a defined variable and if the types are compatible
        case IDENTIFIER_TOKEN:

            // Undefined variable case
            if ((var = SymtableStackFindVariable(parser->symtable_stack, token->attribute)) == NULL)
            {
                fprintf(stderr, "Undefined variable \"%s\"\n", token->attribute);
                PrintError("Error in syntax analysis: Line %d: Undefined variable \"%s\"", parser->line_number, token->attribute);
                DestroyTokenVector(operands);
                DestroyTokenVector(stream);
                SymtableStackDestroy(parser->symtable_stack);
                DestroySymtable(parser->global_symtable);
                exit(ERROR_SEMANTIC_UNDEFINED);
            }

            // Incompatible types case
            else if (!CheckParamType(func->parameters[loaded]->type, var->type))
            {
                PrintError("Error in semantic analysis: Line %d: Incompatible types in parameter %d of function call \"%s\"", parser->line_number, loaded + 1, func->name);
                fprintf(stderr, "Expected type %d, got %d\n", func->parameters[loaded]->type, var->type);
                DestroyTokenVector(operands);
                DestroyTokenVector(stream);
                SymtableStackDestroy(parser->symtable_stack);
                DestroySymtable(parser->global_symtable);
                exit(ERROR_SEMANTIC_TYPECOUNT_FUNCTION);
            }
            var->was_used = true;
            // If we got here, we have a valid parameter
            AppendToken(operands, CopyToken(token));
            loaded++;
            break;

        case INTEGER_32:

            // Incompatible types case
            if (!CheckParamType(func->parameters[loaded]->type, INT32_TYPE))
            {
                PrintError("Error in semantic analysis: Line %d: Incompatible types in parameter %d of function call \"%s\"", parser->line_number, loaded + 1, func->name);
                DestroyTokenVector(operands);
                DestroyTokenVector(stream);
                SymtableStackDestroy(parser->symtable_stack);
                DestroySymtable(parser->global_symtable);
                exit(ERROR_SEMANTIC_TYPECOUNT_FUNCTION);
            }

            // Valid parameter
            AppendToken(operands, CopyToken(token));
            loaded++;
            break;

        case DOUBLE_64:

            // Incompatible types case
            if (!CheckParamType(func->parameters[loaded]->type, DOUBLE64_TYPE))
            {
                PrintError("Error in semantic analysis: Line %d: Incompatible types in parameter %d of function call \"%s\"", parser->line_number, loaded + 1, func->name);
                DestroyTokenVector(operands);
                DestroyTokenVector(stream);
                SymtableStackDestroy(parser->symtable_stack);
                DestroySymtable(parser->global_symtable);
                exit(ERROR_SEMANTIC_TYPECOUNT_FUNCTION);
            }

            // Valid parameter
            AppendToken(operands, CopyToken(token));
            loaded++;
            break;

        case LITERAL_TOKEN:

            // Incompatible types case
            if (!CheckParamType(func->parameters[loaded]->type, U8_ARRAY_TYPE))
            {
                PrintError("Error in semantic analysis: Line %d: Incompatible types in parameter %d of function call \"%s\"", parser->line_number, loaded + 1, func->name);
                DestroyTokenVector(operands);
                DestroyTokenVector(stream);
                SymtableStackDestroy(parser->symtable_stack);
                DestroySymtable(parser->global_symtable);
                exit(ERROR_SEMANTIC_TYPECOUNT_FUNCTION);
            }

            // Valid parameter
            AppendToken(operands, CopyToken(token));
            loaded++;
            break;

        // Invalid/unexpected token
        default:
            PrintError("Error in syntax analysis: Line %d: Unexpected token \"%s\" in function call \"%s\"", parser->line_number, token->attribute, func->name);
            DestroyTokenVector(operands);
            DestroyTokenVector(stream);
            SymtableStackDestroy(parser->symtable_stack);
            DestroySymtable(parser->global_symtable);
            exit(ERROR_SYNTACTIC);
        }

        // Check the param count
        if (loaded > func->num_of_parameters)
        {
            PrintError("Error in semantic analysis: Line %d: Invalid parameter count when calling function '%s': Expected %d, got %d", parser->line_number, func->name, func->num_of_parameters, loaded);
            DestroyTokenVector(operands);
            DestroyTokenVector(stream);
            SymtableStackDestroy(parser->symtable_stack);
            DestroySymtable(parser->global_symtable);
            exit(ERROR_SEMANTIC_TYPECOUNT_FUNCTION);
        }

        // If we have reached the last param, we can exit the function
        if (loaded == func->num_of_parameters)
        {
            CheckTokenTypeVector(parser, R_ROUND_BRACKET);
            break;
        }

        // Check for a comma (in this case, the function is being called so the last param can't be empty)
        else
            CheckTokenTypeVector(parser, COMMA_TOKEN);
    }

    return operands;
}

void EmbeddedFunctionCall(Parser *parser, FunctionSymbol *func, VariableSymbol *var)
{

    // Give the variable a type derived from the function if it doen't have one yet
    if (var != NULL && var->type == VOID_TYPE)
        var->type = func->return_type;

    CheckTokenTypeVector(parser, L_ROUND_BRACKET);

    // Load the parameters
    // If this is successsful, the types and count of the parameters are checked and are 100% valid
    TokenVector *params = ParseEmbeddedFunctionParams(parser, func);
    CheckTokenTypeVector(parser, SEMICOLON);

    // Now we can generate the code depending on the function
    if (!strcmp(func->name, "readi32"))
        READ(var, LOCAL_FRAME, INT32_TYPE);

    else if (!strcmp(func->name, "readf64"))
        READ(var, LOCAL_FRAME, DOUBLE64_TYPE);

    else if (!strcmp(func->name, "readstr"))
        READ(var, LOCAL_FRAME, U8_ARRAY_TYPE);

    else if (!strcmp(func->name, "write"))
        WRITEINSTRUCTION(params->token_string[0], LOCAL_FRAME);

    else if (!strcmp(func->name, "i2f"))
        INT2FLOAT(var, params->token_string[0], LOCAL_FRAME, LOCAL_FRAME);

    else if (!strcmp(func->name, "f2i"))
        FLOAT2INT(var, params->token_string[0], LOCAL_FRAME, LOCAL_FRAME);

    else if (!strcmp(func->name, "length"))
        STRLEN(var, params->token_string[0], LOCAL_FRAME, LOCAL_FRAME);

    else if (!strcmp(func->name, "concat"))
        CONCAT(var, params->token_string[0], params->token_string[1], LOCAL_FRAME, LOCAL_FRAME, LOCAL_FRAME);

    else if (!strcmp(func->name, "strcmp"))
        STRCMP(var, params->token_string[0], params->token_string[1], LOCAL_FRAME, LOCAL_FRAME, LOCAL_FRAME);

    else if (!strcmp(func->name, "string"))
        STRING(var, params->token_string[0], LOCAL_FRAME, LOCAL_FRAME);

    else if (!strcmp(func->name, "ord"))
        ORD(var, params->token_string[0], params->token_string[1], LOCAL_FRAME, LOCAL_FRAME, LOCAL_FRAME);

    else if (!strcmp(func->name, "chr"))
        INT2CHAR(var, params->token_string[0], LOCAL_FRAME, LOCAL_FRAME);

    // This is by far the most complicated out of these :(
    else if (!strcmp(func->name, "substring"))
        SUBSTRING(var, params->token_string[0], params->token_string[1], params->token_string[2], LOCAL_FRAME, LOCAL_FRAME, LOCAL_FRAME, LOCAL_FRAME);

    DestroyTokenVector(params);
}