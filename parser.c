#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scanner.h"
#include "error.h"

void PrintToken(Token *token)
{
    switch (token->token_type)
    {
    case IDENTIFIER_TOKEN:
        printf("Type: Identifier token, attribute: %s", (char *)(token->attribute));
        break;

    case INTEGER_32:
        printf("Type: Int32 token, attribute: %d", *(int *)(token->attribute));
        break;

    case DOUBLE_64:
        printf("Type: F64 token, attribute: %lf", *(double *)(token->attribute));
        break;

    case ARRAY_TOKEN:
        printf("Type: []");
        break;

    case ASSIGNMENT:
        printf("Type: =");
        break;

    case MULTIPLICATION_OPERATOR:
        printf("Type: *");
        break;

    case DIVISION_OPERATOR:
        printf("Type: /");
        break;

    case ADDITION_OPERATOR:
        printf("Type: +");
        break;

    case SUBSTRACTION_OPERATOR:
        printf("Type: -");
        break;

    case EQUAL_OPERATOR:
        printf("Type: ==");
        break;

    case NOT_EQUAL_OPERATOR:
        printf("Type: !=");
        break;

    case LESS_THAN_OPERATOR:
        printf("Type: <");
        break;

    case LARGER_THAN_OPERATOR:
        printf("Type: >");
        break;

    case LESSER_EQUAL_OPERATOR:
        printf("Type: <=");
        break;

    case LARGER_EQUAL_OPERATOR:
        printf("Type: >=");
        break;

    case L_ROUND_BRACKET:
        printf("Type: (");
        break;

    case R_ROUND_BRACKET:
        printf("Type: )");
        break;

    case L_CURLY_BRACKET:
        printf("Type: {");
        break;

    case R_CURLY_BRACKET:
        printf("Type: }");
        break;

    case PREFIX_TOKEN:
        printf("Type: ?");
        break;

    case SEMICOLON:
        printf("Type: ;");
        break;

    case EOF_TOKEN:
        printf("Type: EOF");
        break;

    case KEYWORD:
        printf("Type: Keyword. Keyword type: %s", (char *)(token->attribute));
        break;

    case UNDERSCORE_TOKEN:
        printf("Type: _");
        break;

    case AT_TOKEN:
        printf("Type: @");
        break;

    case VERTICAL_BAR_TOKEN:
        printf("Type: |");
        break;

    case COLON_TOKEN:
        printf("Type: :");
        break;

    case DOT_TOKEN:
        printf("Type: .");
        break;

    case COMMA_TOKEN:
        printf("Type: ,");
        break;

    case LITERAL_TOKEN:
        printf("Type: string literal, value: %s", (char *)(token->attribute));
        break;

    default:
        printf("PrintToken() default case");
    }
    printf("\n");
}

// checks all parameters
void Parameters(int *line_number)
{
    Token *token;

    while (1)
    {
        token = GetNextToken(line_number);
        PrintToken(token);
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
            if ((token = GetNextToken(line_number))->token_type != COLON_TOKEN)
            {
                DestroyToken(token);
                ErrorExit(ERROR_SYNTACTIC, "Expected ':' at line %d", *line_number);
            }

            if ((token = GetNextToken(line_number))->token_type != KEYWORD && (token->keyword_type != I32 || token->keyword_type != F64 || token->keyword_type != U8))
            {
                DestroyToken(token);
                ErrorExit(ERROR_SYNTACTIC, "Expected data type at line %d", *line_number);
            }

            // checks if there is another parameter
            if ((token = GetNextToken(line_number))->token_type != COMMA_TOKEN)
            {
                if (token->token_type == R_ROUND_BRACKET) // no more parameters
                {
                    break;
                }
                DestroyToken(token);
                ErrorExit(ERROR_SYNTACTIC, "Expected ',' or ')' at line %d", *line_number);
            }
        }
        else
        {
            DestroyToken(token);
            ErrorExit(ERROR_SYNTACTIC, "Expected identifier at line %d", *line_number);
        }

        DestroyToken(token);
    }
    DestroyToken(token);
}

// pub fn id ( seznam_parametrů ) návratový_typ {
// sekvence_příkazů
// }
void Function(int *line_number)
{
    Token *token = GetNextToken(line_number);
    if (token->keyword_type != FN) // expected 'fn' keyword
    {
        DestroyToken(token);
        ErrorExit(ERROR_SYNTACTIC, "Expected 'fn' keyword at line %d", *line_number);
    }

    DestroyToken(token);
    token = GetNextToken(line_number);
    if (token->token_type != IDENTIFIER_TOKEN) // expected function name
    {
        DestroyToken(token);
        ErrorExit(ERROR_SYNTACTIC, "Expected function name at line %d", *line_number);
    }

    DestroyToken(token);
    token = GetNextToken(line_number);
    if (token->token_type != L_ROUND_BRACKET) // expected '('
    {
        DestroyToken(token);
        ErrorExit(ERROR_SYNTACTIC, "Expected '(' at line %d", *line_number);
    }

    DestroyToken(token);
    // check paramters
    Parameters(line_number);
}

void ProgramBody(int *line_number)
{
    Token *token;
    while ((token = GetNextToken(line_number))->token_type != EOF_TOKEN)
    {
        switch (token->token_type)
        {
        case KEYWORD:
            if (token->keyword_type == PUB)
            {
                Function(line_number);
            }
            break;
            token = GetNextToken(line_number);

        default:
            // printf("Unexpected token at line %d\n", *line_number);
            // ErrorExit(ERROR_SYNTACTIC, "Unexpected token at line %d", *line_number);
            break;
        }
        DestroyToken(token);
    }
    DestroyToken(token);
}

int main()
{
    int line_number = 1;
    ProgramBody(&line_number);
    return 0;
}
