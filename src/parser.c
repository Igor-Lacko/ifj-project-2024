#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scanner.h"
#include "error.h"
#include "parser.h"
#include "symtable.h"
#include "expression_parser.h"

// checks if the next token is of the expected type
void CheckTokenType(Parser *parser, TOKEN_TYPE type)
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

void ProgramBody(Parser *parser);

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
void Parameters(Parser *parser, char *function_name)
{
    Token *token;
    int param_count = 0;
    FunctionSymbol *func = FindFunctionSymbol(parser->symtable, function_name);

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
            ErrorExit(ERROR_SYNTACTIC, "Didnt you forget ) at line %d ?", parser->line_number);
        }

        // id : data_type
        if (token->token_type == IDENTIFIER_TOKEN)
        {
            // add to symtable
            VariableSymbol *var = VariableSymbolInit();
            var->name = strdup(token->attribute);
            var->value = NULL;
            var->is_const = false;

            DestroyToken(token);

            // TODO: possible var leak
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
            func->parameters = realloc(func->parameters, (param_count + 1) * sizeof(VariableSymbol *));
            func->parameters[param_count] = var;
            param_count++;

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
    func->num_of_parameters = param_count;
    DestroyToken(token);
}

// pub fn id ( seznam_parametrů ) návratový_typ {
// sekvence_příkazů
// }
void Function(Parser *parser)
{
    CheckKeywordType(parser, FN);
    Token *token;
    token = CheckAndReturnToken(parser, IDENTIFIER_TOKEN);

    // add an empty function to the symtable
    FunctionSymbol *func = FunctionSymbolInit();
    func->name = strdup(token->attribute);
    func->num_of_parameters = 0;
    func->parameters = NULL;
    func->return_type = VOID_TYPE;
    func->return_value = NULL;
    DestroyToken(token);

    if (!InsertFunctionSymbol(parser->symtable, func))
    {
        DestroySymtable(parser->symtable);
        ErrorExit(ERROR_SEMANTIC_REDEFINED, "Function %s already declared", func->name);
    }

    // check if the main function is present
    if (!strcmp(func->name, "main"))
        parser->has_main = true;

    CheckTokenType(parser, L_ROUND_BRACKET);
    Parameters(parser, func->name); // params with )

    if ((token = GetNextToken(&parser->line_number))->token_type != KEYWORD || (token->keyword_type != I32 && token->keyword_type != F64 && token->keyword_type != U8 && token->keyword_type != VOID))
    {
        DestroyToken(token);
        DestroySymtable(parser->symtable);
        ErrorExit(ERROR_SYNTACTIC, "Expected data type at line %d", parser->line_number);
    }

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
    DestroyToken(token);

    CheckTokenType(parser, L_CURLY_BRACKET);
    // symtable_real is being pushed into a stack
    Symtable *symtable_real = InitSymtable(TABLE_COUNT);
    // push the global symtable
    SymtableStackPush(parser->symtable_stack, symtable_real);

    parser->in_function = true;

    ProgramBody(parser); // function body

    parser->in_function = false;

    CheckTokenType(parser, R_CURLY_BRACKET);
}

// if(expression)|id|{} else{}
// if(expression){}else{}
void IfElse(Parser *parser)
{
    Symtable *symtable = InitSymtable(TABLE_COUNT);
    SymtableStackPush(parser->symtable_stack, symtable);

    CheckTokenType(parser, L_ROUND_BRACKET);
    // expression
    TokenVector *postfix = InfixToPostfix(parser);
    ExpressionReturn *return_value = EvaluatePostfixExpression(postfix, *parser);
    CheckTokenType(parser, R_ROUND_BRACKET);

    bool if_block = true; // flag to indicate whether to do the if else block

    // null value or false boolean expression cause the else block to be executed
    if(return_value -> type == NULL_DATA_TYPE) if_block = false;
    if(return_value -> type == BOOLEAN && *(bool *)(return_value -> value) == false) if_block = false;


    Token *token;
    token = GetNextToken(&parser->line_number);
    if (token->token_type == VERTICAL_BAR_TOKEN) // null-if block , the value can't be boolean
    {

        DestroyToken(token);

        if(return_value -> type == BOOLEAN){ // invalid expression type, can't assign to variable
            DestroyToken(token);
            DestroySymtable(parser -> symtable);
            DestroyExpressionReturn(return_value);
            ErrorExit(ERROR_SEMANTIC_TYPE_COMPATIBILITY, "Line %d: Invalid expression type 'boolean' in the If-Else |ID| block");
        }

        VariableSymbol *symbol; // should not be there
        Token *id = CheckAndReturnToken(parser, IDENTIFIER_TOKEN);
        if((symbol = FindVariableSymbol(parser -> symtable, id -> attribute)) == NULL) { // redefinition of variable
            DestroyToken(token);
            DestroyToken(id);
            DestroySymtable(parser -> symtable);
            DestroyExpressionReturn(return_value);
            ErrorExit(ERROR_SEMANTIC_REDEFINED, "Line %d: Redefinition of variable");
        }

        // create a new symbol
        symbol = VariableSymbolInit();
        symbol -> name = id -> attribute;
        symbol -> type = return_value -> type;
        symbol -> nullable = false;
        symbol -> is_const = false;
        switch(symbol -> type) { //add value depending on the type
            case INT32_TYPE:
                symbol -> value = (int *)(return_value -> value);
                break;

            case DOUBLE64_TYPE:
                symbol -> value = (double *)(return_value -> value);
                break;

            case U8_ARRAY_TYPE:
                symbol -> value = (char **)(return_value -> value);
                break;

            default:
                //THIS SHOULD NEVER HAPPEN!
                ErrorExit(ERROR_INTERNAL, "Invalid switch case!!!");
        }

        InsertVariableSymbol(parser->symtable, symbol);
        CheckTokenType(parser, VERTICAL_BAR_TOKEN);

        if(if_block){
            ProgramBody(parser);
            CheckTokenType(parser, R_CURLY_BRACKET);
        }

        else{
            //code that skips if block
            ProgramBody(parser);
        }
    }
    else if (token->token_type == L_CURLY_BRACKET)
    {
        DestroyToken(token);
    }
    else
    {
        DestroyToken(token);
        DestroySymtable(parser->symtable);
        ErrorExit(ERROR_SYNTACTIC, "Expected '|' or '{' at line %d", parser->line_number);
    }

    if(if_block) ProgramBody(parser);
    CheckTokenType(parser, R_CURLY_BRACKET);
    CheckKeywordType(parser, ELSE); 
    CheckTokenType(parser, L_CURLY_BRACKET);
    // code that skips else block
    if(!if_block) ProgramBody(parser);
    CheckTokenType(parser, R_CURLY_BRACKET);
}

void WhileLoop(Parser *parser)
{
    Symtable *symtable = InitSymtable(TABLE_COUNT);
    SymtableStackPush(parser->symtable_stack, symtable);

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
    var->value = NULL;
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
    // expression
    TokenVector *postfix = InfixToPostfix(parser);
    ExpressionReturn *ret_value = EvaluatePostfixExpression(postfix, *parser);

    // add the computed value to the variable
    var->type = ret_value->type;
    var->value = ret_value->value;
    PrintResult(ret_value);
    DestroyExpressionReturn(ret_value);
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
    var->value = NULL;
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
    }
    // else if (token->token_type != ASSIGNMENT)
    // {
    //     DestroyToken(token);
    //     DestroySymtable(parser->symtable);
    //     ErrorExit(ERROR_SYNTACTIC, "Expected '=' at line %d", parser->line_number);
    // }
    PrintToken(token);

    // PrintToken(GetNextToken(&parser->line_number));

    // expression
    TokenVector *postfix = InfixToPostfix(parser);
    ExpressionReturn *ret_value = EvaluatePostfixExpression(postfix, *parser);
    var->value = ret_value->value;

    printf("var type: %d, ret_value type: %d\n", var->type, ret_value->type);
    // check if the types are compatible
    if (var->type != ret_value->type && var->type != VOID_TYPE)
    {
        DestroyExpressionReturn(ret_value);
        DestroySymtable(parser->symtable);
        ErrorExit(ERROR_SEMANTIC_TYPE_COMPATIBILITY, "Incompatible types at line %d", parser->line_number);
    }

    DestroyExpressionReturn(ret_value);

    PrintResult(ret_value);
    CheckTokenType(parser, SEMICOLON);
}

void ProgramBody(Parser *parser)
{
    Token *token;
    while ((token = GetNextToken(&parser->line_number))->token_type != EOF_TOKEN)
    {
        switch (token->token_type)
        {
        case KEYWORD:
            // start of function declaration
            if (token->keyword_type == PUB)
            {
                DestroyToken(token);
                Function(parser);
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
            else
            {
                DestroyToken(token);
                // ErrorExit(ERROR_SYNTACTIC, "Unexpected at line %d", *line_number);
            }
            break;
        case R_CURLY_BRACKET:
            DestroyToken(token);
            --(parser->nested_level);
            SymtableStackPrint(parser->symtable_stack);
            SymtableStackPop(parser->symtable_stack);
            ungetc('}', stdin);
            return;

        default:
            // ErrorExit(ERROR_SYNTACTIC, "Unexpected token at line %d", *line_number);
            DestroyToken(token);
            break;
        }
    }
    DestroyToken(token);
}

#ifndef IFJ24_DEBUG // not for debugs

int main()
{
    Parser parser;
    parser.line_number = 1;
    parser.has_main = false;
    parser.in_function = false;
    parser.nested_level = 0;
    parser.symtable = InitSymtable(TABLE_COUNT);
    parser.symtable_stack = SymtableStackInit();

    Header(&parser);
    ProgramBody(&parser);
    printf("\033[1m\033[32m"
           "SYNTAX OK\n"
           "\033[0m");

    // check if the main function is present
    if (!parser.has_main)
    {
        DestroySymtable(parser.symtable);
        ErrorExit(ERROR_SEMANTIC_UNDEFINED, "Main function not found");
    }
    // PrintTable(parser.symtable);
    SymtableStackPrint(parser.symtable_stack);
    printf("symstack lenght: %ld\n", parser.symtable_stack->size);
    // SymtableStackDestroy(parser.symtable_stack);
    return 0;
}

#endif