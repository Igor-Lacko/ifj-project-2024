#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shared.h"
#include "scanner.h"
#include "error.h"
#include "core_parser.h"
#include "symtable.h"
#include "codegen.h"
#include "embedded_functions.h"
#include "expression_parser.h"
#include "function_parser.h"
#include "vector.h"
#include "stack.h"
#include "loop.h"
#include "conditionals.h"

Parser InitParser()
{
    Parser parser =
        {
            .current_function = NULL,
            .global_symtable = InitSymtable(TABLE_COUNT),
            .has_main = false,
            .end_of_program = false,
            .line_number = 1,
            .nested_level = 0,
            .symtable = InitSymtable(TABLE_COUNT),
            .symtable_stack = SymtableStackInit(),
            .parsing_functions = true};

    // Push the initial symtable to the top of the stack and add embedded functions to the global one
    SymtableStackPush(parser.symtable_stack, parser.symtable);
    InsertEmbeddedFunctions(&parser);

    return parser;
}

Token *GetNextToken(Parser *parser)
{
    if (stream_index >= stream->length)
    {
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        DestroyTokenVector(stream);
        ErrorExit(ERROR_INTERNAL, "Calling GetNextToken out of bounds. Fix your code!!!");
    }

    Token *token = stream->token_string[stream_index++];
    parser->line_number = token->line_number;
    return token;
}

void ProgramBegin()
{
    // initial codegen instructions
    IFJCODE24
    InitRegisters(); // registers exist in main
    JUMP("main")
}

// checks if the next token is of the expected type
void CheckTokenTypeStream(Parser *parser, TOKEN_TYPE type)
{
    Token *token;
    if ((token = LoadTokenFromStream(&parser->line_number))->token_type != type)
    {
        DestroyToken(token);
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        DestroyTokenVector(stream);
        ErrorExit(ERROR_SYNTACTIC, " Expected '%s' at line %d",
                  token_types[type], parser->line_number);
    }

    else AppendToken(stream, token);
}

// checks if the next token is of the expected keyword type
void CheckKeywordTypeStream(Parser *parser, KEYWORD_TYPE type)
{
    Token *token;
    if ((token = LoadTokenFromStream(&parser->line_number))->keyword_type != type)
    {
        DestroyToken(token);
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        DestroyTokenVector(stream);
        ErrorExit(ERROR_SYNTACTIC, "Expected '%s' keyword at line %d",
                  keyword_types[type], parser->line_number);
    }

    else AppendToken(stream, token);
}

// checks if the next token is of the expected type and returns it
Token *CheckAndReturnTokenStream(Parser *parser, TOKEN_TYPE type)
{
    Token *token;
    if ((token = LoadTokenFromStream(&parser->line_number))->token_type != type)
    {
        DestroyToken(token);
        DestroyTokenVector(stream);
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        ErrorExit(ERROR_SYNTACTIC, " Expected '%s' at line %d",
                  token_types[type], parser->line_number);
    }
    AppendToken(stream, token);
    return token;
}

Token *CheckAndReturnKeywordStream(Parser *parser, KEYWORD_TYPE type)
{
    Token *token;
    if ((token = LoadTokenFromStream(&parser->line_number))->keyword_type != type)
    {
        DestroyToken(token);
        DestroyTokenVector(stream);
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        ErrorExit(ERROR_SYNTACTIC, "Expected '%s' keyword at line %d",
                  keyword_types[type], parser->line_number);
    }
    AppendToken(stream, token);
    return token;
}

void CheckTokenTypeVector(Parser *parser, TOKEN_TYPE type)
{
    Token *token;
        if ((token = GetNextToken(parser))->token_type != type)
    {
        DestroyTokenVector(stream);
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        ErrorExit(ERROR_SYNTACTIC, " Expected '%s' at line %d",
                  token_types[type], parser->line_number);
    }
}

void CheckKeywordTypeVector(Parser *parser, KEYWORD_TYPE type)
{
    Token *token;
    if ((token = GetNextToken(parser))->keyword_type != type)
    {
        DestroyTokenVector(stream);
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        ErrorExit(ERROR_SYNTACTIC, "Expected '%s' keyword at line %d",
                  keyword_types[type], parser->line_number);
    }
}

Token *CheckAndReturnTokenVector(Parser *parser, TOKEN_TYPE type)
{
    Token *token;
    if ((token = GetNextToken(parser))->token_type != type)
    {
        DestroyTokenVector(stream);
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        ErrorExit(ERROR_SYNTACTIC, " Expected '%s' at line %d",
                  token_types[type], parser->line_number);
    }
    return token;
}

Token *CheckAndReturnKeywordVector(Parser *parser, KEYWORD_TYPE type)
{
    Token *token;
    if ((token = GetNextToken(parser))->keyword_type != type)
    {
        DestroyTokenVector(stream);
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        ErrorExit(ERROR_SYNTACTIC, "Expected '%s' keyword at line %d",
                  keyword_types[type], parser->line_number);
    }
    return token;
}

VariableSymbol *IsVariableAssignment(Token *token, Parser *parser)
{
    VariableSymbol *var;

    // Not a function, and a recognized variable
    if (FindFunctionSymbol(parser->global_symtable, token->attribute) == NULL && (var = SymtableStackFindVariable(parser->symtable_stack, token->attribute)) != NULL)
    {
        // The next token has to be an '=' operator, a variable by itself is not an expression
        CheckTokenTypeVector(parser, ASSIGNMENT);
        stream_index-=2;
        return var;
    }

    return NULL;
}

bool IsFunctionCall(Parser *parser)
{
    stream_index--;
    Token *id = GetNextToken(parser);
    PrintToken(id);
    Token *braces = GetNextToken(parser);
    if (id->token_type == IDENTIFIER_TOKEN && braces->token_type == L_ROUND_BRACKET)
    {
        stream_index-=2;
        return true;
    }

    stream_index-=2;
    return false;
}

void PrintStreamTokens(Parser *parser)
{
    for(int i = 0; i < stream->length; i++)
    {
        PrintToken(stream->token_string[i]);
    }
    //DestroyTokenVector(stream);
    SymtableStackDestroy(parser->symtable_stack);
    DestroySymtable(parser->global_symtable);
    DestroyTokenVector(stream);
    exit(SUCCESS);
}

// const ifj = @import("ifj24.zig");
void Header(Parser *parser)
{
    CheckKeywordTypeVector(parser, CONST);
    CheckTokenTypeVector(parser, IDENTIFIER_TOKEN);
    CheckTokenTypeVector(parser, ASSIGNMENT);
    CheckTokenTypeVector(parser, IMPORT_TOKEN);
    CheckTokenTypeVector(parser, L_ROUND_BRACKET);
    CheckTokenTypeVector(parser, LITERAL_TOKEN);
    CheckTokenTypeVector(parser, R_ROUND_BRACKET);
    CheckTokenTypeVector(parser, SEMICOLON);
}

// if(expression)|id|{} else{}
// if(expression){}else{}
void IfElse(Parser *parser)
{
    Symtable *symtable = InitSymtable(TABLE_COUNT);
    SymtableStackPush(parser->symtable_stack, symtable);
    parser->symtable = symtable;

    if(!IsIfNullableType(parser))
        ParseIfStatement(parser);

    else ParseNullableIfStatement(parser);

    parser->nested_level--;
}

void WhileLoop(Parser *parser)
{
    Symtable *symtable = InitSymtable(TABLE_COUNT);
    SymtableStackPush(parser->symtable_stack, symtable);
    parser->symtable = symtable;

    if(!IsLoopNullableType(parser))
        ParseWhileLoop(parser);


    else ParseNullableWhileLoop(parser);

    parser->nested_level--;
}

// const/var id = expression;
void VarDeclaration(Parser *parser, bool is_const)
{
    Token *token;
    token = CheckAndReturnTokenVector(parser, IDENTIFIER_TOKEN);

    // add to symtable
    VariableSymbol *var = VariableSymbolInit();
    var->name = strdup(token->attribute);
    var->is_const = is_const;
    var->type = VOID_TYPE;

    // Define a variable in IFJCode24
    DefineVariable(var->name, LOCAL_FRAME);

    // check if the variable is already declared in the current symtable
    VariableSymbol *var_in_stack = FindVariableSymbol(parser->symtable, var->name);

    if (var_in_stack != NULL)
    {
        ErrorExit(ERROR_SEMANTIC_REDEFINED, "Variable %s already declared", var->name);
    }

    // insert into symtable on top of the stack
    InsertVariableSymbol(SymtableStackTop(parser->symtable_stack), var);


    token = GetNextToken(parser);

    if (token->token_type != ASSIGNMENT && token->token_type != COLON_TOKEN)
    {
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->symtable);
        DestroyTokenVector(stream);
        ErrorExit(ERROR_SYNTACTIC, "Expected '=' or ':' at line %d", parser->line_number);
    }

    // const/var a : i32 = 5;
    if (token->token_type == COLON_TOKEN)
    {
        // data type
        token = GetNextToken(parser);
        if (token->token_type != KEYWORD || (token->keyword_type != I32 && token->keyword_type != F64 && token->keyword_type != U8))
        {
            DestroyTokenVector(stream);
            DestroySymtable(parser->symtable);
            SymtableStackDestroy(parser->symtable_stack);
            ErrorExit(ERROR_SYNTACTIC, "Expected data type at line %d", parser->line_number);
        }
        var->type = token->keyword_type == I32 ? INT32_TYPE : token->keyword_type == F64 ? DOUBLE64_TYPE
                                                                                         : U8_ARRAY_TYPE;

        CheckTokenTypeVector(parser, ASSIGNMENT);
    }

    else if (token->token_type != ASSIGNMENT)
    {
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        DestroyTokenVector(stream);
        ErrorExit(ERROR_SYNTACTIC, "Expected '=' at line %d", parser->line_number);
    }

    // Assign to the variable
    VariableAssignment(parser, var);
}

// TODO: revamp this to work accordingly with function_parser.c
void FunctionCall(Parser *parser, FunctionSymbol *func, const char *fun_name, DATA_TYPE expected_return)
{
    // Invalid return value
    if (func->return_type != expected_return)
    {
        PrintError("Line %d: Invalid return type for function \"%s\"",
                   parser->line_number, fun_name);
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
    }

    CREATEFRAME
    ParametersOnCall(parser, func);
    FUNCTIONCALL(func->name)
<<<<<<< Updated upstream:src/core_parser.c
    CheckTokenType(parser, SEMICOLON);
=======
    func->was_called = true;

    POPFRAME
>>>>>>> Stashed changes:src/parser.c
}

void ParametersOnCall(Parser *parser, FunctionSymbol *func)
{
    fprintf(stderr, "Function %s has %d parameters\n", func->name, func->num_of_parameters);
    Token *token;
    int loaded = 0;
    VariableSymbol *symb1; // for identifier parameter checking
    bool break_flag = false;

    // Main loop
    while((token=GetNextToken(parser))->token_type != R_ROUND_BRACKET)
    {
        switch(token->token_type)
        {
            // Basically the same handling for all of these, check if the type matches and generate code
            case INTEGER_32: case DOUBLE_64: case LITERAL_TOKEN:
                // Also check if the count isn't too many
                if(loaded >= func->num_of_parameters) INVALID_PARAM_COUNT

                // Get the data type depending on the token type
                DATA_TYPE type_got = token->token_type == INTEGER_32 ? INT32_TYPE : token->token_type == DOUBLE_64 ? DOUBLE64_TYPE : U8_ARRAY_TYPE;

                // Check if the type matches
                if(!CheckParamType(func->parameters[loaded]->type, type_got)) INVALID_PARAM_TYPE

                // Everything's fine, generate code
                NEWPARAM(loaded)
                SETPARAM(loaded++, token->attribute, token->token_type, LOCAL_FRAME);

                // Check if the next token is a comma or a closing bracket
                if((token = GetNextToken(parser))->token_type != R_ROUND_BRACKET && token->token_type != COMMA_TOKEN) INVALID_PARAM_TOKEN

                // If it's a comma, we can continue the loop
                else if(token->token_type == COMMA_TOKEN) break;

                // If it's a closing bracket, we're done and we can perform checks after the loop
                else
                {
                    break_flag = true;
                    break;
                }

            // Very similar to the case above, but we have to check if the identifier is defined, access it in the symtable and check the type
            case IDENTIFIER_TOKEN:
                // Again, first check too many params case
                if(loaded >= func->num_of_parameters) INVALID_PARAM_COUNT

                // Check if the identifier is defined
                else if((symb1 = SymtableStackFindVariable(parser->symtable_stack, token->attribute)) == NULL)
                {
                    fprintf(stderr, "Error in semantic analysis: Line %d: Undefined variable '%s'\n", parser->line_number, token->attribute);
                    DestroySymtable(parser->global_symtable);
                    SymtableStackDestroy(parser->symtable_stack);
                    DestroyTokenVector(stream);
                    free(token);
                    exit(ERROR_SEMANTIC_UNDEFINED);
                }

                // Check the type of the identifier
                else if(!CheckParamType(func->parameters[loaded]->type, symb1->type)) INVALID_PARAM_TYPE

                // Generate code
                NEWPARAM(loaded)
                SETPARAM(loaded++, token->attribute, token->token_type, LOCAL_FRAME);

                // Check if the next token is a comma or a closing bracket
                if((token = GetNextToken(parser))->token_type != R_ROUND_BRACKET && token->token_type != COMMA_TOKEN) INVALID_PARAM_TOKEN

                // If it's a comma, we can continue the loop
                else if(token->token_type == COMMA_TOKEN) break;

                // If it's a closing bracket, we're done and we can perform checks after the loop
                else
                {
                    break_flag = true;
                    break;
                }

            // Invalid token
            default:
                INVALID_PARAM_TOKEN
        }
        if(break_flag) break;
    }

    // Check if we have the right amount of parameters
    if(loaded != func->num_of_parameters) INVALID_PARAM_COUNT
}

bool IsTermType(DATA_TYPE type)
{
    return (type == INT32_TYPE || type == DOUBLE64_TYPE || type == U8_ARRAY_TYPE
    || type == INT32_NULLABLE_TYPE || type == DOUBLE64_NULLABLE_TYPE || type == U8_ARRAY_NULLABLE_TYPE
    || type == TERM_TYPE);
}

bool CheckParamType(DATA_TYPE param_expected, DATA_TYPE param_got)
{
    return  (param_expected == param_got ||
            (param_expected == U8_ARRAY_NULLABLE_TYPE && param_got == U8_ARRAY_TYPE) ||
            (param_expected == INT32_NULLABLE_TYPE && param_got == INT32_TYPE) ||
            (param_expected == DOUBLE64_NULLABLE_TYPE && param_got == DOUBLE64_TYPE) ||
            (param_expected == TERM_TYPE && IsTermType(param_got)));
}

void FunctionDefinition(Parser *parser)
{
    // These next few lines should ALWAYS run succesfully, since the function parser already checks them
    CheckKeywordTypeVector(parser, FN);
    Token *token = CheckAndReturnTokenVector(parser, IDENTIFIER_TOKEN);
    FunctionSymbol *func = FindFunctionSymbol(parser->global_symtable, token->attribute);

    // Generate code for the function label
    FUNCTIONLABEL(func->name)
    if(!strcmp(func->name, "main")) CREATEFRAME
    PUSHFRAME

    // Skip all tokens until the function body begins (so after '{')
    while ((token = GetNextToken(parser))->token_type != L_CURLY_BRACKET)
        continue;

    // Create a new symtable for the function's stack frame
    Symtable *symtable = InitSymtable(TABLE_COUNT);
    SymtableStackPush(parser->symtable_stack, symtable);
    parser->symtable = symtable;

    // Add the function parameters to the symtable and define them on the local frame
    for(int i = 0; i < func->num_of_parameters; i++)
    {
        DEFPARAM(i)
        InsertVariableSymbol(symtable, VariableSymbolCopy(func->parameters[i]));
    }

    ProgramBody(parser);
}

void FunctionReturn(Parser *parser)
{
    Token *token;
    if (!parser->current_function) // In main, return exits the program
    {
        if ((token = GetNextToken(parser))->token_type != SEMICOLON)
        {
            SymtableStackDestroy(parser->symtable_stack);
            DestroySymtable(parser->symtable);
            DestroyTokenVector(stream);
            ErrorExit(ERROR_SEMANTIC_MISSING_EXPR, "Line %d: Invalid usage of \"return\" in main function (unexpected expression)");
        }
        IFJ24SUCCESS // Successful return from main = EXIT 0 (todo: evaluate this? idk how to actually generate it)
            return;
    }

    // void function case
    if (parser->current_function->return_type == VOID_TYPE)
    {
        if ((token = GetNextToken(parser))->token_type != SEMICOLON) // returning something from void function
        {
            PrintError("Line %d: Returning a value from void function \"%s\"",
                        parser->line_number, parser->current_function->name);
            SymtableStackDestroy(parser->symtable_stack);
            DestroySymtable(parser->global_symtable);
            DestroyTokenVector(stream);
            exit(ERROR_SEMANTIC_MISSING_EXPR);
        }

        else
        {
            FUNCTION_RETURN
            return;
        }
    }

    // function with a return type
    // return expression;
    else
    {
        // The returned type is now in R0/B0/F0
        TokenVector *postfix = InfixToPostfix(parser);
        DATA_TYPE expr_type = GeneratePostfixExpression(parser, postfix, NULL);

        // Invalid return type
        if(expr_type != parser->current_function->return_type)
        {
            PrintError("Error in semantic analysis: Line %d: Invalid return type for function \"%s\"",
                       parser->line_number, parser->current_function->name);
            SymtableStackDestroy(parser->symtable_stack);
            DestroySymtable(parser->global_symtable);
            DestroyTokenVector(stream);
            exit(ERROR_SEMANTIC_TYPE_COMPATIBILITY);
        }

        FUNCTION_RETURN
        return;
    }
}

DATA_TYPE NullableToNormal(DATA_TYPE type)
{
    switch(type)
    {
        case U8_ARRAY_NULLABLE_TYPE:
            return U8_ARRAY_TYPE;

        case INT32_NULLABLE_TYPE:
            return INT32_TYPE;

        case DOUBLE64_NULLABLE_TYPE:
            return DOUBLE64_TYPE;

        default:
            return type;
    }
}

void VariableAssignment(Parser *parser, VariableSymbol *var)
{
<<<<<<< Updated upstream:src/core_parser.c
    if (var->is_const && var->defined)
=======
    if (var->is_const)
>>>>>>> Stashed changes:src/parser.c
    {
        PrintError("Error in semantic analysis: Line %d: Reassignment of constant variable \"%s\"",
                   parser->line_number, var->name);
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        exit(ERROR_SEMANTIC_REDEFINED);
    }

    stream_index++; // For IsFunctionCall to work correctly
    if (IsFunctionCall(parser))
    {
        // Get the function name to use as a key into the hash table
        Token *func_name = GetNextToken(parser);
        FunctionSymbol *func = FindFunctionSymbol(parser->global_symtable, func_name->attribute); // This should always be successful
        FunctionToVariable(parser, var, func);
        return;
    }

    // Check for a embedded function call
    Token *token = GetNextToken(parser);
    if(!strcmp(token->attribute, "ifj"))
    {
        FunctionSymbol *func = IsEmbeddedFunction(parser);
        stream_index+=2; // skip the 'ifj' and the '.' token
        EmbeddedFunctionCall(parser, func, var);
        return;
    }

    // Null can also be assigned to a variable, but we have to check if it's a nullable type
    else if(token->keyword_type == NULL_TYPE)
    {
        if(!IsNullable(var->type))
        {
            PrintError("Error in semantic analysis: Line %d: Assigning NULL to non-nullable variable \"%s\"",
                        parser->line_number, var->name);
            SymtableStackDestroy(parser->symtable_stack);
            DestroySymtable(parser->global_symtable);
            DestroyTokenVector(stream);
            exit(ERROR_SEMANTIC_TYPE_COMPATIBILITY);
        }

        // Assign NULL to the variable
        MOVE(var->name, "nil@nil", false, LOCAL_FRAME);
        return;
    }

    // If the 'ifj' token is not present, move the stream back and expect an expression
    else stream_index--;

    DATA_TYPE expr_type;
    TokenVector *postfix = InfixToPostfix(parser);

    if ((expr_type = GeneratePostfixExpression(parser, postfix, var)) != var->type && var->type != VOID_TYPE)
    {
        PrintError("Error in semantic analysis: Line %d: Assigning invalid type to variable \"%s\", expected %d, got %d",
                   parser->line_number, var->name, var->type, expr_type);
=======
    // TODO: Check for function return assignment to the variable
    TokenVector *postfix = InfixToPostfix(parser);
    if (GeneratePostfixExpression(parser, postfix, var) != var->type)
    {
        PrintError("Error in semantic analysis: Line %d: Assigning invalid type to variable \"%s\"",
                   parser->line_number, var->name);
>>>>>>> Stashed changes:src/parser.c
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        exit(ERROR_SEMANTIC_TYPE_COMPATIBILITY);
    }

    // if the variable doesn't have a type yet, derive it from the expression (TODO: Add check for invalid types, etc.)
    if (var->type == VOID_TYPE)
        var->type = expr_type;

    var->defined = true;
}

void FunctionToVariable(Parser *parser, VariableSymbol *var, FunctionSymbol *func)
{
    // Function has no return value
    if (func->return_type == VOID_TYPE)
    {
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        DestroyTokenVector(stream);
        ErrorExit(ERROR_SEMANTIC_TYPE_COMPATIBILITY, "Line %d: Assigning return value of void function to variable.", parser->line_number);
    }

    // Incompatible return type
    if ( // But for example, a U8 function return to ?U8 would be valid, since the latter can have a U8 or NULL type
        func->return_type != var->type &&
        ((var->type == U8_ARRAY_NULLABLE_TYPE && func->return_type != U8_ARRAY_TYPE) ||
        (var->type == INT32_NULLABLE_TYPE && func->return_type != INT32_TYPE) ||
        (var->type == DOUBLE64_NULLABLE_TYPE && func->return_type != DOUBLE64_TYPE)) &&
        var->type != VOID_TYPE // VOID_TYPE on variable --> the variable is just being declared, so the type has to be derived from the function
    )
    {
        SymtableStackDestroy(parser->symtable_stack);
        DestroyTokenVector(stream);
        DestroySymtable(parser->global_symtable);
        ErrorExit(ERROR_SEMANTIC_TYPE_COMPATIBILITY, "Line %d: Invalid type in assigning to variable");
    }

    // Since we are expecting '(' as the next token, we need to load the params
    CheckTokenTypeVector(parser, L_ROUND_BRACKET);
    CREATEFRAME
    ParametersOnCall(parser, func);

    /*
        ---- Compatible return type or has to be derived ----
        - The return value is on top of the data stack
        - Note: Default IFJ24 doesn't have booleans, maybe expand this for the BOOLTHEN extension later?
    */
    fprintf(stdout, "CALL %s\n", func->name);
    fprintf(stdout, "POPS LF@%s\n", var->name); // TODO: run the IFJCODE24 interpreter on a code using this if it actually works
}

void ProgramBody(Parser *parser)
{
    Token *token;
    FunctionSymbol *func;
    VariableSymbol *var;
    while (true)
    {
        token = GetNextToken(parser);
        switch (token->token_type)
        {
        case KEYWORD:
            // start of function declaration, either skip it or throw an error
            if (token->keyword_type == PUB)
            {
                ++(parser->nested_level);
                FunctionDefinition(parser);
            }
            // start of if-else block
            else if (token->keyword_type == IF)
            {
                ++(parser->nested_level);
                IfElse(parser);
            }
            // start of while loop
            else if (token->keyword_type == WHILE)
            {
                ++(parser->nested_level);
                WhileLoop(parser);
            }
            else if (token->keyword_type == CONST)
            {
                VarDeclaration(parser, true);
            }
            else if (token->keyword_type == VAR)
            {
                VarDeclaration(parser, false);
            }
            else if (token->keyword_type == RETURN) // in function
            {
                FunctionReturn(parser);
            }
            else
            {
                break;
                // ErrorExit(ERROR_SYNTACTIC, "Unexpected at line %d", *line_number);
            }
            break;

        case R_CURLY_BRACKET:
            --(parser->nested_level);
            SymtableStackRemoveTop(parser->symtable_stack);
            parser->symtable = SymtableStackTop(parser->symtable_stack);
            return;

        case IDENTIFIER_TOKEN:
            // Can be a embedded function, or a defined function, or a called function, or a reassignment to a variable
            if (!strcmp(token->attribute, "ifj"))
            {
                func = IsEmbeddedFunction(parser);
                stream_index+=2; // skip the 'ifj' and the '.' token
                EmbeddedFunctionCall(parser, func, NULL); // Void type since we aren't assigning the result anywhere
            }

            else if ((var = IsVariableAssignment(token, parser)) != NULL)
            {
                // Move past the ID =
                token = GetNextToken(parser);
                token = GetNextToken(parser);

                // Variable assignment
                VariableAssignment(parser, var);
            }

            else if (IsFunctionCall(parser))
            {
                // Store the function name in a temporary variable since we will be moving the tokens forward
                char *tmp_func_name = strdup(token->attribute);

                // Move past the ID(
                stream_index+=2;

                // Function call
                FunctionCall(parser, FindFunctionSymbol(parser->global_symtable, tmp_func_name), tmp_func_name, VOID_TYPE);
                free(tmp_func_name);
            }

            break;

        case EOF_TOKEN:
            return;

        default:
            // ErrorExit(ERROR_SYNTACTIC, "Unexpected token at line %d", *line_number);
            break;
        }
    }
}

void PrintEqualTokens()
{
    for(int i = 0; i < stream->length; i++)
    {
        for(int j = i+1; j < stream->length; j++)
        {
            if(i != j && stream->token_string[i] == stream->token_string[j])
            {
                fprintf(stderr, "Equal tokens found at indexes %d and %d\n", i, j);
                PrintToken(stream->token_string[i]);
                PrintToken(stream->token_string[j]);
            }
        }
    }
}

int main()
{
    // parser instance
    Parser parser = InitParser();

    // First go-through of the stream file, add functions to the global symtable
    ParseFunctions(&parser);

    // Check for the presence of a a main function
    if (!parser.has_main)
    {
        SymtableStackDestroy(parser.symtable_stack);
        DestroySymtable(parser.global_symtable);
        DestroyTokenVector(stream);
        ErrorExit(ERROR_SEMANTIC_UNDEFINED, "Main function not found");
    }

    // Second go-through of the stream file, parse the program body
    ProgramBegin(&parser);
    Header(&parser);
    ProgramBody(&parser);

    SymtableStackDestroy(parser.symtable_stack);
    DestroySymtable(parser.global_symtable);
    DestroyTokenVector(stream);
    return 0;
}
