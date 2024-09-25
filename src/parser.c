#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scanner.h"
#include "error.h"

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

void ProgramBody(int *line_number);

// const ifj = @import("ifj24.zig");
void Header(int *line_number)
{
    CheckKeywordType(line_number, CONST);
    CheckTokenType(line_number, IDENTIFIER_TOKEN);
    CheckTokenType(line_number, ASSIGNMENT);
    CheckTokenType(line_number, AT_TOKEN);
    CheckTokenType(line_number, IDENTIFIER_TOKEN);
    CheckTokenType(line_number, L_ROUND_BRACKET);
    CheckTokenType(line_number, LITERAL_TOKEN);
    CheckTokenType(line_number, R_ROUND_BRACKET);
    CheckTokenType(line_number, SEMICOLON);
}

void Expression(int *line_number)
{
    Token *token;
    while ((token = GetNextToken(line_number))->token_type != SEMICOLON)
    {
        if (token->token_type == EOF_TOKEN)
        {
            DestroyToken(token);
            ErrorExit(ERROR_SYNTACTIC, "Expected ';' at line %d", *line_number);
        }

        // TODO: complete expressions, for now just skip
        DestroyToken(token);
    }
    DestroyToken(token);
}

// checks all parameters
void Parameters(int *line_number)
{
    Token *token;

    while (1)
    {
        token = GetNextToken(line_number);
        if (token->token_type == R_ROUND_BRACKET) // reached ')' so all parameters are checked
            break;

        if (token->token_type == EOF_TOKEN) // reached EOF without ')'
        {
            DestroyToken(token);
            ErrorExit(ERROR_SYNTACTIC, "Didnt you forget ) at line %d ?", *line_number);
        }

        // id : data_type
        if (token->token_type == IDENTIFIER_TOKEN)
        {
            DestroyToken(token);

            CheckTokenType(line_number, COLON_TOKEN);

            if ((token = GetNextToken(line_number))->token_type != KEYWORD && (token->keyword_type != I32 || token->keyword_type != F64 || token->keyword_type != U8))
            {
                DestroyToken(token);
                ErrorExit(ERROR_SYNTACTIC, "Expected data type at line %d", *line_number);
            }
            DestroyToken(token);

            // TODO: fix the last char comma problem
            // param : data_type , )
            // UPDATE: maybe not needed page 6,8 of the pdf

            // checks if there is another parameter
            if ((token = GetNextToken(line_number))->token_type != COMMA_TOKEN)
            {
                if (token->token_type == R_ROUND_BRACKET) // no more parameters
                    break;
                DestroyToken(token);
                ErrorExit(ERROR_SYNTACTIC, "Expected ',' or ')' at line %d", *line_number);
            }
            DestroyToken(token);
        }
        else
        {
            DestroyToken(token);
            ErrorExit(ERROR_SYNTACTIC, "Expected identifier at line %d", *line_number);
        }
    }
    DestroyToken(token);
}

// pub fn id ( seznam_parametrů ) návratový_typ {
// sekvence_příkazů
// }
void Function(int *line_number)
{
    CheckKeywordType(line_number, FN);
    CheckTokenType(line_number, IDENTIFIER_TOKEN);
    CheckTokenType(line_number, L_ROUND_BRACKET);
    Parameters(line_number); // params with )

    Token *token; // return type
    if ((token = GetNextToken(line_number))->token_type != KEYWORD && (token->keyword_type != I32 || token->keyword_type != F64 || token->keyword_type != U8 || token->keyword_type != VOID))
    {
        DestroyToken(token);
        ErrorExit(ERROR_SYNTACTIC, "Expected data type at line %d", *line_number);
    }
    DestroyToken(token);

    CheckTokenType(line_number, L_CURLY_BRACKET);

    ProgramBody(line_number); // function body

    CheckTokenType(line_number, R_CURLY_BRACKET);
}

// TODO: if(expression)|id|{} else{}
// if(expression){}else{}
void IfElse(int *line_number)
{
    CheckTokenType(line_number, L_ROUND_BRACKET);
    // expression
    CheckTokenType(line_number, R_ROUND_BRACKET);

    CheckTokenType(line_number, L_CURLY_BRACKET);
    ProgramBody(line_number);
    CheckTokenType(line_number, R_CURLY_BRACKET);

    CheckKeywordType(line_number, ELSE);
    CheckTokenType(line_number, L_CURLY_BRACKET);
    ProgramBody(line_number);
    CheckTokenType(line_number, R_CURLY_BRACKET);
}

void WhileLoop(int *line_number)
{
    CheckTokenType(line_number, L_ROUND_BRACKET);
    // expression
    CheckTokenType(line_number, R_ROUND_BRACKET);

    CheckTokenType(line_number, L_CURLY_BRACKET);
    ProgramBody(line_number);
    CheckTokenType(line_number, R_CURLY_BRACKET);
}

void ProgramBody(int *line_number)
{
    Token *token;
    while ((token = GetNextToken(line_number))->token_type != EOF_TOKEN)
    {
        switch (token->token_type)
        {
        case KEYWORD:
            // start of function declaration
            if (token->keyword_type == PUB)
            {
                DestroyToken(token);
                Function(line_number);
            }
            // start of if-else block
            else if (token->keyword_type == IF)
            {
                DestroyToken(token);
                IfElse(line_number);
            }
            else if (token->keyword_type == WHILE)
            {
                DestroyToken(token);
                WhileLoop(line_number);
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

/*int main()
{
    int line_number = 1;
    Header(&line_number);
    ProgramBody(&line_number);
    printf("\033[1m\033[32m"
           "SYNTAX OK\n"
           "\033[0m");
    return 0;
}*/
