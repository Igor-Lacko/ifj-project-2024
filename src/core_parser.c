#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    return stream->token_string[stream_index++];
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
    char char_types[30][16] = {"Identifier", "_", "Keyword", "integer", "double", "u8", "string", "import", "=", "*", "/", "+", "-", "==", "!=", "<", ">", "<=", ">=", "(", ")", "{", "}", "|", ";", ",", ".", ":", "@", "EOF"};
    if ((token = LoadTokenFromStream(&parser->line_number))->token_type != type)
    {
        DestroyToken(token);
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        DestroyTokenVector(stream);
        ErrorExit(ERROR_SYNTACTIC, " Expected '%s' at line %d",
                  char_types[type], parser->line_number);
    }

    else AppendToken(stream, token);
}

// checks if the next token is of the expected keyword type
void CheckKeywordTypeStream(Parser *parser, KEYWORD_TYPE type)
{
    Token *token;
    char keyword_types[13][20] = {"const", "else", "fn", "if", "i32", "f64", "null", "pub", "return", "u8", "var", "void", "while"};
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
    char char_types[30][15] = {"Identifier", "_", "Keyword", "integer", "double", "u8", "string", "=", "*", "/", "+", "-", "==", "!=", "<", ">", "<=", ">=", "(", ")", "{", "}", "|", ";", ",", ".", ":", "@", "EOF"};
    if ((token = LoadTokenFromStream(&parser->line_number))->token_type != type)
    {
        DestroyToken(token);
        DestroyTokenVector(stream);
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        ErrorExit(ERROR_SYNTACTIC, " Expected '%s' at line %d",
                  char_types[type], parser->line_number);
    }
    AppendToken(stream, token);
    return token;
}

Token *CheckAndReturnKeywordStream(Parser *parser, KEYWORD_TYPE type)
{
    Token *token;
    char keyword_types[13][20] = {"const", "else", "fn", "if", "i32", "f64", "null", "pub", "return", "u8", "var", "void", "while"};
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
    char char_types[30][15] = {"Identifier", "_", "Keyword", "integer", "double", "u8", "string", "=", "*", "/", "+", "-", "==", "!=", "<", ">", "<=", ">=", "(", ")", "{", "}", "|", ";", ",", ".", ":", "@", "EOF"};
    if ((token = GetNextToken(parser))->token_type != type)
    {
        DestroyTokenVector(stream);
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        ErrorExit(ERROR_SYNTACTIC, " Expected '%s' at line %d",
                  char_types[type], parser->line_number);
    }
}

void CheckKeywordTypeVector(Parser *parser, KEYWORD_TYPE type)
{
    Token *token;
    char keyword_types[13][20] = {"const", "else", "fn", "if", "i32", "f64", "null", "pub", "return", "u8", "var", "void", "while"};
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
    char char_types[30][15] = {"Identifier", "_", "Keyword", "integer", "double", "u8", "string", "=", "*", "/", "+", "-", "==", "!=", "<", ">", "<=", ">=", "(", ")", "{", "}", "|", ";", ",", ".", ":", "@", "EOF"};
    if ((token = GetNextToken(parser))->token_type != type)
    {
        DestroyTokenVector(stream);
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        ErrorExit(ERROR_SYNTACTIC, " Expected '%s' at line %d",
                  char_types[type], parser->line_number);
    }
    return token;
}

Token *CheckAndReturnKeywordVector(Parser *parser, KEYWORD_TYPE type)
{
    Token *token;
    char keyword_types[13][20] = {"const", "else", "fn", "if", "i32", "f64", "null", "pub", "return", "u8", "var", "void", "while"};
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
        CheckTokenTypeStream(parser, ASSIGNMENT);
        stream_index-=2;
        return var;
    }

    return NULL;
}

bool IsFunctionCall(Parser *parser)
{
    Token *braces = GetNextToken(parser);
    if (braces->token_type == L_ROUND_BRACKET)
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
    CheckKeywordTypeStream(parser, CONST);
    CheckTokenTypeStream(parser, IDENTIFIER_TOKEN);
    CheckTokenTypeStream(parser, ASSIGNMENT);
    CheckTokenTypeStream(parser, IMPORT_TOKEN);
    CheckTokenTypeStream(parser, L_ROUND_BRACKET);
    CheckTokenTypeStream(parser, LITERAL_TOKEN);
    CheckTokenTypeStream(parser, R_ROUND_BRACKET);
    CheckTokenTypeStream(parser, SEMICOLON);
}

// if(expression)|id|{} else{}
// if(expression){}else{}
void IfElse(Parser *parser)
{
    IfLabel(LOCAL_FRAME);

    Symtable *symtable = InitSymtable(TABLE_COUNT);
    SymtableStackPush(parser->symtable_stack, symtable);
    parser->symtable = symtable;

    CheckTokenTypeVector(parser, L_ROUND_BRACKET);
    // expression
    TokenVector *postfix = InfixToPostfix(parser);
    (void)postfix; // Can't handle expressions yet
    CheckTokenTypeVector(parser, R_ROUND_BRACKET);

    Token *token = GetNextToken(parser);
    if (token->token_type == L_CURLY_BRACKET)
    {
        ProgramBody(parser); // if block
        CheckTokenTypeVector(parser, R_CURLY_BRACKET);
        CheckKeywordTypeVector(parser, ELSE);
        CheckTokenTypeVector(parser, L_CURLY_BRACKET);
        ProgramBody(parser); // else block
        CheckTokenTypeVector(parser, R_CURLY_BRACKET);
    }

    else if (token->token_type == VERTICAL_BAR_TOKEN)
    {
        CheckTokenTypeVector(parser, VERTICAL_BAR_TOKEN);
        Token *id = CheckAndReturnTokenVector(parser, IDENTIFIER_TOKEN);
        VariableSymbol *var = SymtableStackFindVariable(parser->symtable_stack, id->attribute);
        if (var != NULL)
        {
            SymtableStackDestroy(parser->symtable_stack);
            ErrorExit(ERROR_SEMANTIC_REDEFINED, "Line %d: redefined variable");
        }
    }

    else
    {
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        DestroyTokenVector(stream);
        ErrorExit(ERROR_SYNTACTIC, "Expected '{' or '|' in if block on line %d", parser->line_number);
    }

    // If block
    ProgramBody(parser);
    CheckTokenTypeVector(parser, R_CURLY_BRACKET);

    // Else block
    CheckKeywordTypeVector(parser, ELSE);
    ElseLabel(LOCAL_FRAME);
    CheckTokenTypeVector(parser, L_CURLY_BRACKET);
    ProgramBody(parser);

    // If-Else finish
    CheckTokenTypeVector(parser, R_CURLY_BRACKET);
    EndIfLabel(LOCAL_FRAME);
}

void WhileLoop(Parser *parser)
{
    Symtable *symtable = InitSymtable(TABLE_COUNT);
    SymtableStackPush(parser->symtable_stack, symtable);
    parser->symtable = symtable;

    CheckTokenTypeVector(parser, L_ROUND_BRACKET);
    // expression
    TokenVector *postfix = InfixToPostfix(parser);
    GeneratePostfixExpression(parser, postfix, NULL);
    CheckTokenTypeVector(parser, R_ROUND_BRACKET);

    CheckTokenTypeVector(parser, L_CURLY_BRACKET);
    ProgramBody(parser);
    CheckTokenTypeVector(parser, R_CURLY_BRACKET);
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
            DestroyToken(token);
            DestroySymtable(parser->symtable);
            ErrorExit(ERROR_SYNTACTIC, "Expected data type at line %d", parser->line_number);
        }
        var->type = token->keyword_type == I32 ? INT32_TYPE : token->keyword_type == F64 ? DOUBLE64_TYPE
                                                                                         : U8_ARRAY_TYPE;

        CheckTokenTypeStream(parser, ASSIGNMENT);
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

    CheckTokenTypeVector(parser, L_ROUND_BRACKET);
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
    Token *token;
    int loaded = 0;
    VariableSymbol *symb1; // for identifier parameter checking

    // Load all parameters
    while ((token = GetNextToken(parser))->token_type != R_ROUND_BRACKET)
    {
        if(loaded == func->num_of_parameters) break; // Invalid count checking after the loop is done
        switch (token->token_type)
        {
            // Variable as a parameter
            case IDENTIFIER_TOKEN:
                symb1 = SymtableStackFindVariable(parser->symtable_stack, token->attribute);
                if (symb1 == NULL)
                {
<<<<<<< Updated upstream:src/core_parser.c
                    PrintError("Error in semantic analysis: Line %d: Undefined variable \"%s\"",
                                parser->line_number, token->attribute);
                    SymtableStackDestroy(parser->symtable_stack);
                    DestroySymtable(parser->global_symtable);
                    DestroyTokenVector(stream);
                    exit(ERROR_SEMANTIC_UNDEFINED);
=======
                    if (!strcmp("write", func->name) && func->return_type == TERM_TYPE) // Special case: ifj.write function
                        CHECK_PARAM(INT32_TYPE, func->parameters[loaded - 1]->type)

                    SETPARAM(loaded, token->attribute)
>>>>>>> Stashed changes:src/parser.c
                }

                // Check if the parameter type is valid
                if(!CheckParamType(func->parameters[loaded]->type, symb1->type))
                    INVALID_PARAM_TYPE

                NEWPARAM(loaded)
                SETPARAM(loaded++, token->attribute, token->token_type, LOCAL_FRAME);

                break;

            case INTEGER_32:
                if(CheckParamType(func->parameters[loaded]->type, INT32_TYPE))
                {
<<<<<<< Updated upstream:src/core_parser.c
                    NEWPARAM(loaded)
                    SETPARAM(loaded++, token->attribute, token->token_type, LOCAL_FRAME);
=======
                    if (!strcmp("write", func->name) && func->return_type == TERM_TYPE)
                        CHECK_PARAM(DOUBLE64_TYPE, func->parameters[loaded - 1]->type)

                    SETPARAM(loaded, token->attribute)
>>>>>>> Stashed changes:src/parser.c
                }
                else
                    INVALID_PARAM_TYPE

                break;

            case DOUBLE_64:
                if(CheckParamType(func->parameters[loaded]->type, DOUBLE64_TYPE))
                {
<<<<<<< Updated upstream:src/core_parser.c
                    NEWPARAM(loaded)
                    SETPARAM(loaded++, token->attribute, token->token_type, LOCAL_FRAME);
=======
                    if (!strcmp("write", func->name) && func->return_type == TERM_TYPE)
                        CHECK_PARAM(U8_ARRAY_TYPE, func->parameters[loaded - 1]->type)

                    SETPARAM(loaded, token->attribute)
>>>>>>> Stashed changes:src/parser.c
                }
                else
                    INVALID_PARAM_TYPE

                break;

            case LITERAL_TOKEN:
                if(CheckParamType(func->parameters[loaded]->type, U8_ARRAY_TYPE))
                {
                    NEWPARAM(loaded)
                    SETPARAM(loaded++, token->attribute, token->token_type, LOCAL_FRAME);
                }
                else
                    INVALID_PARAM_TYPE

                break;

            // Any other token type is a syntax error
            default:
                PrintError("Error in syntactic analysis: Line %d: Unexpected token \"%s\" in function call",
                           parser->line_number, token->attribute);
                DestroyTokenVector(stream);
                SymtableStackDestroy(parser->symtable_stack);
                DestroySymtable(parser->global_symtable);
                exit(ERROR_SYNTACTIC);
        }

        // In the case of the last parameter, check if the next token is a comma/')'
        if(loaded == func->num_of_parameters)
        {
            if((token=GetNextToken(parser))->token_type != R_ROUND_BRACKET && token->token_type != COMMA_TOKEN)
                INVALID_PARAM_COUNT

            else if(token->token_type == COMMA_TOKEN)
            {
<<<<<<< Updated upstream:src/core_parser.c
                DestroyToken(token);
                CheckTokenType(parser, R_ROUND_BRACKET);
                break;
=======
                if (func->was_defined)
                {
                    if ((loaded) <= func->num_of_parameters)
                    {
                        if (!strcmp("write", func->name) && func->return_type == TERM_TYPE)
                            CHECK_PARAM(symb1->type, func->parameters[loaded - 1]->type)

                        SETPARAM(loaded, token->attribute)
                    }

                    else
                        INVALID_PARAM_COUNT
                }

                else
                {
                    LENGHTEN_PARAMS
                    VariableSymbol *param = VariableSymbolInit();
                    param->type = symb1->type;
                    func->parameters[loaded - 1] = param;

                    // Allocate a new string for the var name
                    char *name = malloc(strlen(token->attribute) + 4); // + LF@ and '\0'
                    if (name == NULL)
                    {
                        SymtableStackDestroy(parser->symtable_stack);
                        DestroySymtable(parser->global_symtable);
                        DestroyToken(token);
                        ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
                    }

                    sprintf(name, "LF@%s", token->attribute);
                    SETPARAM(loaded, name)
                    free(name);
                }
>>>>>>> Stashed changes:src/parser.c
            }

            else break;
        }
    }

    // Check if the number of parameters isn't lower than the function expects
    if(loaded < func->num_of_parameters)
        INVALID_PARAM_COUNT
}

bool IsTermType(DATA_TYPE type)
{
    return (type == INT32_TYPE || type == DOUBLE64_TYPE || type == U8_ARRAY_TYPE
    || type == INT32_NULLABLE_TYPE || type == DOUBLE64_NULLABLE_TYPE || type == U8_ARRAY_NULLABLE_TYPE
    || type == TERM_TYPE);
}

bool CheckParamType(DATA_TYPE param_expected, DATA_TYPE param_got)
{
    return (param_expected == param_got ||
            (param_expected == U8_ARRAY_NULLABLE_TYPE && param_got == U8_ARRAY_TYPE) ||
            (param_expected == INT32_NULLABLE_TYPE && param_got == INT32_TYPE) ||
            (param_expected == DOUBLE64_NULLABLE_TYPE && param_got == DOUBLE64_TYPE) ||
            (param_expected == TERM_TYPE && IsTermType(param_got)));
}

void FunctionDefinition(Parser *parser)
{
    // These next few lines should ALWAYS run succesfully, since the function parser already checks them
    CheckKeywordTypeStream(parser, FN);
    Token *token = CheckAndReturnTokenStream(parser, IDENTIFIER_TOKEN);
    FunctionSymbol *func = FindFunctionSymbol(parser->global_symtable, token->attribute);

    // Generate code for the function label
    FUNCTIONLABEL(func->name)
    PUSHFRAME

    // Skip all tokens until the function body begins (so after '{')
    while ((token = GetNextToken(parser))->token_type != L_CURLY_BRACKET)
        continue;

    // Create a new symtable for the function's stack frame
    Symtable *symtable = InitSymtable(TABLE_COUNT);
    SymtableStackPush(parser->symtable_stack, symtable);
    parser->symtable = symtable;

    // Add the function parameters to the symtable
    for(int i = 0; i < func->num_of_parameters; i++)
        InsertVariableSymbol(symtable, VariableSymbolCopy(func->parameters[i]));

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
        IFJ24SUCCESS // Successful return from main = EXIT 0
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
    else
    {
        VariableSymbol *var = VariableSymbolInit();
        if ((var->name = malloc(snprintf(NULL, 0, "LF@%sRETURN", parser->current_function->name) + 1)) == NULL)
        {
            SymtableStackDestroy(parser->symtable_stack);
            DestroySymtable(parser->global_symtable);
            DestroyTokenVector(stream);
            ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
        }
        sprintf(var->name, "GF@%sRETURN", parser->current_function->name);
        TokenVector *postfix = InfixToPostfix(parser);
        DATA_TYPE return_type = GeneratePostfixExpression(parser, postfix, var);
        (void)return_type; // STFU gcc

        // TODO: give it to the variable
        DestroyVariableSymbol(var);
        return;
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

<<<<<<< Updated upstream:src/core_parser.c
    if (IsFunctionCall(GetNextToken(&parser->line_number), parser))
    {
        // Get the function name to use as a key into the hash table
        Token *func_name = GetNextToken(parser);
        FunctionSymbol *func = FindFunctionSymbol(parser->global_symtable, func_name->attribute); // This should always be successful
        FunctionToVariable(parser, var, func);
        return;
    }

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

    CheckTokenTypeStream(parser, SEMICOLON);
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
    CheckTokenTypeStream(parser, L_ROUND_BRACKET);
    CREATEFRAME
    ParametersOnCall(parser, func);

    /*
        ---- Compatible return type or has to be derived ----
        - The return value is on top of the data stack
        - Note: Default IFJ24 doesn't have booleans, maybe expand this for the BOOLTHEN extension later?
    */
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
            ungetc('}', stdin);
            return;

        case IDENTIFIER_TOKEN:
            // Can be a embedded function, or a defined function, or a called function, or a reassignment to a variable
            if (!strcmp(token->attribute, "ifj"))
            {
                func = IsEmbeddedFunction(parser);
                FunctionCall(parser, func, func->name, VOID_TYPE); // Void type since we aren't assigning the result anywhere
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
                token = GetNextToken(parser);
                token = GetNextToken(parser);

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
    ProgramBegin(&parser);
    ParseFunctions(&parser);
    PrintStreamTokens(&parser);
    exit(0);


    // check if the main function is present
    if (!parser.has_main)
    {
        SymtableStackDestroy(parser.symtable_stack);
        DestroySymtable(parser.global_symtable);
        ErrorExit(ERROR_SEMANTIC_UNDEFINED, "Main function not found");
    }

    SymtableStackDestroy(parser.symtable_stack);
    DestroySymtable(parser.global_symtable);
    IFJ24SUCCESS
    return 0;
}
