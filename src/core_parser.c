/**
 * @file core_parser.c
 * @brief The main parser for the compiler, handling the overall syntax and semantic analysis.
 *
 * This file serves as the central component of the compiler's parsing process.
 * It coordinates the parsing of various language constructs, ensures syntax and
 * semantic correctness, and builds the necessary intermediate representations for the compilation pipeline.
 *
 *
 * @authors
 * - Igor Lacko [xlackoi00]
 * - Boris Semanco [xseman06]
 */

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

    else
        AppendToken(stream, token);
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

    else
        AppendToken(stream, token);
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
        stream_index -= 2;
        return var;
    }

    return NULL;
}

bool IsFunctionCall(Parser *parser)
{
    stream_index--;
    Token *id = GetNextToken(parser);
    Token *braces = GetNextToken(parser);
    if (id->token_type == IDENTIFIER_TOKEN && braces->token_type == L_ROUND_BRACKET)
    {
        stream_index -= 2;
        return true;
    }

    stream_index -= 2;
    return false;
}

void PrintStreamTokens(Parser *parser)
{
    for (int i = 0; i < stream->length; i++)
    {
        PrintToken(stream->token_string[i]);
    }
    // DestroyTokenVector(stream);
    SymtableStackDestroy(parser->symtable_stack);
    DestroySymtable(parser->global_symtable);
    DestroyTokenVector(stream);
    exit(SUCCESS);
}

// const ifj = @import("ifj24.zig");
void Header(Parser *parser)
{
    CheckKeywordTypeVector(parser, CONST);

    Token *ifj = CheckAndReturnTokenVector(parser, IDENTIFIER_TOKEN);
    if (strcmp(ifj->attribute, "ifj") != 0)
    {
        DestroyTokenVector(stream);
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        ErrorExit(ERROR_SYNTACTIC, "Expected 'ifj' at line %d", parser->line_number);
    }

    CheckTokenTypeVector(parser, ASSIGNMENT);
    CheckTokenTypeVector(parser, IMPORT_TOKEN);
    CheckTokenTypeVector(parser, L_ROUND_BRACKET);

    Token *import_ifj24 = CheckAndReturnTokenVector(parser, LITERAL_TOKEN);
    if (strcmp(import_ifj24->attribute, "ifj24.zig") != 0)
    {
        DestroyTokenVector(stream);
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        ErrorExit(ERROR_SYNTACTIC, "Expected 'ifj24.zig' at line %d", parser->line_number);
    }

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

    if (!IsIfNullableType(parser))
        ParseIfStatement(parser);

    else
        ParseNullableIfStatement(parser);
}

void WhileLoop(Parser *parser)
{
    Symtable *symtable = InitSymtable(TABLE_COUNT);
    SymtableStackPush(parser->symtable_stack, symtable);
    parser->symtable = symtable;

    if (!IsLoopNullableType(parser))
        ParseWhileLoop(parser);

    else
        ParseNullableWhileLoop(parser);
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

    // insert into symtable on top of the stack
    InsertVariableSymbol(parser, var);

    token = GetNextToken(parser);

    if (token->token_type != ASSIGNMENT && token->token_type != COLON_TOKEN)
    {
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
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
            DestroySymtable(parser->global_symtable);
            SymtableStackDestroy(parser->symtable_stack);
            ErrorExit(ERROR_SYNTACTIC, "Expected data type at line %d", parser->line_number);
        }
        var->type = token->keyword_type == I32 ? INT32_TYPE : token->keyword_type == F64 ? DOUBLE64_TYPE
                                                                                         : U8_ARRAY_TYPE;

        if (token->attribute[0] == '?')
        {
            var->nullable = true;
            var->type = token->keyword_type == I32 ? INT32_NULLABLE_TYPE : token->keyword_type == F64 ? DOUBLE64_NULLABLE_TYPE
                                                                                                      : U8_ARRAY_NULLABLE_TYPE;
        }

        CheckTokenTypeVector(parser, ASSIGNMENT);
    }

    else if (token->token_type != ASSIGNMENT)
    {
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        DestroyTokenVector(stream);
        ErrorExit(ERROR_SYNTACTIC, "Expected '=' at line %d", parser->line_number);
    }

    // Try to remember the variable's value at compile-time
    if (var->is_const && ConstValueAssignment(var))
        return;

    // Assign to the variable
    VariableAssignment(parser, var, false);
}

bool ConstValueAssignment(VariableSymbol *var)
{
    Token *potential_value = stream->token_string[stream_index];
    Token *potential_semicolon = stream->token_string[stream_index + 1];

    // If the [next + 1] token is a semicolon, we can check the value
    if (potential_semicolon->token_type == SEMICOLON)
    {
        switch (potential_value->token_type)
        {
        // Valid types in expression
        case INTEGER_32:
            if (var->type == INT32_TYPE || var->type == INT32_NULLABLE_TYPE || var->type == VOID_TYPE)
            {
                var->value = strdup(potential_value->attribute);
                stream_index += 2;
                fprintf(stdout, "MOVE LF@%s int@%s\n", var->name, var->value);
                return true;
            }

            break;

        case DOUBLE_64:
            if (var->type == DOUBLE64_TYPE || var->type == DOUBLE64_NULLABLE_TYPE || var->type == VOID_TYPE)
            {
                var->value = strdup(potential_value->attribute);
                stream_index += 2;
                fprintf(stdout, "MOVE LF@%s float@%a\n", var->name, strtod(var->value, NULL));
                return true;
            }

            break;

        case KEYWORD:
            if (potential_value->keyword_type == NULL_TYPE && var->nullable)
            {
                var->value = strdup(potential_value->attribute);
                stream_index += 2;
                fprintf(stdout, "MOVE LF@%s nil@nil\n", var->name);
                return true;
            }

            break;

        default:
            return false;
        }
    }

    return false;
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
    CheckTokenTypeVector(parser, SEMICOLON);
}

void ParametersOnCall(Parser *parser, FunctionSymbol *func)
{
    Token *token;
    int loaded = 0;
    VariableSymbol *symb1; // for identifier parameter checking
    bool break_flag = false;

    // Main loop
    while ((token = GetNextToken(parser))->token_type != R_ROUND_BRACKET)
    {
        switch (token->token_type)
        {
        // Basically the same handling for all of these, check if the type matches and generate code
        case INTEGER_32:
        case DOUBLE_64:
        case LITERAL_TOKEN:
            // Also check if the count isn't too many
            if (loaded >= func->num_of_parameters)
                INVALID_PARAM_COUNT

            // Get the data type depending on the token type
            DATA_TYPE type_got = token->token_type == INTEGER_32 ? INT32_TYPE : token->token_type == DOUBLE_64 ? DOUBLE64_TYPE
                                                                                                               : U8_ARRAY_TYPE;

            // Check if the type matches
            if (!CheckParamType(func->parameters[loaded]->type, type_got))
                INVALID_PARAM_TYPE

            // Everything's fine, generate code
            NEWPARAM(loaded)
            SETPARAM(loaded++, token->attribute, token->token_type, LOCAL_FRAME);

            // Check if the next token is a comma or a closing bracket
            if ((token = GetNextToken(parser))->token_type != R_ROUND_BRACKET && token->token_type != COMMA_TOKEN)
                INVALID_PARAM_TOKEN

            // If it's a comma, we can continue the loop
            else if (token->token_type == COMMA_TOKEN)
                break;

            // If it's a closing bracket, we're done and we can perform checks after the loop
            else
            {
                break_flag = true;
                break;
            }

        // Very similar to the case above, but we have to check if the identifier is defined, access it in the symtable and check the type
        case IDENTIFIER_TOKEN:
            // Again, first check too many params case
            if (loaded >= func->num_of_parameters)
                INVALID_PARAM_COUNT

            // Check if the identifier is defined
            else if ((symb1 = SymtableStackFindVariable(parser->symtable_stack, token->attribute)) == NULL)
            {
                PrintError("Line %d: Undefined variable \"%s\"", parser->line_number, token->attribute);
                DestroySymtable(parser->global_symtable);
                SymtableStackDestroy(parser->symtable_stack);
                DestroyTokenVector(stream);
                exit(ERROR_SEMANTIC_UNDEFINED);
            }

            // Check the type of the identifier
            else if (!CheckParamType(func->parameters[loaded]->type, symb1->type))
                INVALID_PARAM_TYPE

            symb1->was_used = true;

            // Generate code
            NEWPARAM(loaded)
            SETPARAM(loaded++, token->attribute, token->token_type, LOCAL_FRAME);

            // Check if the next token is a comma or a closing bracket
            if ((token = GetNextToken(parser))->token_type != R_ROUND_BRACKET && token->token_type != COMMA_TOKEN)
                INVALID_PARAM_TOKEN

            // If it's a comma, we can continue the loop
            else if (token->token_type == COMMA_TOKEN)
                break;

            // If it's a closing bracket, we're done and we can perform checks after the loop
            else
            {
                break_flag = true;
                break;
            }

        // Invalid token
        default:
            fprintf(stderr, "Unexpected token \"%s\"\n", token->attribute);
            INVALID_PARAM_TOKEN
        }
        if (break_flag)
            break;
    }

    // Check if we have the right amount of parameters
    if (loaded != func->num_of_parameters)
        INVALID_PARAM_COUNT
}

bool IsTermType(DATA_TYPE type)
{
    return (type == INT32_TYPE || type == DOUBLE64_TYPE || type == U8_ARRAY_TYPE || type == INT32_NULLABLE_TYPE || type == DOUBLE64_NULLABLE_TYPE || type == U8_ARRAY_NULLABLE_TYPE || type == TERM_TYPE);
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
    CheckKeywordTypeVector(parser, FN);
    Token *token = CheckAndReturnTokenVector(parser, IDENTIFIER_TOKEN);
    FunctionSymbol *func = FindFunctionSymbol(parser->global_symtable, token->attribute);

    // Generate code for the function label
    FUNCTIONLABEL(func->name)
    if (!strcmp(func->name, "main"))
        CREATEFRAME
    PUSHFRAME

    // define variables
    for (int i = 0; i < func->variables.count; i++)
    {
        DefineVariable(func->variables.strings[i], LOCAL_FRAME);
    }

    // Skip all tokens until the function body begins (so after '{')
    while ((token = GetNextToken(parser))->token_type != L_CURLY_BRACKET)
        continue;

    // Create a new symtable for the function's stack frame
    Symtable *symtable = InitSymtable(TABLE_COUNT);
    SymtableStackPush(parser->symtable_stack, symtable);
    parser->symtable = symtable;

    // Add the function parameters to the symtable and define them on the local frame
    for (int i = 0; i < func->num_of_parameters; i++)
    {
        // Check for redefinition of the parameter
        if (FindVariableSymbol(symtable, func->parameters[i]->name) != NULL)
        {
            PrintError("Error in semantic analysis: Line %d: Redefinition of parameter \"%s\" in function \"%s\"",
                       parser->line_number, func->parameters[i]->name, func->name);
            CLEANUP
            exit(ERROR_SEMANTIC_REDEFINED);
        }

        // Valid param
        DefineVariable(func->parameters[i]->name, LOCAL_FRAME);
        fprintf(stdout, "MOVE LF@%s LF@PARAM%d\n", func->parameters[i]->name, i);
        VariableSymbol *param = VariableSymbolCopy(func->parameters[i]);
        param->is_const = true; // Parameters are always constants for some reason
        param->defined = true;
        InsertVariableSymbol(parser, param);
    }

    // Set the current function
    parser->current_function = func;

    // Function body
    ProgramBody(parser);

    // If the function is main, exit at the end
    if (!strcmp(func->name, "main"))
        IFJ24SUCCESS

    // Also if it's a void function put a return at the end
    else if (func->return_type == VOID_TYPE)
    {
        POPFRAME
        FUNCTION_RETURN
    }

    // If it's not a void function and doesn't have a return statement, throw an error
    else if (!func->has_return)
    {
        PrintError("Error in semantic analysis: Line %d: Missing return statement in function \"%s\"",
                   parser->line_number, func->name);
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        DestroyTokenVector(stream);
        exit(ERROR_SEMANTIC_MISSING_EXPR);
    }

    // Not in a function anymore
    parser->current_function = NULL;
}

void FunctionReturn(Parser *parser)
{
    /* The function has a return statement
        - TODO: Assert that this doesn't get called when outside of a function?
        - That would throw a segfault, but i think ProgramBody() detects that already
    */
    parser->current_function->has_return = true;

    Token *token;
    if (!strcmp(parser->current_function->name, "main")) // In main, return exits the program
    {
        if ((token = GetNextToken(parser))->token_type != SEMICOLON)
        {
            SymtableStackDestroy(parser->symtable_stack);
            DestroySymtable(parser->global_symtable);
            DestroyTokenVector(stream);
            ErrorExit(ERROR_SEMANTIC_MISSING_EXPR, "Line %d: Invalid usage of \"return\" in main function (unexpected expression)");
        }
        POPFRAME
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
            POPFRAME
            FUNCTION_RETURN
            return;
        }
    }

    // function with a return type
    // return expression;
    else
    {
        // Check if an expression follows the return statement and revert the stream
        if ((token = GetNextToken(parser))->token_type == SEMICOLON)
        {
            PrintError("Error in semantic analysis: Line %d: Missing expression in return statement for function \"%s\"",
                       parser->line_number, parser->current_function->name);
            SymtableStackDestroy(parser->symtable_stack);
            DestroySymtable(parser->global_symtable);
            DestroyTokenVector(stream);
            exit(ERROR_SEMANTIC_MISSING_EXPR);
        }

        stream_index--; // Revert the stream to the beginning of the expression

        /* 2 options here after the return statement:
            - return EXPR;
            - return OPERAND; --> this means we have to check the next 2 tokens
        */

        // Case 1
        Token *potential_operand = GetNextToken(parser);
        Token *potential_semicolon = GetNextToken(parser);

        if (potential_semicolon->token_type == SEMICOLON)
        {
            VariableSymbol *potential_retval = NULL;
            switch (potential_operand->token_type)
            {
            case INTEGER_32:
                if (!AreTypesCompatible(parser->current_function->return_type, INT32_TYPE))
                {
                    PrintError("Error in semantic analysis: Line %d: Invalid return type for function \"%s\"",
                               parser->line_number, parser->current_function->name);
                    CLEANUP
                    exit(ERROR_SEMANTIC_TYPECOUNT_FUNCTION);
                }

                fprintf(stdout, "PUSHS int@%s\n", potential_operand->attribute);
                POPFRAME
                FUNCTION_RETURN
                return;

            case DOUBLE_64:
                if (!AreTypesCompatible(parser->current_function->return_type, DOUBLE64_TYPE))
                {
                    PrintError("Error in semantic analysis: Line %d: Invalid return type for function \"%s\"",
                               parser->line_number, parser->current_function->name);
                    CLEANUP
                    exit(ERROR_SEMANTIC_TYPECOUNT_FUNCTION);
                }

                fprintf(stdout, "PUSHS float@%a\n", strtod(potential_operand->attribute, NULL));
                POPFRAME
                FUNCTION_RETURN
                return;

            case KEYWORD:
                if (token->keyword_type != NULL_TYPE)
                {
                    PrintError("Error in syntax analysis: Line %d: Unexpected token \"%s\" in return statement",
                               parser->line_number, token->attribute);
                    CLEANUP
                    exit(ERROR_SYNTACTIC);
                }

                else if (!AreTypesCompatible(parser->current_function->return_type, NULL_DATA_TYPE))
                {
                    PrintError("Error in semantic analysis: Line %d: Invalid return type for function \"%s\"",
                               parser->line_number, parser->current_function->name);
                    fprintf(stderr, "expected %d, got %d\n", parser->current_function->return_type, NULL_DATA_TYPE);
                    CLEANUP
                    exit(ERROR_SEMANTIC_TYPECOUNT_FUNCTION);
                }

                fprintf(stdout, "PUSHS nil@nil\n");
                POPFRAME
                FUNCTION_RETURN
                return;

            case IDENTIFIER_TOKEN:
                potential_retval = SymtableStackFindVariable(parser->symtable_stack, potential_operand->attribute);

                // Check if the variable is defined
                if (potential_retval == NULL)
                {
                    PrintError("Error in semantic analysis: Line %d: Undefined variable \"%s\"",
                               parser->line_number, potential_operand->attribute);
                    CLEANUP
                    exit(ERROR_SEMANTIC_UNDEFINED);
                }

                // Mark the variable as used
                potential_retval->was_used = true;

                // Check if the variable is compatible with the return type
                if (!AreTypesCompatible(parser->current_function->return_type, potential_retval->type))
                {
                    PrintError("Error in semantic analysis: Line %d: Invalid return type for function \"%s\"",
                               parser->line_number, parser->current_function->name);
                    CLEANUP
                    exit(ERROR_SEMANTIC_TYPECOUNT_FUNCTION);
                }

                fprintf(stdout, "PUSHS LF@%s\n", potential_operand->attribute);
                POPFRAME
                FUNCTION_RETURN
                return;

            default:
                PrintError("Error in syntax analysis: Line %d: Unexpected token \"%s\" in return statement",
                           parser->line_number, token->attribute);
                CLEANUP
                exit(ERROR_SYNTACTIC);
            }
        }

        // Case 2
        else
        {
            // Revert the expression back
            stream_index -= 2;

            // The returned type is now on top of the data stack
            TokenVector *postfix = InfixToPostfix(parser);
            DATA_TYPE expr_type = ParseExpression(postfix, parser);

            // Invalid return type
            if (expr_type != parser->current_function->return_type)
            {
                fprintf(stderr, "Expected %d, got %d\n", parser->current_function->return_type, expr_type);
                PrintError("Error in semantic analysis: Line %d: Invalid return type for function \"%s\"",
                           parser->line_number, parser->current_function->name);
                SymtableStackDestroy(parser->symtable_stack);
                DestroySymtable(parser->global_symtable);
                DestroyTokenVector(stream);
                exit(ERROR_SEMANTIC_TYPECOUNT_FUNCTION);
            }

            // exit the function
            POPFRAME
            FUNCTION_RETURN
            return;
        }
    }
}

DATA_TYPE NullableToNormal(DATA_TYPE type)
{
    switch (type)
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

void VariableAssignment(Parser *parser, VariableSymbol *var, bool is_underscore)
{
    if (!is_underscore && var->is_const && var->defined)
    {
        PrintError("Error in semantic analysis: Line %d: Reassignment of constant variable \"%s\"",
                   parser->line_number, var->name);
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        exit(ERROR_SEMANTIC_REDEFINED);
    }

    if (!is_underscore && var->defined)
        var->was_used = true;

    stream_index++; // For IsFunctionCall to work correctly
    if (IsFunctionCall(parser))
    {
        // Get the function name to use as a key into the hash table
        Token *func_name = GetNextToken(parser);
        FunctionSymbol *func = FindFunctionSymbol(parser->global_symtable, func_name->attribute);
        if (func == NULL)
        {
            PrintError("Error in semantic analysis: Line %d: Undefined function \"%s\"",
                       parser->line_number, func_name->attribute);
            CLEANUP
            exit(ERROR_SEMANTIC_UNDEFINED);
        }
        FunctionToVariable(parser, var, func, is_underscore);
        CheckTokenTypeVector(parser, SEMICOLON);
        return;
    }

    // Check for a embedded function call
    Token *token = GetNextToken(parser);
    if (!strcmp(token->attribute, "ifj"))
    {
        FunctionSymbol *func = IsEmbeddedFunction(parser);
        stream_index += 2;                       // skip the 'ifj' and the '.' token
        EmbeddedFunctionCall(parser, func, var); // If var is null, that means is_underscore is true
        return;
    }

    // String literals can't be assigned to variables, raise type compatibility error if variable has a type, else type derivation error
    else if (token->token_type == LITERAL_TOKEN)
    {
        // Type compatibility error
        if (var->type != VOID_TYPE)
        {
            PrintError("Error in semantic analysis: Line %d: Assigning string literal to variable \"%s\"",
                       parser->line_number, var->name);
            CLEANUP
            exit(ERROR_SEMANTIC_TYPE_COMPATIBILITY);
        }

        // Type derivation error
        PrintError("Error in semantic analysis: Line %d: Can't derive type for variable \"%s\" from string literal",
                   parser->line_number, var->name);
        CLEANUP
        exit(ERROR_SEMANTIC_TYPE_DERIVATION);
    }

    // Null can also be assigned to a variable, but we have to check if it's a nullable type
    else if (token->keyword_type == NULL_TYPE)
    {
        // Assigning only NULL
        if (GetNextToken(parser)->token_type == SEMICOLON)
        {
            // Type derivation error
            if (!is_underscore && var->type == VOID_TYPE)
            {
                PrintError("Error in semantic analysis: Line %d: Assigning NULL to variable \"%s\" with no type",
                           parser->line_number, var->name);
                CLEANUP
                exit(ERROR_SEMANTIC_TYPE_DERIVATION);
            }

            if (!is_underscore && !IsNullable(var->type))
            {
                PrintError("Error in semantic analysis: Line %d: Assigning NULL to non-nullable variable \"%s\"",
                           parser->line_number, var->name);
                SymtableStackDestroy(parser->symtable_stack);
                DestroySymtable(parser->global_symtable);
                DestroyTokenVector(stream);
                exit(ERROR_SEMANTIC_TYPE_COMPATIBILITY);
            }

            // Assign NULL to the variable
            if (!is_underscore)
                fprintf(stdout, "MOVE LF@%s nil@nil\n", var->name);
            else
            {
                fprintf(stdout, "PUSHS nil@nil\n");
                CLEARS
            }
            return;
        }

        else
            stream_index--; // Move the stream back to before the potential semicolon
    }

    // If the 'ifj' token is not present, and NULL is not present move the stream back and expect an expression
    else
        stream_index--;

    /* Once again, two cases:
        - The expression is a single token (int, double, identifier) and a semicolon follows
        - The expression is a complex expression and a semicolon follows
    */
    Token *potential_operand = GetNextToken(parser);
    Token *potential_semicolon = GetNextToken(parser);

    // Case 1
    if (potential_semicolon->token_type == SEMICOLON)
    {
        // Because FOR SOME REASON, C doesn't allow you to declare variables at the start of a switch case
        VariableSymbol *potential_operand_symbol = NULL;
        switch (potential_operand->token_type)
        {
        case INTEGER_32:
            if (is_underscore)
                return;

            else if (AreTypesCompatible(var->type, INT32_TYPE) || var->type == VOID_TYPE)
            {
                fprintf(stdout, "MOVE LF@%s int@%s\n", var->name, potential_operand->attribute);
                var->type = INT32_TYPE;
                var->defined = true;
                return;
            }

            else
            {
                PrintError("Error in semantic analysis: Line %d: Assigning invalid type to variable \"%s\"",
                           parser->line_number, var->name);
                CLEANUP
                exit(ERROR_SEMANTIC_TYPE_COMPATIBILITY);
            }

            break;

        case DOUBLE_64:
            if (is_underscore)
                return;

            else if (AreTypesCompatible(var->type, DOUBLE64_TYPE) || var->type == VOID_TYPE)
            {
                fprintf(stdout, "MOVE LF@%s float@%a\n", var->name, strtod(potential_operand->attribute, NULL));
                var->type = DOUBLE64_TYPE;
                var->defined = true;
                return;
            }

            else
            {
                PrintError("Error in semantic analysis: Line %d: Assigning invalid type to variable \"%s\"",
                           parser->line_number, var->name);
                CLEANUP
                exit(ERROR_SEMANTIC_TYPE_COMPATIBILITY);
            }

            break;

        case KEYWORD:
            if (potential_operand->keyword_type != NULL_TYPE)
            {
                PrintError("Error in syntax analysis: Line %d: Unexpected token \"%s\" in assignment",
                           parser->line_number, potential_operand->attribute);
                CLEANUP
                exit(ERROR_SYNTACTIC);
            }

            // Can not derive a type from null
            else if (var->type == VOID_TYPE)
            {
                PrintError("Error in semantic analysis: Line %d: Can't derive type for variable \"%s\" from \"null\"",
                           parser->line_number, var->name);
                CLEANUP
                exit(ERROR_SEMANTIC_TYPE_DERIVATION);
            }

            else if (AreTypesCompatible(var->type, NULL_DATA_TYPE))
            {
                fprintf(stdout, "MOVE LF@%s nil@nil\n", var->name);
                var->defined = true;
                return;
            }

            else
            {
                PrintError("Error in semantic analysis: Line %d: Assigning invalid type to variable \"%s\"",
                           parser->line_number, var->name);
                CLEANUP
                exit(ERROR_SEMANTIC_TYPE_COMPATIBILITY);
            }

            break;

        case IDENTIFIER_TOKEN:
            potential_operand_symbol = SymtableStackFindVariable(parser->symtable_stack, potential_operand->attribute);

            // Undefined case
            if (potential_operand_symbol == NULL)
            {
                PrintError("Error in semantic analysis: Line %d: Undefined variable \"%s\"",
                           parser->line_number, potential_operand->attribute);
                CLEANUP
                exit(ERROR_SEMANTIC_UNDEFINED);
            }

            // Mark the variable as used
            potential_operand_symbol->was_used = true;

            // Type compatibility check
            if (AreTypesCompatible(var->type, potential_operand_symbol->type) || var->type == VOID_TYPE)
            {
                fprintf(stdout, "MOVE LF@%s LF@%s\n", var->name, potential_operand->attribute);
                var->type = potential_operand_symbol->type;
                var->defined = true;
                return;
            }

            else
            {
                PrintError("Error in semantic analysis: Line %d: Assigning invalid type to variable \"%s\"",
                           parser->line_number, var->name);
                CLEANUP
                exit(ERROR_SEMANTIC_TYPE_COMPATIBILITY);
            }

            break;

        // Syntax error
        default:
            PrintError("Error in syntax analysis: Line %d: Unexpected token \"%s\" in assignment to variable",
                       parser->line_number, potential_operand->attribute);
            CLEANUP
            exit(ERROR_SYNTACTIC);
        }
    }

    else
    {
        // Case 2, move the stream back and parse the expression
        stream_index -= 2;

        TokenVector *postfix = InfixToPostfix(parser);
        DATA_TYPE expr_type = ParseExpression(postfix, parser);

        if (!is_underscore && !AreTypesCompatible(var->type, expr_type) && var->type != VOID_TYPE)
        {
            PrintError("Error in semantic analysis: Line %d: Assigning invalid type to variable \"%s\", expected %d, got %d",
                       parser->line_number, var->name, var->type, expr_type);
            DestroyTokenVector(stream);
            SymtableStackDestroy(parser->symtable_stack);
            DestroySymtable(parser->global_symtable);
            exit(ERROR_SEMANTIC_TYPE_COMPATIBILITY);
        }

        if (!is_underscore)
            fprintf(stdout, "POPS LF@%s\n", var->name);
        CLEARS

        if (is_underscore)
            return;

        // if the variable doesn't have a type yet, derive it from the expression (TODO: Add check for invalid types, etc.)
        if (var->type == VOID_TYPE)
            var->type = expr_type;

        var->defined = true;
    }
}

bool AreTypesCompatible(DATA_TYPE expected, DATA_TYPE got)
{
    return (expected == got ||
            (expected == U8_ARRAY_NULLABLE_TYPE && got == U8_ARRAY_TYPE) ||
            (expected == INT32_NULLABLE_TYPE && got == INT32_TYPE) ||
            (expected == DOUBLE64_NULLABLE_TYPE && got == DOUBLE64_TYPE) ||
            (got == NULL_DATA_TYPE && IsNullable(expected)));
}

void ThrowAwayExpression(Parser *parser)
{
    // We are after the '_' token
    CheckTokenTypeVector(parser, ASSIGNMENT);
    VariableAssignment(parser, NULL, true);
}

void FunctionToVariable(Parser *parser, VariableSymbol *var, FunctionSymbol *func, bool is_underscore)
{
    // If the variable is not thrown away, we have to perform semantic analysis
    if (!is_underscore)
    {
        // Function has no return value
        if (func->return_type == VOID_TYPE)
        {
            SymtableStackDestroy(parser->symtable_stack);
            DestroySymtable(parser->global_symtable);
            DestroyTokenVector(stream);
            ErrorExit(ERROR_SEMANTIC_TYPE_COMPATIBILITY, "Line %d: Assigning return value of void function to variable.", parser->line_number);
        }

        // Var type yet to be derived
        if (var->type == VOID_TYPE)
            var->type = func->return_type;

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
    if (!is_underscore)
        fprintf(stdout, "POPS LF@%s\n", var->name);
    CLEARS
}

void ProgramBody(Parser *parser)
{
    Token *token;
    FunctionSymbol *func;
    VariableSymbol *var;

    while (true)
    {
        token = GetNextToken(parser);
        // Code can't be outside of a function, so if we aren't in a function the next token has to be pub

        if (!parser->current_function)
        {
            if (token->token_type != EOF_TOKEN && token->keyword_type != PUB)
            {
                SymtableStackDestroy(parser->symtable_stack);
                DestroySymtable(parser->global_symtable);
                DestroyTokenVector(stream);
                ErrorExit(ERROR_SYNTACTIC, "Expected 'pub' keyword at line %d", parser->line_number);
            }
        }

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
                PrintError("Unexpected token \"%s\" at line %d", token->attribute, parser->line_number);
                SymtableStackDestroy(parser->symtable_stack);
                DestroySymtable(parser->global_symtable);
                DestroyTokenVector(stream);
                exit(ERROR_SYNTACTIC);
            }
            break;

        case R_CURLY_BRACKET:
            --(parser->nested_level);
            SymtableStackRemoveTop(parser);
            parser->symtable = SymtableStackTop(parser->symtable_stack);
            return;

        case IDENTIFIER_TOKEN:
            // Can be a embedded function, or a defined function, or a called function, or a reassignment to a variable
            if (!strcmp(token->attribute, "ifj"))
            {
                func = IsEmbeddedFunction(parser);
                stream_index += 2;                        // skip the 'ifj' and the '.' token
                EmbeddedFunctionCall(parser, func, NULL); // Void type since we aren't assigning the result anywhere
            }

            else if ((var = IsVariableAssignment(token, parser)) != NULL)
            {
                // Move past the ID =
                token = GetNextToken(parser);
                token = GetNextToken(parser);

                // Variable assignment
                VariableAssignment(parser, var, false);
            }

            else if (IsFunctionCall(parser))
            {
                // Store the function name in a temporary variable since we will be moving the tokens forward
                char *tmp_func_name = strdup(token->attribute);

                // Move past the ID(
                stream_index += 2;

                // Function call
                FunctionCall(parser, FindFunctionSymbol(parser->global_symtable, tmp_func_name), tmp_func_name, VOID_TYPE);
                free(tmp_func_name);
            }

            // Check for an undefinded variable case
            else if(!SymtableStackFindVariable(parser->symtable_stack, token->attribute))
            {
                PrintError("Error in semantic analysis: Line %d: Undefined variable \"%s\"",
                           parser->line_number, token->attribute);
                CLEANUP
                exit(ERROR_SEMANTIC_UNDEFINED);
            }

            break;

        case UNDERSCORE_TOKEN:
            ThrowAwayExpression(parser);
            break;

        case EOF_TOKEN:
            // EOF can't be inside of a code block
            if (parser->current_function)
            {
                SymtableStackDestroy(parser->symtable_stack);
                DestroySymtable(parser->global_symtable);
                DestroyTokenVector(stream);
                ErrorExit(ERROR_SYNTACTIC, "Unexpected end of file");
            }

            else
                return;

            break; // Shut up gcc

        default:
            PrintError("Unexpected token \"%s\" at line %d", token->attribute, parser->line_number);
            SymtableStackDestroy(parser->symtable_stack);
            DestroySymtable(parser->global_symtable);
            DestroyTokenVector(stream);
            exit(ERROR_SYNTACTIC);
        }
    }
}

void PrintEqualTokens()
{
    for (int i = 0; i < stream->length; i++)
    {
        for (int j = i + 1; j < stream->length; j++)
        {
            if (i != j && stream->token_string[i] == stream->token_string[j])
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
    parser.current_function = NULL;

    // Check for the header
    ProgramBegin(&parser);
    Header(&parser);

    // Check for the presence of a a main function
    if (!parser.has_main)
    {
        SymtableStackDestroy(parser.symtable_stack);
        DestroySymtable(parser.global_symtable);
        DestroyTokenVector(stream);
        ErrorExit(ERROR_SEMANTIC_UNDEFINED, "Main function not found");
    }

    // Second go-through of the stream file, parse the program body
    ProgramBody(&parser);

    SymtableStackDestroy(parser.symtable_stack);
    DestroySymtable(parser.global_symtable);
    DestroyTokenVector(stream);
    return 0;
}
