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

Parser InitParser()
{
    Parser parser =
        {
            .current_function = NULL,
            .global_symtable = InitSymtable(TABLE_COUNT),
            .has_main = false,
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

void ProgramBegin()
{
    // initial codegen instructions
    IFJCODE24
    InitRegisters(); // registers exist in main
    JUMP("main")
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
        DestroySymtable(parser->global_symtable);
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
        DestroySymtable(parser->global_symtable);
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
        DestroySymtable(parser->global_symtable);
        ErrorExit(ERROR_SYNTACTIC, " Expected '%s' at line %d",
                  char_types[type], parser->line_number);
    }
    return token;
}

Token *CheckAndReturnKeyword(Parser *parser, KEYWORD_TYPE type)
{
    Token *token;
    char keyword_types[13][20] = {"const", "else", "fn", "if", "i32", "f64", "null", "pub", "return", "u8", "var", "void", "while"};
    if ((token = GetNextToken(&parser->line_number))->keyword_type != type)
    {
        DestroyToken(token);
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
        Token *tok2 = CheckAndReturnToken(parser, ASSIGNMENT);
        UngetToken(tok2);
        UngetToken(token);
        return var;
    }

    return NULL;
}

bool IsFunctionCall(Token *token, Parser *parser)
{
    Token *braces = GetNextToken(&parser->line_number);
    if (braces->token_type == L_ROUND_BRACKET)
    {
        UngetToken(braces);
        UngetToken(token);
        return true;
    }

    return false;
}

void PrintStream()
{
    int c;
    while((c=getchar()) != EOF) putchar(c);
    exit(SUCCESS);
}

void PrintStreamTokens(Parser *parser)
{
    Token *token;
    while((token=GetNextToken(&parser->line_number))->token_type != EOF_TOKEN)
    {
        PrintToken(token);
        DestroyToken(token);
    }
    exit(SUCCESS);
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

// const/var id = expression;
void VarDeclaration(Parser *parser, bool is_const)
{
    Token *token;
    token = CheckAndReturnToken(parser, IDENTIFIER_TOKEN);

    // add to symtable
    VariableSymbol *var = VariableSymbolInit();
    var->name = strdup(token->attribute);
    var->is_const = is_const;
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

    // const/var a : i32 = 5;
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

    // Assign to the variable
    VariableAssignment(parser, var);
}

// TODO: revamp this to work accordingly with function_parser.c
void FunctionCall(Parser *parser, FunctionSymbol *func, const char *fun_name, DATA_TYPE expected_return)
{
    CREATEFRAME
    // Check if the function exists
    if (func == NULL)
    {
        func = FunctionSymbolInit();
        func->name = strdup(fun_name);
        func->num_of_parameters = -1; // Unspecified number of parameters
        func->parameters = NULL;
        func->return_type = expected_return;

        // Create a new value in the symtable
        (void)InsertFunctionSymbol(parser->global_symtable, func);
    }

    // Invalid return value
    else if (func->return_type != expected_return)
    {
        PrintError("Line %d: Invalid return type for function \"%s\"",
                   parser->line_number, fun_name);
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
    }

    CheckTokenType(parser, L_ROUND_BRACKET);
    ParametersOnCall(parser, func);
    FUNCTIONCALL(func->name)
    func->was_called = true;
}

void ParametersOnCall(Parser *parser, FunctionSymbol *func)
{
    //Token *token;
    //int loaded = 0;
    //VariableSymbol *symb1; // for identifier parameter checking
    (void)func;
    (void)parser;
}

void FunctionDefinition(Parser *parser)
{
    // These next few lines should ALWAYS run succesfully, since the function parser already checks them
    CheckKeywordType(parser, FN);
    Token *token = CheckAndReturnToken(parser, IDENTIFIER_TOKEN);
    FunctionSymbol *func = FindFunctionSymbol(parser->global_symtable, token->attribute);
    DestroyToken(token);

    // Generate code for the function label
    FUNCTIONLABEL(func->name)
    PUSHFRAME

    // Skip all tokens until the function body begins (so after '{')
    while((token=GetNextToken(&parser->line_number))->token_type != L_CURLY_BRACKET)
        DestroyToken(token);

    ProgramBody(parser);
}

void FunctionReturn(Parser *parser)
{
    Token *token;
    if (!parser->current_function) // In main, return exits the program
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

void VariableAssignment(Parser *parser, VariableSymbol *var)
{
    if (var->is_const && var->defined)
    {
        PrintError("Error in semantic analysis: Line %d: Reassignment of constant variable \"%s\"",
                   parser->line_number, var->name);
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        exit(ERROR_SEMANTIC_REDEFINED);
    }

    if (IsFunctionCall(GetNextToken(&parser->line_number), parser))
    {
    }

    DATA_TYPE expr_type;
    TokenVector *postfix = InfixToPostfix(parser);

    if ((expr_type = GeneratePostfixExpression(parser, postfix, var)) != var->type && var->type != VOID_TYPE)
    {
        PrintError("Error in semantic analysis: Line %d: Assigning invalid type to variable \"%s\"",
                   parser->line_number, var->name);
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        exit(ERROR_SEMANTIC_TYPE_COMPATIBILITY);
    }

    // if the variable doesn't have a type yet, derive it from the expression (TODO: Add check for invalid types, etc.)
    if (var->type == VOID_TYPE)
        var->type = expr_type;

    var->defined = true;

    CheckTokenType(parser, SEMICOLON);
}

void FunctionToVariable(Parser *parser, VariableSymbol *var, FunctionSymbol *func)
{
    // Function has no return value
    if (func->return_type == VOID_TYPE)
    {
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
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
        DestroySymtable(parser->global_symtable);
        ErrorExit(ERROR_SEMANTIC_TYPE_COMPATIBILITY, "Line %d: Invalid type in assigning to variable");
    }

    /*
        ---- Compatible return type or has to be derived ----
        - The return value is on top of the data stack
        - Note: Default IFJ24 doesn't have booleans, maybe expand this for the BOOLTHEN extension later?
    */
    fprintf(stdout, "POPS LF@%s\n", var->name);
}

void ProgramBody(Parser *parser)
{
    Token *token;
    FunctionSymbol *func;
    VariableSymbol *var;
    while (true)
    {
        token = GetNextToken(&parser->line_number);
        switch (token->token_type)
        {
        case KEYWORD:
            // start of function declaration, either skip it or throw an error
            if (token->keyword_type == PUB)
            {
                DestroyToken(token);
                ++(parser->nested_level);
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
                VarDeclaration(parser, true);
            }
            else if (token->keyword_type == VAR)
            {
                DestroyToken(token);
                VarDeclaration(parser, false);
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
            // Can be a embedded function, or a defined function, or a called function, or a reassignment to a variable
            if (!strcmp(token->attribute, "ifj"))
            {
                printf("looking man\n");
                func = IsEmbeddedFunction(parser);
                FunctionCall(parser, func, func->name, VOID_TYPE); // Void type since we aren't assigning the result anywhere
            }

            else if ((var = IsVariableAssignment(token, parser)) != NULL)
            {
                // Move past the ID =
                token = GetNextToken(&parser->line_number);
                DestroyToken(token);
                token = GetNextToken(&parser->line_number);

                // Variable assignment
                VariableAssignment(parser, var);
            }

            else if (IsFunctionCall(token, parser))
            {
                // Store the function name in a temporary variable since we will be moving the tokens forward
                char *tmp_func_name = strdup(token->attribute);

                // Move past the ID(
                token = GetNextToken(&parser->line_number);
                DestroyToken(token);
                token = GetNextToken(&parser->line_number);

                // Function call
                FunctionCall(parser, FindFunctionSymbol(parser->global_symtable, tmp_func_name), tmp_func_name, VOID_TYPE);
                free(tmp_func_name);
            }

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


int main()
{    
    // parser instance
    Parser parser = InitParser();
    //ProgramBegin(&parser);
    ParseFunctions(&parser);
    UngetStream(&parser);
    Header(&parser);

    ProgramBody(&parser);
    /*printf("\033[1m\033[32m"
           "SYNTAX OK\n"
           "\033[0m");*/

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

