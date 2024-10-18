#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scanner.h"
#include "error.h"
#include "parser.h"
#include "symtable.h"
#include "expression_parser.h"
#include "codegen.h"
#include "embedded_functions.h"

Parser InitParser()
{
    Parser parser =
        {
            .current_function = NULL,
            .global_symtable = InitSymtable(TABLE_COUNT),
            .has_main = false,
            .in_function = false,
            .line_number = 1,
            .nested_level = 0,
            .symtable = InitSymtable(TABLE_COUNT),
            .symtable_stack = SymtableStackInit()};

    // Push the initial symtable to the top of the stack and add embedded functions to the global one
    SymtableStackPush(parser.symtable_stack, parser.symtable);
    InsertEmbeddedFunctions(&parser);

    return parser;
}

void ProgramBegin(Parser *parser)
{
    // initial codegen instructions
    IFJCODE24
    InitRegisters();
    Jump("main", GLOBAL_FRAME);
    Header(parser);
}

// checks if the next token is of the expected type
void CheckTokenType(Parser *parser, TOKEN_TYPE type)
{
    Token *token;
    char char_types[30][16] = {"Identifier", "_", "Keyword", "integer", "double", "u8", "string", "import", "=", "*", "/", "+", "-", "==", "!=", "<", ">", "<=", ">=", "(", ")", "{", "}", "|", ";", ",", ".", ":", "@", "EOF"};
    if ((token = GetNextToken(&parser->line_number))->token_type != type)
    {
        DestroyToken(token);
        SymtableStackDestroy(parser->symtable_stack);
        ErrorExit(ERROR_SYNTACTIC, " Expected '%s' at line %d",
                  char_types[type], parser->line_number);
    }
    DestroyToken(token);
}

// checks if the next token is of the expected keyword type
void CheckKeywordType(Parser *parser, KEYWORD_TYPE type)
{
    Token *token;
    char keyword_types[13][20] = {"const", "else", "fn", "if", "i32", "f64", "null", "pub", "return", "u8", "var", "void", "while"};
    if ((token = GetNextToken(&parser->line_number))->keyword_type != type)
    {
        DestroyToken(token);
        SymtableStackDestroy(parser->symtable_stack);
        ErrorExit(ERROR_SYNTACTIC, "Expected '%s' keyword at line %d",
                  keyword_types[type], parser->line_number);
    }
    DestroyToken(token);
}

// checks if the next token is of the expected type and returns it
Token *CheckAndReturnToken(Parser *parser, TOKEN_TYPE type)
{
    Token *token;
    char char_types[30][15] = {"Identifier", "_", "Keyword", "integer", "double", "u8", "string", "=", "*", "/", "+", "-", "==", "!=", "<", ">", "<=", ">=", "(", ")", "{", "}", "|", ";", ",", ".", ":", "@", "EOF"};
    if ((token = GetNextToken(&parser->line_number))->token_type != type)
    {
        DestroyToken(token);
        SymtableStackDestroy(parser->symtable_stack);
        ErrorExit(ERROR_SYNTACTIC, " Expected '%s' at line %d",
                  char_types[type], parser->line_number);
    }
    return token;
}

// const ifj = @import("ifj24.zig");
void Header(Parser *parser)
{
    CheckKeywordType(parser, CONST);
    CheckTokenType(parser, IDENTIFIER_TOKEN);
    CheckTokenType(parser, ASSIGNMENT);
    CheckTokenType(parser, IMPORT_TOKEN);
    CheckTokenType(parser, L_ROUND_BRACKET);
    CheckTokenType(parser, LITERAL_TOKEN);
    CheckTokenType(parser, R_ROUND_BRACKET);
    CheckTokenType(parser, SEMICOLON);
}

void Expression(Parser *parser)
{
    Token *token;
    int bracket_count = 0;
    while (1)
    {
        token = GetNextToken(&parser->line_number);

        // ; closing the expression
        if (token->token_type == SEMICOLON)
        {
            DestroyToken(token);
            ungetc(';', stdin);
            return;
        }

        // if the expression is not closed
        if (token->token_type == EOF_TOKEN)
        {
            DestroyToken(token);
            DestroySymtable(parser->symtable);
            ErrorExit(ERROR_SYNTACTIC, "Expected ';' at line %d", parser->line_number);
        }

        if (token->token_type == L_ROUND_BRACKET)
            bracket_count++;
        else if (token->token_type == R_ROUND_BRACKET)
            bracket_count--;

        // | closing the expression
        if (token->token_type == VERTICAL_BAR_TOKEN)
        {
            DestroyToken(token);
            ungetc('|', stdin);
            return;
        }

        // ) closing the expression
        if (bracket_count < 0)
        {
            DestroyToken(token);
            ungetc(')', stdin);
            return;
        }

        // TODO: complete expressions, for now just skip
        DestroyToken(token);
    }
    DestroyToken(token);
}

// checks all parameters
void ParametersDefinition(Parser *parser, FunctionSymbol *func)
{
    Token *token;
    int param_count = 0;

    // loops through all parameters
    while (1)
    {
        token = GetNextToken(&parser->line_number);
        if (token->token_type == R_ROUND_BRACKET) // reached ')' so all parameters are checked
            break;

        if (token->token_type == EOF_TOKEN) // reached EOF without ')'
        {
            DestroyToken(token);
            DestroySymtable(parser->symtable);
            ErrorExit(ERROR_SYNTACTIC, "Didn't you forget ) at line %d ?", parser->line_number);
        }

        // id : data_type
        if (token->token_type == IDENTIFIER_TOKEN)
        {
            VariableSymbol *var = VariableSymbolInit();
            var->name = strdup(token->attribute);
            var->is_const = false;

            DestroyToken(token);

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
            CheckTokenType(parser, COLON_TOKEN);

            if ((token = GetNextToken(&parser->line_number))->token_type != KEYWORD && (token->keyword_type != I32 || token->keyword_type != F64 || token->keyword_type != U8))
            {
                DestroyToken(token);
                DestroyVariableSymbol(var);
                DestroySymtable(parser->symtable);
                ErrorExit(ERROR_SYNTACTIC, "Expected data type at line %d", parser->line_number);
            }

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
            DestroyToken(token);

            // add parameter to the function symbol
            if (!func->was_called)
            {
                func->parameters = realloc(func->parameters, (param_count + 1) * sizeof(VariableSymbol *));
                func->parameters[param_count++] = var;
            }

            else
            {
                CHECK_PARAM(var->type, func->parameters[param_count++]->type)
            }

            // checks if there is another parameter
            if ((token = GetNextToken(&parser->line_number))->token_type != COMMA_TOKEN)
            {
                if (token->token_type == R_ROUND_BRACKET) // no more parameters
                    break;
                DestroyToken(token);
                DestroySymtable(parser->symtable);
                ErrorExit(ERROR_SYNTACTIC, "Expected ',' or ')' at line %d", parser->line_number);
            }
            DestroyToken(token);
        }
        else
        {
            DestroyToken(token);
            DestroySymtable(parser->symtable);
            ErrorExit(ERROR_SYNTACTIC, "Expected identifier at line %d", parser->line_number);
        }
    }
    if (func->was_called && param_count != func->num_of_parameters)
    {
        PrintError("Invalid number of parameters for function \"%s\": Given %d, expected %d",
                   func->name, func->num_of_parameters, param_count);
        DestroyToken(token);
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        exit(ERROR_SEMANTIC_TYPECOUNT_FUNCTION);
    }
    func->num_of_parameters = param_count;
    DestroyToken(token);
}

// pub fn id ( seznam_parametrů ) návratový_typ {
// sekvence_příkazů
// }
void FunctionDefinition(Parser *parser)
{
    CheckKeywordType(parser, FN);
    Token *token;
    token = CheckAndReturnToken(parser, IDENTIFIER_TOKEN);

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
        if (func->was_defined)
        {
            PrintError("Error in semantic analysis: Line %d: Redefinition of function %s",
                       parser->line_number, func->name);
            SymtableStackDestroy(parser->symtable_stack);
            DestroySymtable(parser->global_symtable);
            DestroyToken(token);
            exit(ERROR_SEMANTIC_REDEFINED);
        }
    }
    DestroyToken(token);

    // Create a new symtable at the top of the stack
    Symtable *local_symtable = InitSymtable(TABLE_COUNT);
    SymtableStackPush(parser->symtable_stack, local_symtable);
    parser->symtable = local_symtable;

    // Generate a function label and create a new temporary frame
    FUNCTIONLABEL(func->name)

    // check if the main function is present
    if (!strcmp(func->name, "main"))
    {
        parser->has_main = true;
        is_main = true;
    }

    CheckTokenType(parser, L_ROUND_BRACKET);
    ParametersDefinition(parser, func); // params with )

    if ((token = GetNextToken(&parser->line_number))->token_type != KEYWORD || (token->keyword_type != I32 && token->keyword_type != F64 && token->keyword_type != U8 && token->keyword_type != VOID))
    {
        DestroyToken(token);
        DestroySymtable(parser->symtable);
        ErrorExit(ERROR_SYNTACTIC, "Expected data type at line %d", parser->line_number);
    }

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
    DestroyToken(token);

    // Check for correct return type/params in case of main
    if (is_main && (func->return_type != VOID_TYPE || func->num_of_parameters != 0))
    {
        SymtableStackDestroy(parser->symtable_stack);
        ErrorExit(ERROR_SEMANTIC_TYPECOUNT_FUNCTION, "Main function has incorrect return type or parameters");
    }

    CheckTokenType(parser, L_CURLY_BRACKET);

    parser->in_function = true;

    ProgramBody(parser); // function body

    parser->in_function = false;

    CheckTokenType(parser, R_CURLY_BRACKET);
}

// if(expression)|id|{} else{}
// if(expression){}else{}
void IfElse(Parser *parser)
{
    IfLabel(LOCAL_FRAME);

    Symtable *symtable = InitSymtable(TABLE_COUNT);
    SymtableStackPush(parser->symtable_stack, symtable);
    parser->symtable = symtable;

    CheckTokenType(parser, L_ROUND_BRACKET);
    // expression
    TokenVector *postfix = InfixToPostfix(parser);
    (void)postfix; // Can't handle expressions yet
    CheckTokenType(parser, R_ROUND_BRACKET);

    Token *token = GetNextToken(&parser->line_number);
    if (token->token_type == L_CURLY_BRACKET)
    {
        ProgramBody(parser); // if block
        CheckTokenType(parser, R_CURLY_BRACKET);
        CheckKeywordType(parser, ELSE);
        CheckTokenType(parser, L_CURLY_BRACKET);
        ProgramBody(parser); // else block
        CheckTokenType(parser, R_CURLY_BRACKET);
    }

    else if (token->token_type == VERTICAL_BAR_TOKEN)
    {
        CheckTokenType(parser, VERTICAL_BAR_TOKEN);
        Token *id = CheckAndReturnToken(parser, IDENTIFIER_TOKEN);
        VariableSymbol *var = SymtableStackFindVariable(parser->symtable_stack, id->attribute);
        if (var != NULL)
        {
            SymtableStackDestroy(parser->symtable_stack);
            ErrorExit(ERROR_SEMANTIC_REDEFINED, "Line %d: redefined variable");
        }
    }

    else
    {
        DestroyToken(token);
        SymtableStackDestroy(parser->symtable_stack);
        ErrorExit(ERROR_SYNTACTIC, "Expected '{' or '|' in if block on line %d", parser->line_number);
    }

    // If block
    ProgramBody(parser);
    CheckTokenType(parser, R_CURLY_BRACKET);

    // Else block
    CheckKeywordType(parser, ELSE);
    ElseLabel(LOCAL_FRAME);
    CheckTokenType(parser, L_CURLY_BRACKET);
    ProgramBody(parser);

    // If-Else finish
    CheckTokenType(parser, R_CURLY_BRACKET);
    EndIfLabel(LOCAL_FRAME);
}

void WhileLoop(Parser *parser)
{
    Symtable *symtable = InitSymtable(TABLE_COUNT);
    SymtableStackPush(parser->symtable_stack, symtable);
    parser->symtable = symtable;

    CheckTokenType(parser, L_ROUND_BRACKET);
    // expression
    Expression(parser);
    CheckTokenType(parser, R_ROUND_BRACKET);

    CheckTokenType(parser, L_CURLY_BRACKET);
    ProgramBody(parser);
    CheckTokenType(parser, R_CURLY_BRACKET);
}

void VarDeclaration(Parser *parser)
{
    Token *token;
    token = CheckAndReturnToken(parser, IDENTIFIER_TOKEN);

    // add to symtable
    VariableSymbol *var = VariableSymbolInit();
    var->name = strdup(token->attribute);
    var->is_const = false;
    var->type = VOID_TYPE;

    // check if the variable is already declared in stack
    VariableSymbol *var_in_stack = SymtableStackFindVariable(parser->symtable_stack, var->name);

    if (var_in_stack != NULL)
    {
        ErrorExit(ERROR_SEMANTIC_REDEFINED, "Variable %s already declared", var->name);
    }

    // insert into symtable on top of the stack
    InsertVariableSymbol(SymtableStackTop(parser->symtable_stack), var);

    DestroyToken(token);
    CheckTokenType(parser, ASSIGNMENT);
    // expressi
    CheckTokenType(parser, SEMICOLON);
}

// const id = expression;
void ConstDeclaration(Parser *parser)
{
    Token *token;
    token = CheckAndReturnToken(parser, IDENTIFIER_TOKEN);

    // add to symtable
    VariableSymbol *var = VariableSymbolInit();
    var->name = strdup(token->attribute);
    var->is_const = true;
    var->type = VOID_TYPE;

    // Define a variable in IFJCode24
    DefineVariable(var->name, LOCAL_FRAME);

    // check if the variable is already declared in stack
    VariableSymbol *var_in_stack = SymtableStackFindVariable(parser->symtable_stack, var->name);

    if (var_in_stack != NULL)
    {
        ErrorExit(ERROR_SEMANTIC_REDEFINED, "Variable %s already declared", var->name);
    }

    // insert into symtable on top of the stack
    InsertVariableSymbol(SymtableStackTop(parser->symtable_stack), var);

    DestroyToken(token);

    token = GetNextToken(&parser->line_number);

    if (token->token_type != ASSIGNMENT && token->token_type != COLON_TOKEN)
    {
        DestroyToken(token);
        DestroySymtable(parser->symtable);
        ErrorExit(ERROR_SYNTACTIC, "Expected '=' or ':' at line %d", parser->line_number);
    }

    // const a : i32 = 5;
    if (token->token_type == COLON_TOKEN)
    {
        // data type
        DestroyToken(token);
        token = GetNextToken(&parser->line_number);
        if (token->token_type != KEYWORD || (token->keyword_type != I32 && token->keyword_type != F64 && token->keyword_type != U8))
        {
            DestroyToken(token);
            DestroySymtable(parser->symtable);
            ErrorExit(ERROR_SYNTACTIC, "Expected data type at line %d", parser->line_number);
        }
        var->type = token->keyword_type == I32 ? INT32_TYPE : token->keyword_type == F64 ? DOUBLE64_TYPE
                                                                                         : U8_ARRAY_TYPE;
        DestroyToken(token);

        CheckTokenType(parser, ASSIGNMENT);
    }

    else if (token->token_type != ASSIGNMENT)
    {
        DestroyToken(token);
        SymtableStackDestroy(parser->symtable_stack);
        ErrorExit(ERROR_SYNTACTIC, "Expected '=' at line %d", parser->line_number);
    }

    else
        DestroyToken(token);

    // expression
    TokenVector *postfix = InfixToPostfix(parser);
    DATA_TYPE expr_type;
    if ((expr_type = GeneratePostfixExpression(parser, postfix, var)) != var->type && var->type != VOID_TYPE)
    {
        // assignment to invalid type case
        SymtableStackDestroy(parser->symtable_stack);
        if (token != NULL)
            DestroyToken(token);
        ErrorExit(ERROR_SEMANTIC_TYPE_COMPATIBILITY, "Line %d: Incompatible type in assignment to variable", parser->line_number);
    }

    // if the variable doesn't have a type yet, derive it from the expression (TODO: Add check for invalid types, etc.)
    if (var->type == VOID_TYPE)
        var->type = expr_type;

    CheckTokenType(parser, SEMICOLON);
}

void FunctionCall(Parser *parser, FunctionSymbol *func, const char *fun_name, DATA_TYPE expected_return)
{
    CREATEFRAME
    // Check if the function exists
    if (func == NULL)
    {
        func = FunctionSymbolInit();
        func->name = strdup(fun_name);
        func->was_defined = false;
        func->num_of_parameters = -1; // Unspecified number of parameters
        func->parameters = NULL;
        func->return_type = expected_return;

        // Create a new value in the symtable
        InsertFunctionSymbol(parser->global_symtable, func);
    }

    // Invalid return value
    else if (func->return_type != expected_return)
    {
        PrintError("Line %d: Invalid return type for function \"%s\"",
                   parser->line_number, fun_name);
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
    }

    ParametersOnCall(parser, func);
    PUSHFRAME
    FUNCTIONCALL(func->name)
    func->was_called = true;
}

void ParametersOnCall(Parser *parser, FunctionSymbol *func)
{
    Token *token;
    int loaded = 0;
    VariableSymbol *symb1; // for identifier parameter checking

    // Load all params
    while ((token = GetNextToken(&parser->line_number))->token_type != R_ROUND_BRACKET)
    {
        NEWPARAM(++loaded)
        switch (token->token_type)
        {
        case INTEGER_32:
            if (func->was_defined)
            {
                if ((loaded) <= func->num_of_parameters)
                    CHECK_PARAM(INT32_TYPE, func->parameters[loaded - 1]->type)

                else // Invalid count of parameters
                    INVALID_PARAM_COUNT
            }

            else // function not defined yet, append a parameter
            {
                // Realloc and check for NULL
                LENGHTEN_PARAMS
                VariableSymbol *param = VariableSymbolInit();
                param->type = INT32_TYPE;
                func->parameters[loaded - 1] = param;
                SETPARAM(loaded, token->attribute)
            }

            break;

        case DOUBLE_64:
            if (func->was_defined)
            {
                if ((loaded) <= func->num_of_parameters)
                    CHECK_PARAM(DOUBLE64_TYPE, func->parameters[loaded - 1]->type)

                else
                    INVALID_PARAM_COUNT
            }

            else
            {
                LENGHTEN_PARAMS
                VariableSymbol *param = VariableSymbolInit();
                param->type = DOUBLE64_TYPE;
                func->parameters[loaded - 1] = param;
                SETPARAM(loaded, token->attribute)
            }

            break;

        case LITERAL_TOKEN:
            if (func->was_defined)
            {
                if ((loaded) <= func->num_of_parameters)
                    CHECK_PARAM(U8_ARRAY_TYPE, func->parameters[loaded]->type)

                else
                    INVALID_PARAM_COUNT
            }

            else
            {
                LENGHTEN_PARAMS
                VariableSymbol *param = VariableSymbolInit();
                param->type = U8_ARRAY_TYPE;
                func->parameters[loaded - 1] = param;
                SETPARAM(loaded, token->attribute)
            }

            break;

        case IDENTIFIER_TOKEN:
            symb1 = SymtableStackFindVariable(parser->symtable_stack, token->attribute);
            if (symb1 == NULL) // Undefined variable as a parameter
            {
                PrintError("Error in semantic analysis: Line %d: Undefined variable \"%s\" used in function call",
                           parser->line_number, token->attribute);
                SymtableStackDestroy(parser->symtable_stack);
                DestroySymtable(parser->global_symtable);
                DestroyToken(token);
                exit(ERROR_SEMANTIC_UNDEFINED);
            }

            else // Valid variable as a parameter
            {
                if (func->was_defined)
                {
                    if ((loaded) <= func->num_of_parameters)
                        CHECK_PARAM(symb1->type, func->parameters[loaded - 1]->type)

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
            }

            break;

        default:
            PrintError("Error in syntax analysis: Line %d: Invalid token in expression",
                       parser->line_number);
            DestroyToken(token);
            SymtableStackDestroy(parser->symtable_stack);
            DestroySymtable(parser->global_symtable);
            exit(ERROR_SYNTACTIC);
        }

        DestroyToken(token);

        if ((token = GetNextToken(&parser->line_number))->token_type == R_ROUND_BRACKET)
        {
            DestroyToken(token);
            break;
        }

        else if (token->token_type != COMMA_TOKEN)
        {
            PrintError("Error in syntax analysis: Line %d: Invalid token in expression",
                       parser->line_number);
            DestroyToken(token);
            SymtableStackDestroy(parser->symtable_stack);
            DestroySymtable(parser->global_symtable);
            exit(ERROR_SYNTACTIC);
        }

        else
            DestroyToken(token);
    }

    ungetc(')', stdin);
    if (!func->was_defined)
        func->num_of_parameters = loaded;
}

void FunctionReturn(Parser *parser)
{
    Token *token;
    if (!parser->in_function) // In main, return exits the program
    {
        if ((token = GetNextToken(&parser->line_number))->token_type != SEMICOLON)
        {
            SymtableStackDestroy(parser->symtable_stack);
            DestroySymtable(parser->symtable);
            DestroyToken(token);
            ErrorExit(ERROR_SEMANTIC_MISSING_EXPR, "Line %d: Invalid usage of \"return\" in main function (unexpected expression)");
        }
        IFJ24SUCCESS // Successful return from main = EXIT 0
            return;
    }

    // void function case
    if (parser->current_function->return_type == VOID_TYPE)
    {
        if ((token = GetNextToken(&parser->line_number))->token_type != SEMICOLON) // returning something from void function
        {
            PrintError("Line %d: Returning a value from void function \"%s\"",
                       parser->line_number, parser->current_function->name);
            SymtableStackDestroy(parser->symtable_stack);
            DestroySymtable(parser->global_symtable);
            DestroyToken(token);
            exit(ERROR_SEMANTIC_MISSING_EXPR);
        }

        else
        {
            DestroyToken(token);
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

void ProgramBody(Parser *parser)
{
    Token *token; FunctionSymbol *func;
    while (true)
    {
        token = GetNextToken(&parser->line_number);
        switch (token->token_type)
        {
        case KEYWORD:
            // start of function declaration
            if (token->keyword_type == PUB)
            {
                DestroyToken(token);
                FunctionDefinition(parser);
            }
            // start of if-else block
            else if (token->keyword_type == IF)
            {
                DestroyToken(token);
                ++(parser->nested_level);
                IfElse(parser);
            }
            // start of while loop
            else if (token->keyword_type == WHILE)
            {
                DestroyToken(token);
                ++(parser->nested_level);
                WhileLoop(parser);
            }
            else if (token->keyword_type == CONST)
            {
                DestroyToken(token);
                ConstDeclaration(parser);
            }
            else if (token->keyword_type == VAR)
            {
                DestroyToken(token);
                VarDeclaration(parser);
            }
            else if (token->keyword_type == RETURN) // in function
            {
                DestroyToken(token);
                FunctionReturn(parser);
            }
            else
            {
                DestroyToken(token);
                // ErrorExit(ERROR_SYNTACTIC, "Unexpected at line %d", *line_number);
            }
            break;

        case R_CURLY_BRACKET:
            DestroyToken(token);
            --(parser->nested_level);
            SymtableStackRemoveTop(parser->symtable_stack);
            ungetc('}', stdin);
            return;

        case IDENTIFIER_TOKEN:
            // Can be a embedded function, or a defined function, or a called function
            func = !strcmp("ifj", token->attribute) ? IsEmbeddedFunction(parser) : FindFunctionSymbol(parser->global_symtable, token->attribute);
            CheckTokenType(parser, L_ROUND_BRACKET);
            FunctionCall(parser, func, token->attribute, VOID_TYPE); // Void type since we are just calling the function, not assigning it
            CheckTokenType(parser, R_ROUND_BRACKET);
            CheckTokenType(parser, SEMICOLON);
            DestroyToken(token);
            break;

        case EOF_TOKEN:
            DestroyToken(token);
            return;

        default:
            // ErrorExit(ERROR_SYNTACTIC, "Unexpected token at line %d", *line_number);
            DestroyToken(token);
            break;
        }
    }
    DestroyToken(token);
}

#ifdef IFJ24_DEBUG // not for debugs

int main()
{
    // parser instance
    Parser parser = InitParser();
    ProgramBegin(&parser);

    ProgramBody(&parser);
    printf("\033[1m\033[32m"
           "SYNTAX OK\n"
           "\033[0m");

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

#endif