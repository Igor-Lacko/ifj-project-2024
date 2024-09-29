#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scanner.h"
#include "error.h"
#include "parser.h"
#include "symtable.h"

void CheckTokenType(int *line_number, TOKEN_TYPE type)
{
    Token *token;
    char char_types[30][15] = {"Identifier", "_", "Keyword", "integer", "double", "u8", "string", "=", "*", "/", "+", "-", "==", "!=", "<", ">", "<=", ">=", "(", ")", "{", "}", "|", ";", ",", ".", ":", "@", "EOF"};
    if ((token = GetNextToken(line_number))->token_type != type)
    {
        DestroyToken(token);
        ErrorExit(ERROR_SYNTACTIC, " Expected '%s' at line %d",
                  char_types[type], *line_number);
    }
    DestroyToken(token);
}

void CheckKeywordType(int *line_number, KEYWORD_TYPE type)
{
    Token *token;
    char keyword_types[13][20] = {"const", "else", "fn", "if", "i32", "f64", "null", "pub", "return", "u8", "var", "void", "while"};
    if ((token = GetNextToken(line_number))->keyword_type != type)
    {
        DestroyToken(token);
        ErrorExit(ERROR_SYNTACTIC, "Expected '%s' keyword at line %d",
                  keyword_types[type], *line_number);
    }
    DestroyToken(token);
}

Token *CheckAndReturnToken(int *line_number, TOKEN_TYPE type)
{
    Token *token;
    char char_types[30][15] = {"Identifier", "_", "Keyword", "integer", "double", "u8", "string", "=", "*", "/", "+", "-", "==", "!=", "<", ">", "<=", ">=", "(", ")", "{", "}", "|", ";", ",", ".", ":", "@", "EOF"};
    if ((token = GetNextToken(line_number))->token_type != type)
    {
        DestroyToken(token);
        ErrorExit(ERROR_SYNTACTIC, " Expected '%s' at line %d",
                  char_types[type], *line_number);
    }
    return token;
}

void ProgramBody(Parser *parser);

// const ifj = @import("ifj24.zig");
void Header(Parser *parser)
{
    CheckKeywordType(&parser->line_number, CONST);
    CheckTokenType(&parser->line_number, IDENTIFIER_TOKEN);
    CheckTokenType(&parser->line_number, ASSIGNMENT);
    CheckTokenType(&parser->line_number, AT_TOKEN);
    CheckTokenType(&parser->line_number, IDENTIFIER_TOKEN);
    CheckTokenType(&parser->line_number, L_ROUND_BRACKET);
    CheckTokenType(&parser->line_number, LITERAL_TOKEN);
    CheckTokenType(&parser->line_number, R_ROUND_BRACKET);
    CheckTokenType(&parser->line_number, SEMICOLON);
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
void Parameters(Parser *parser)
{
    Token *token;

    // loops through all parameters
    while (1)
    {
        token = GetNextToken(&parser->line_number);
        if (token->token_type == R_ROUND_BRACKET) // reached ')' so all parameters are checked
            break;

        if (token->token_type == EOF_TOKEN) // reached EOF without ')'
        {
            DestroyToken(token);
            ErrorExit(ERROR_SYNTACTIC, "Didnt you forget ) at line %d ?", parser->line_number);
        }

        // id : data_type
        if (token->token_type == IDENTIFIER_TOKEN)
        {
            DestroyToken(token);

            CheckTokenType(&parser->line_number, COLON_TOKEN);

            if ((token = GetNextToken(&parser->line_number))->token_type != KEYWORD && (token->keyword_type != I32 || token->keyword_type != F64 || token->keyword_type != U8))
            {
                DestroyToken(token);
                ErrorExit(ERROR_SYNTACTIC, "Expected data type at line %d", parser->line_number);
            }
            DestroyToken(token);

            // checks if there is another parameter
            if ((token = GetNextToken(&parser->line_number))->token_type != COMMA_TOKEN)
            {
                if (token->token_type == R_ROUND_BRACKET) // no more parameters
                    break;
                DestroyToken(token);
                ErrorExit(ERROR_SYNTACTIC, "Expected ',' or ')' at line %d", parser->line_number);
            }
            DestroyToken(token);
        }
        else
        {
            DestroyToken(token);
            ErrorExit(ERROR_SYNTACTIC, "Expected identifier at line %d", parser->line_number);
        }
    }
    DestroyToken(token);
}

// pub fn id ( seznam_parametrů ) návratový_typ {
// sekvence_příkazů
// }
void Function(Parser *parser)
{
    CheckKeywordType(&parser->line_number, FN);
    CheckTokenType(&parser->line_number, IDENTIFIER_TOKEN);
    CheckTokenType(&parser->line_number, L_ROUND_BRACKET);
    Parameters(parser); // params with )

    Token *token; // return type

    if ((token = GetNextToken(&parser->line_number))->token_type != KEYWORD || (token->keyword_type != I32 && token->keyword_type != F64 && token->keyword_type != U8 && token->keyword_type != VOID))
    {
        DestroyToken(token);
        ErrorExit(ERROR_SYNTACTIC, "Expected data type at line %d", parser->line_number);
    }
    DestroyToken(token);

    CheckTokenType(&parser->line_number, L_CURLY_BRACKET);

    ProgramBody(parser); // function body

    CheckTokenType(&parser->line_number, R_CURLY_BRACKET);
}

// if(expression)|id|{} else{}
// if(expression){}else{}
void IfElse(Parser *parser)
{
    CheckTokenType(&parser->line_number, L_ROUND_BRACKET);
    // expression
    Expression(parser);
    CheckTokenType(&parser->line_number, R_ROUND_BRACKET);

    Token *token;
    token = GetNextToken(&parser->line_number);
    if (token->token_type == VERTICAL_BAR_TOKEN)
    {
        DestroyToken(token);
        Expression(parser);
        CheckTokenType(&parser->line_number, VERTICAL_BAR_TOKEN);
    }
    else if (token->token_type == L_CURLY_BRACKET)
    {
        DestroyToken(token);
    }
    else
    {
        DestroyToken(token);
        ErrorExit(ERROR_SYNTACTIC, "Expected '|' or '{' at line %d", parser->line_number);
    }

    ProgramBody(parser);
    CheckTokenType(&parser->line_number, R_CURLY_BRACKET);

    CheckKeywordType(&parser->line_number, ELSE);
    CheckTokenType(&parser->line_number, L_CURLY_BRACKET);
    ProgramBody(parser);
    CheckTokenType(&parser->line_number, R_CURLY_BRACKET);
}

void WhileLoop(Parser *parser)
{
    CheckTokenType(&parser->line_number, L_ROUND_BRACKET);
    // expression
    Expression(parser);
    CheckTokenType(&parser->line_number, R_ROUND_BRACKET);

    CheckTokenType(&parser->line_number, L_CURLY_BRACKET);
    ProgramBody(parser);
    CheckTokenType(&parser->line_number, R_CURLY_BRACKET);
}

// const id = expression;
void ConstDeclaration(Parser *parser)
{
    Token *token;
    token = CheckAndReturnToken(&parser->line_number, IDENTIFIER_TOKEN);

    // add to symtable
    VariableSymbol *var = VariableSymbolInit();
    var->name = strdup(token->attribute);
    var->is_declared = true;
    var->type = INT32_TYPE;
    var->value = NULL;
    if (!InsertVariableSymbol(parser->symtable, var))
        ErrorExit(ERROR_SEMANTIC_REDEFINED, "Variable %s already declared", var->name);

    // VariableSymbol *var2 = FindVariableSymbol(parser->symtable, var->name);
    // if (var2 == NULL)
    //     ErrorExit(ERROR_SEMANTIC_UNDEFINED, "Variable %s not found", var->name);

    PrintTable(parser->symtable);

    DestroyToken(token);

    CheckTokenType(&parser->line_number, ASSIGNMENT);
    // expression
    Expression(parser);
    CheckTokenType(&parser->line_number, SEMICOLON);
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
                IfElse(parser);
            }
            // start of while loop
            else if (token->keyword_type == WHILE)
            {
                DestroyToken(token);
                WhileLoop(parser);
            }
            else if (token->keyword_type == CONST)
            {
                DestroyToken(token);
                ConstDeclaration(parser);
            }
            else
            {
                DestroyToken(token);
                // ErrorExit(ERROR_SYNTACTIC, "Unexpected at line %d", *line_number);
            }
            break;
        case R_CURLY_BRACKET:
            DestroyToken(token);
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

int main()
{
    Parser parser;
    parser.line_number = 1;
    parser.has_main = false;
    parser.in_function = false;
    parser.symtable = InitSymtable(TABLE_COUNT);
    Header(&parser);
    ProgramBody(&parser);
    printf("\033[1m\033[32m"
           "SYNTAX OK\n"
           "\033[0m");
    DestroySymtable(parser.symtable);
    return 0;
}
