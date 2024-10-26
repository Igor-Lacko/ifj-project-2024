#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "function_parser.h"

TokenVector *tokens = NULL;
int first_token_line = 0;

// pub fn id ( seznam_parametrů ) návratový_typ {
// sekvence_příkazů
// }
void ParseFunctionDefinition(Parser *parser)
{
    Token *token;
    token = CheckAndReturnKeyword(parser, FN);
    AppendToken(tokens, token);
    token = CheckAndReturnToken(parser, IDENTIFIER_TOKEN);
    AppendToken(tokens, token);

    // flag for checking return/params of main function
    bool is_main = false;

    FunctionSymbol *func;

    // Check if the function exists already (so if it was called, which can happen, or redefined, which is an error)
    if ((func = FindFunctionSymbol(parser->global_symtable, token->attribute)) == NULL)
    {
        func = FunctionSymbolInit();
        func->name = strdup(token->attribute);
        InsertFunctionSymbol(parser->global_symtable, func);
    }

    // the function already exists in some scope
    else
    {
        PrintError("Error in semantic analysis: Line %d: Redefinition of function %s",
                   parser->line_number, func->name);
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        DestroyToken(token);
        exit(ERROR_SEMANTIC_REDEFINED);
    }

    // Create a new symtable at the top of the stack
    Symtable *local_symtable = InitSymtable(TABLE_COUNT);
    SymtableStackPush(parser->symtable_stack, local_symtable);
    parser->symtable = local_symtable;

    // check if the main function is present
    if (!strcmp(func->name, "main"))
    {
        parser->has_main = true;
        is_main = true;
    }

    token = CheckAndReturnToken(parser, L_ROUND_BRACKET);
    AppendToken(tokens, token);
    ParseParameters(parser, func); // params with )

    if ((token = GetNextToken(&parser->line_number))->token_type != KEYWORD || (token->keyword_type != I32 && token->keyword_type != F64 && token->keyword_type != U8 && token->keyword_type != VOID))
    {
        DestroyToken(token);
        DestroySymtable(parser->symtable);
        ErrorExit(ERROR_SYNTACTIC, "Expected data type at line %d", parser->line_number);
    }

    AppendToken(tokens, token);

    // set the return type
    DATA_TYPE return_type;
    switch (token->keyword_type)
    {
    case I32:
        return_type = INT32_TYPE;
        break;
    case F64:
        return_type = DOUBLE64_TYPE;
        break;
    case U8:
        return_type = U8_ARRAY_TYPE;
        break;
    default:
        return_type = VOID_TYPE;
        break;
    }
    CHECK_RETURN_VALUE

    // Check for correct return type/params in case of main
    if (is_main && (func->return_type != VOID_TYPE || func->num_of_parameters != 0))
    {
        DestroyToken(token);
        DestroySymtable(parser->global_symtable);
        SymtableStackDestroy(parser->symtable_stack);
        ErrorExit(ERROR_SEMANTIC_TYPECOUNT_FUNCTION, "Main function has incorrect return type or parameters");
    }

    token = CheckAndReturnToken(parser, L_CURLY_BRACKET);
    AppendToken(tokens, token);
}

// checks all parameters
void ParseParameters(Parser *parser, FunctionSymbol *func)
{
    Token *token;
    int param_count = 0;

    // loops through all parameters
    while (1)
    {
        token = GetNextToken(&parser->line_number);
        if (token->token_type == R_ROUND_BRACKET) // reached ')' so all parameters are checked
        {
            AppendToken(tokens, token);
            break;
        }

        if (token->token_type == EOF_TOKEN) // reached EOF without ')'
        {
            DestroyToken(token);
            DestroySymtable(parser->symtable);
            SymtableStackDestroy(parser->symtable_stack);
            ErrorExit(ERROR_SYNTACTIC, "Didn't you forget ) at line %d ?", parser->line_number);
        }

        // id : data_type
        if (token->token_type == IDENTIFIER_TOKEN)
        {
            AppendToken(tokens, token);
            VariableSymbol *var = VariableSymbolInit();
            var->name = strdup(token->attribute);
            var->is_const = false;

            // add to symtable/check for redefinition
            if (!InsertVariableSymbol(parser->symtable, var))
            {
                PrintError("Error in semantic analysis: Line %d: Multiple parameters with name '%s' in function '%s'",
                           parser->line_number, var->name, func->name);

                // free resources
                SymtableStackDestroy(parser->symtable_stack);
                DestroyVariableSymbol(var);

                exit(ERROR_SEMANTIC_REDEFINED);
            }

            // TODO: possible var leak (???? Explain pls)
            token = CheckAndReturnToken(parser, COLON_TOKEN);
            AppendToken(tokens, token);

            if ((token = GetNextToken(&parser->line_number))->token_type != KEYWORD && (token->keyword_type != I32 || token->keyword_type != F64 || token->keyword_type != U8))
            {
                DestroyToken(token);
                DestroyVariableSymbol(var);
                DestroySymtable(parser->symtable);
                ErrorExit(ERROR_SYNTACTIC, "Expected data type at line %d", parser->line_number);
            }

            AppendToken(tokens, token);

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
            if (!func->was_called)
            {
                func->parameters = realloc(func->parameters, (param_count + 1) * sizeof(VariableSymbol *));
                func->parameters[param_count++] = VariableSymbolCopy(var); // To avoid double frees since the symbol would be in the local and global symtable both
            }

            // checks if there is another parameter
            if ((token = GetNextToken(&parser->line_number))->token_type != COMMA_TOKEN)
            {
                // no more parameters
                if (token->token_type == R_ROUND_BRACKET)
                {
                    AppendToken(tokens, token);
                    break;
                }
                DestroyToken(token);
                DestroySymtable(parser->symtable);
                SymtableStackDestroy(parser->symtable_stack);
                ErrorExit(ERROR_SYNTACTIC, "Expected ',' or ')' at line %d", parser->line_number);
            }
            AppendToken(tokens, token);
        }
        else
        {
            DestroyToken(token);
            DestroySymtable(parser->symtable);
            SymtableStackDestroy(parser->symtable_stack);
            ErrorExit(ERROR_SYNTACTIC, "Expected identifier at line %d", parser->line_number);
        }
    }

    func->num_of_parameters = param_count;
}

void ParseFunctions(Parser *parser)
{
    // The token vector to store the tokens
    tokens = InitTokenVector();

    // Get the line number of the first token
    Token *token = GetNextToken(&parser->line_number);
    first_token_line = parser->line_number;
    UngetToken(token);

    while ((token = GetNextToken(&parser->line_number))->token_type != EOF_TOKEN)
    {
        // Reserve the new token
        AppendToken(tokens, token);

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
                DestroyTokenVector(tokens);
                ErrorExit(ERROR_SEMANTIC_OTHER, "Line %d: Function definition cannot be nested inside another block!!!",
                          parser->line_number);
            }

            // Increment due to the '{' at the end after the function definition to avoid the previous error
            parser->nested_level++;
        }
    }

    DestroyToken(token); // Destroy the EOF token
}

void UngetStream(Parser *parser)
{
    int current_line = parser->line_number;
    for (int i = tokens->length - 1; i >= 0; i--)
    {
        while(current_line != tokens->token_string[i]->line_number)
        {
            current_line--;
            ungetc('\n', stdin);
        }

        current_line = tokens->token_string[i]->line_number;
        UngetToken(tokens->token_string[i]);
        tokens->token_string[i] = NULL;
        if(i != 0) ungetc(' ', stdin); // So that tokens don't get meshed together
    }



    DestroyTokenVector(tokens);
    parser->line_number = first_token_line;
}