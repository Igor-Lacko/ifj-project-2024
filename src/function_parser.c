#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shared.h"
#include "function_parser.h"
#include "error.h"
#include "core_parser.h"
#include "symtable.h"
#include "vector.h"
#include "stack.h"
#include "scanner.h"


// pub fn id ( seznam_parametrů ) návratový_typ {
// sekvence_příkazů
// }


void ParseVariableDeclaration(Parser *parser)
{

    Token *token;
    token = CheckAndReturnTokenStream(parser, IDENTIFIER_TOKEN);

    if (token == NULL)
    {
        PrintError("Error in semantic analysis: Line %d: Expected identifier", parser->line_number);
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        DestroyTokenVector(stream);
        exit(ERROR_SYNTACTIC);
    }

    if(parser->current_function == NULL)
    {
        PrintError("Error in semantic analysis: Line %d: Variable declaration outside of a function", parser->line_number);
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        DestroyTokenVector(stream);
        exit(ERROR_SEMANTIC_OTHER); // TODO
    }

    if(strcmp(token->attribute, "ifj") == 0)
    {
        return;
    }

    // Add the variable name to the current function's variables array
    AppendStringArray(&parser->current_function->variables, token->attribute);


}

void ParseConstDeclaration(Parser *parser){
    Token *token;
    token = CheckAndReturnTokenStream(parser, IDENTIFIER_TOKEN);

    if (token == NULL)
    {
        ErrorExit(ERROR_SYNTACTIC, "Expected identifier at line %d", parser->line_number);
    }

    if(strcmp(token->attribute, "ifj") == 0)
    {
        return;
    }

    AppendStringArray(&parser->current_function->variables, token->attribute);
}


void ParseFunctionDefinition(Parser *parser)
{
    Token *token;
    CheckKeywordTypeStream(parser, FN);
    token = CheckAndReturnTokenStream(parser, IDENTIFIER_TOKEN);

    // flag for checking return/params of main function
    bool is_main = false;

    FunctionSymbol *func;

    // Check if the function exists already (so if it was redefined, which is an error)
    if ((func = FindFunctionSymbol(parser->global_symtable, token->attribute)) == NULL)
    {
        func = FunctionSymbolInit();
        func->name = strdup(token->attribute);
        InsertFunctionSymbol(parser->global_symtable, func);
        parser->current_function = func;
    }

    // the function already exists in some scope
    else
    {
        PrintError("Error in semantic analysis: Line %d: Redefinition of function %s",
                   parser->line_number, func->name);
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        DestroyTokenVector(stream);
        exit(ERROR_SEMANTIC_REDEFINED);
    }


    // check if the main function is present
    if (!strcmp(func->name, "main"))
    {
        parser->has_main = true;
        is_main = true;
    }

    token = CheckAndReturnTokenStream(parser, L_ROUND_BRACKET);
    ParseParameters(parser, func); // params with )

    if ((token = LoadTokenFromStream(&parser->line_number))->token_type != KEYWORD || 
    (token->keyword_type != I32 && token->keyword_type != F64 && token->keyword_type != U8 && token->keyword_type != VOID))
    {
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        DestroyToken(token);
        DestroyTokenVector(stream);
        ErrorExit(ERROR_SYNTACTIC, "Expected data type at line %d", parser->line_number);
    }

    AppendToken(stream, token);

    // set the return type
    switch (token->keyword_type)
    {
    case I32:
        func->return_type = INT32_TYPE;
        break;
    case F64:
        func->return_type = DOUBLE64_TYPE;
        break;
    case U8:
        func->return_type = U8_ARRAY_TYPE;
        break;
    default:
        func->return_type = VOID_TYPE;
        break;
    }

    // Check for correct return type/params in case of main
    if (is_main && (func->return_type != VOID_TYPE || func->num_of_parameters != 0))
    {
        DestroySymtable(parser->global_symtable);
        SymtableStackDestroy(parser->symtable_stack);
        DestroyTokenVector(stream); 
        ErrorExit(ERROR_SEMANTIC_TYPECOUNT_FUNCTION, "Main function has incorrect return type or parameters");
    }

    token = CheckAndReturnTokenStream(parser, L_CURLY_BRACKET);
}

// checks all parameters
void ParseParameters(Parser *parser, FunctionSymbol *func)
{
    Token *token;
    int param_count = 0;

    // loops through all parameters
    while (1)
    {
        token = LoadTokenFromStream(&parser->line_number);
        if (token->token_type == R_ROUND_BRACKET) // reached ')' so all parameters are checked
        {
            AppendToken(stream, token);
            break;
        }

        if (token->token_type == EOF_TOKEN) // reached EOF without ')'
        {
            DestroyToken(token);
            DestroySymtable(parser->symtable);
            SymtableStackDestroy(parser->symtable_stack);
            DestroyTokenVector(stream);
            ErrorExit(ERROR_SYNTACTIC, "Didn't you forget ) at line %d ?", parser->line_number);
        }

        // id : data_type
        if (token->token_type == IDENTIFIER_TOKEN)
        {
            AppendToken(stream, token);
            VariableSymbol *var = VariableSymbolInit();
            var->name = strdup(token->attribute);
            var->is_const = false;

            for(int i = 0; i < func->num_of_parameters; i++)
            {
                if(!strcmp(func->parameters[i]->name, var->name))
                {
                    PrintError("Error in semantic analysis: Line %d: Multiple parameters with name '%s' in function '%s'",
                               parser->line_number, var->name, func->name);

                    // free resources
                    SymtableStackDestroy(parser->symtable_stack);
                    DestroySymtable(parser->global_symtable);
                    DestroyVariableSymbol(var);
                    DestroyTokenVector(stream);

                    exit(ERROR_SEMANTIC_REDEFINED);
                }
            }

            // TODO: possible var leak (???? Explain pls)
            CheckTokenTypeStream(parser, COLON_TOKEN);

            if ((token = LoadTokenFromStream(&parser->line_number))->token_type != KEYWORD && (token->keyword_type != I32 || token->keyword_type != F64 || token->keyword_type != U8))
            {
                DestroyToken(token);
                DestroyTokenVector(stream);
                DestroyVariableSymbol(var);
                SymtableStackDestroy(parser->symtable_stack);
                DestroySymtable(parser->global_symtable);
                ErrorExit(ERROR_SYNTACTIC, "Expected data type at line %d", parser->line_number);
            }

            AppendToken(stream, token);

            // add parameter data type to the function
            switch (token->keyword_type)
            {
            case I32:
                var->type = INT32_TYPE;
                break;
            case F64:
                var->type = DOUBLE64_TYPE;
                break;
            case U8:
                var->type = U8_ARRAY_TYPE;
                break;
            default:
                var->type = VOID_TYPE;
                break;
            }

            // add parameter to the function symbol
            func->parameters = realloc(func->parameters, (param_count + 1) * sizeof(VariableSymbol *));
            func->parameters[param_count++] = var;

            // checks if there is another parameter
            if ((token = LoadTokenFromStream(&parser->line_number))->token_type != COMMA_TOKEN)
            {
                // no more parameters
                if (token->token_type == R_ROUND_BRACKET)
                {
                    AppendToken(stream, token);
                    break;
                }

                else
                {
                    DestroyToken(token);
                    DestroyTokenVector(stream);
                    DestroySymtable(parser->global_symtable);
                    SymtableStackDestroy(parser->symtable_stack);
                    ErrorExit(ERROR_SYNTACTIC, "Expected ',' or ')' at line %d", parser->line_number);
                }
            }
            AppendToken(stream, token);
        }
        else
        {
            DestroyToken(token);
            DestroyTokenVector(stream);
            DestroySymtable(parser->global_symtable);
            SymtableStackDestroy(parser->symtable_stack);
            ErrorExit(ERROR_SYNTACTIC, "Expected identifier at line %d", parser->line_number);
        }
    }

    func->num_of_parameters = param_count;
}

void ParseFunctions(Parser *parser)
{
    // The token vector to store the tokens
    stream = InitTokenVector();

    // Get the line number of the first token
    Token *token;

    while ((token = LoadTokenFromStream(&parser->line_number))->token_type != EOF_TOKEN)
    {
        // Reserve the new token
        AppendToken(stream, token);

        // Check the nested level
        if (token->token_type == R_CURLY_BRACKET)
        {
            parser->nested_level--;
        }
        else if (token->token_type == L_CURLY_BRACKET)
        {
            parser->nested_level++;
        }

        if (token->keyword_type == PUB)
        {
            // Parse the function definition
            ParseFunctionDefinition(parser);

            // Nested level has to be 0: IFJ24 doesn't support nesting function definitions inside other blocks
            if (parser->nested_level != 0)
            {
                SymtableStackDestroy(parser->symtable_stack);
                DestroySymtable(parser->global_symtable);
                DestroyTokenVector(stream);
                ErrorExit(ERROR_SEMANTIC_OTHER, "Line %d: Function definition cannot be nested inside another block!!!",
                            parser->line_number);
            }

            // Increment due to the '{' at the end after the function definition to avoid the previous error
            parser->nested_level++;
        }

        if(token->keyword_type == VAR){
            ParseVariableDeclaration(parser);
        }

        if(token->keyword_type == CONST){
            ParseConstDeclaration(parser);
        }

    }

    // Append the EOF token
    AppendToken(stream, token);

    // The line number of the first token
    parser->line_number = stream->token_string[0]->line_number; 
}
