/**
 * @file scanner.c
 * @brief Lexical analyzer (scanner) for the compiler
 *
 * This file implements the lexical analysis phase of the compiler,
 * responsible for breaking down the input source code into tokens.
 *
 * @authors Igor Lacko [xlackoi00]
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#include "scanner.h"
#include "error.h"
#include "vector.h"

Token *InitToken()
{
    Token *token;
    if ((token = calloc(1, sizeof(Token))) == NULL)
    {
        ErrorExit(ERROR_INTERNAL, "Memory allocation failed.");
    }

    // set default value and return
    token->keyword_type = NONE;
    return token;
}

Token *CopyToken(Token *token)
{
    Token *copy = InitToken();

    copy->attribute = strdup(token->attribute);
    copy->token_type = token->token_type;
    copy->keyword_type = token->keyword_type;
    copy->line_number = token->line_number;

    return copy;
}

void DestroyToken(Token *token)
{
    if (token->attribute != NULL)
    {
        free(token->attribute);
        token->attribute = NULL;
    }

    free(token);
    token = NULL;
}

char NextChar()
{
    int c = getchar();
    ungetc(c, stdin);
    return c;
}

CHAR_TYPE GetCharType(char c)
{
    if (isdigit(c))
        return NUMBER;
    if (isalpha(c) || c == '_')
        return CHARACTER;
    if (isspace(c))
        return WHITESPACE;
    return OTHER;
}

void ConsumeNumber(Token *token, int *line_number)
{
    // tracking variable
    int c;

    // vector to put the string value of the token and int/doubles to put the actual value
    // if the numbers ends up being a float, we save it as an string (since it can have an exponent, which C doesn't support so we can't save by value)
    Vector *vector = InitVector();

    // boolean values to check which parts the number has (if it's a floating point number)
    // valid float construction: 3.14, or 3e-2 or 3e+2 or 3e2 or 3.14e-2 or 3.14e+2 or 3.14e2
    // so 3e-2.14 would be token 3e-2, . would be another token and 14 would also be a separate token
    bool has_exponent = false, has_floating_point = false;

    token->token_type = INTEGER_32;

    // TODO: zeroes at the start of the part of a number that is whole are invalid
    while (isdigit((c = getchar())) || c == '.' || tolower(c) == 'e')
    { // number can be int/double (double has a '.')
        if (c == '.')
        { // check if we already have a floating point value (in case of doubles)

            // if not, switch the type to float. if yes, invalid token --> error --> exit the program with lexical error code
            if (!has_floating_point)
            {
                token->token_type = DOUBLE_64;
                AppendChar(vector, c);
                has_floating_point = true;
            }

            else
            {
                /*Don't use ErrorExit(here since we need to free memory AFTER printing the message)*/
                AppendChar(vector, '\0'); // terminate the string for printing
                fprintf(stderr, "Line %d: Invalid token %s.\n", *line_number, vector->value);

                // free all allocated resources
                DestroyToken(token);
                DestroyVector(vector);

                exit(ERROR_LEXICAL);
            }
        }

        // do the same for the exponent part
        // we need to break the loop if the exponent ends, since it's always at the end
        else if (tolower(c) == 'e')
        {
            if (!has_exponent)
            {
                ungetc(c, stdin);
                if (!ConsumeExponent(vector, token, has_floating_point))
                {
                    break;
                }

                else
                    token->token_type = DOUBLE_64;
            }
        }

        // c is a number
        else
            AppendChar(vector, c);
    }

    // terminate the vector
    AppendChar(vector, '\0');

    // c is the first character after the number, so put it back to the stream
    ungetc(c, stdin);

    // Sprintf to a string
    if (token->token_type == DOUBLE_64)
    {
        double float_res = strtod(vector->value, NULL);
        unsigned long length = snprintf(NULL, 0, "%lf", float_res);
        if ((token->attribute = malloc((length + 1) * sizeof(char))) == NULL)
            ErrorExit(ERROR_INTERNAL, "Memory allocation failed");

        sprintf(token->attribute, "%lf", float_res);
        DestroyVector(vector);
        return;
    }

    // copy the number to the token's value
    if ((token->attribute = malloc(vector->length * sizeof(char))) == NULL)
        ErrorExit(ERROR_INTERNAL, "Memory allocation failed");

    // the float is saved as a char* since it can contain an exponent
    strcpy((token->attribute), vector->value);

    // Check the leading zeroes, // TODO: check
    if (token->token_type == INTEGER_32 && strlen(token->attribute) > 1 && token->attribute[0] == '0' && token->attribute[1] == '0')
    {
        fprintf(stderr, "Line %d: Invalid token %s\n", *line_number, token->attribute);
        DestroyToken(token);
        exit(ERROR_LEXICAL);
    }

    DestroyVector(vector);
}

bool ConsumeExponent(Vector *vector, Token *token, bool has_floating_point)
{
    // at the start getchar() will return 'e'/'E' since we used ungetc()
    int c = getchar();

    /*reminder: valid float construction:
    3.14, or 3e-2 or 3e+2 or 3e2 or 3.14e-2 or 3.14e+2 or 3.14e2
    */

    // if the next character isn't a sign or a digit the number stays as it is
    int next;
    if ((next = NextChar()) == '+' || next == '-' || isdigit(next))
    {
        AppendChar(vector, c);
        AppendChar(vector, next);
        getchar(); // move the stream forward
    }

    else
    { // return 'e'/'E' to the stream, the token stays a number
        if (!has_floating_point)
            token->token_type = INTEGER_32;
        return false;
    }

    // here we found a digit/sign and appended it, so the only thing that remains is to append remaining digits
    while (isdigit(c = getchar()))
        AppendChar(vector, c);

    // unget the last character after the numbers so that the scanner can process it
    ungetc(c, stdin);
    return true;
}

void ConsumeIdentifier(Token *token, int *line_number)
{
    // needed variables
    int c;
    Vector *vector = InitVector();

    // lone '_' identifier case
    if ((c = getchar()) == '_' && !isalnum(NextChar()) && NextChar() != '_')
    {
        token->token_type = UNDERSCORE_TOKEN;
        token->attribute = strdup("_");
        token->line_number = *line_number;
        DestroyVector(vector);
        return;
    }

    // append the characters until we reach the end of the identifier
    AppendChar(vector, c);
    while (isalnum(c = getchar()) || c == '_')
    {
        AppendChar(vector, c);
    }

    if (c == '\n')
        ++(*line_number);
    else
        ungetc(c, stdin);

    // terminate the vector
    AppendChar(vector, '\0');

    // copy the vector's value to the token's attribute (the identifier name)
    if ((token->attribute = malloc(vector->length * sizeof(char))) == NULL)
    {
        DestroyVector(vector);
        DestroyToken(token);
        ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
    }

    strcpy(token->attribute, vector->value);
    DestroyVector(vector);

    // check if the token isn't an invalid one with a prefix
    if (token->attribute[0] == '?')
    {
        if (!IsValidPrefix(token->attribute))
        {
            fprintf(stderr, RED "Error in lexical analysis: Line %d: Invalid token %s\n" RESET, *line_number, token->attribute);
            DestroyToken(token);
            exit(ERROR_LEXICAL);
        }

        token->token_type = KEYWORD;
        token->keyword_type = IsKeyword(token->attribute);

        return;
    }

    // check if the identifier isn't actually a keyword
    KEYWORD_TYPE keyword_type;
    if ((keyword_type = IsKeyword(token->attribute)) == NONE)
    {
        token->token_type = IDENTIFIER_TOKEN;
    }

    else
    {
        token->token_type = KEYWORD;
        token->keyword_type = keyword_type;
    }

    token->line_number = *line_number;
}

void ConsumeLiteral(Token *token, int *line_number)
{
    int c;
    Vector *vector = InitVector();

    // loop until we encounter another " character
    while ((c = getchar()) != '"' && c != '\n' && c != EOF)
    {
        if (c != '\\')
            AppendChar(vector, c);
        else
        { // possible escape sequence
            switch (c = getchar())
            {
            // all possible \x characters
            case '"':
                AppendChar(vector, '\"');
                break;

            case 'n':
                AppendChar(vector, '\n');
                break;

            case 'r':
                AppendChar(vector, '\r');
                break;

            case 't':
                AppendChar(vector, '\t');
                break;

            case 'x':
                ConsumeHexadecimalEscapeSequence(vector, line_number);
                break;

            case '\\':
                AppendChar(vector, c);
                break;

            // invalid escape sequence, throw a lexical error
            default:
                DestroyToken(token);
                DestroyVector(vector);
                ErrorExit(ERROR_LEXICAL, "Line %d: Invalid escape sequence '/%c' in a literal", *line_number, c);
            }
        }
    }

    // terminate the string
    AppendChar(vector, '\0');

    // either a valid end of a string, or throw an error in case of newline/end of file
    switch (c)
    {
    case '"': // valid string ending, copy the string to the token's attribute
        if ((token->attribute = malloc(vector->length * sizeof(char))) == NULL)
        {
            DestroyToken(token);
            DestroyVector(vector);
            ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
        }

        strcpy(token->attribute, vector->value);
        DestroyVector(vector);
        break;

    case '\n':
    case EOF:
        DestroyToken(token);
        DestroyVector(vector);
        ErrorExit(ERROR_LEXICAL, "Line %d: String missing a second \"", *line_number);
    }
}

void ConsumeMultiLineLiteral(Token *token, int *line_number)
{
    int c;
    Vector *vector = InitVector();

    // At the start, we are after the initial '\\' duo
    while (true)
    {
        c = getchar();

        // Non-escape sequence or newline characters
        if (c != '\n' && c != EOF)
        {
            AppendChar(vector, c);
            continue;
        }

        else if (c == EOF)
        {
            DestroyToken(token);
            DestroyVector(vector);
            ErrorExit(ERROR_LEXICAL, "Line %d: Unexpected end of file", *line_number);
        }

        else
        {
            ++(*line_number);
            if (DoesMultiLineLiteralContinue(line_number))
            {
                AppendChar(vector, '\n');
                continue;
            }
            else
                break;
        }
    }

    // Terminate the string
    AppendChar(vector, '\0');

    // Copy the string to the token's attribute
    token->attribute = strdup(vector->value);
    DestroyVector(vector);
}

bool DoesMultiLineLiteralContinue(int *line_number)
{
    int c;
    while ((c = getchar()) != EOF)
    {
        if (c != '\\' && c != '\n')
        {
            if (!isspace(c))
            {
                ungetc(c, stdin);
                return false;
            }
        }

        else if (c == '\n')
        {
            ++(*line_number);
            return false;
        }

        else
        {
            if ((c = NextChar()) == '\\')
            {
                getchar();
                return true;
            }

            else
            {
                ungetc(c, stdin);
                return false;
            }
        }
    }

    ungetc(c, stdin);
    return false;
}

void ConsumeHexadecimalEscapeSequence(Vector *vector, int *line_number)
{
    int c;
    char digit_1, digit_2;

    // Do this twice :))
    if (!isdigit(c = getchar()))
    {
        DestroyVector(vector);
        ErrorExit(ERROR_LEXICAL, "Line %d: Invalid hexadecimal escape sequence '\\x%c'", *line_number, c);
    }

    else
        digit_1 = c;

    if (!isdigit(c = getchar()))
    {
        DestroyVector(vector);
        ErrorExit(ERROR_LEXICAL, "Line %d: Invalid hexadecimal escape sequence '\\x%c'", *line_number, c);
    }

    else
        digit_2 = c;

    // Convert the two digits to a hexadecimal number
    char hex[3] = {digit_1, digit_2, '\0'};
    int value = strtol(hex, NULL, 16);

    // Append the character to the vector
    AppendChar(vector, value);
}

int ConsumeComment(int *line_number)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
    {
        continue;
    }

    if (c == '\n')
        ++(*line_number);
    return c;
}

void ConsumeImportToken(Token *token, int *line_number)
{
    const char import[8] = "@import";
    int c;
    // compare until we reach the end, or throw an error (@ is an invalid token by itself)

    for (int i = 0; i < 7; i++)
    {
        if ((c = getchar()) != import[i])
        {
            DestroyToken(token);
            ErrorExit(ERROR_LEXICAL, "Line %d: Invalid token '@'", *line_number);
        }
    }

    // token is valid
    token->token_type = IMPORT_TOKEN;
    token->attribute = strdup("@import");
}

int ConsumeWhitespace(int *line_number)
{
    int c;
    while (isspace(c = getchar()) && c != EOF)
    {
        if (c == '\n')
            ++(*line_number);
        continue;
    }

    return c;
}

void ConsumeU8Token(Token *token, int *line_number)
{
    // expected token structure to compare to and a storage array to store the actual token
    char u8_token[] = "[]u8";
    char nullable_u8_token[] = "?[]u8";
    char actual_token[6]; // max size is 6 because ? + u8[] + '\0'

    // iterator and a variable to store all the incoming characters
    int i, c;

    int length = NextChar() == '?' ? 5 : 4;
    for (i = 0; i < length; i++)
    {
        c = getchar();

        if ((c != u8_token[i] && i != 5) && (c != nullable_u8_token[i]))
        {
            DestroyToken(token);
            actual_token[i] = '\0';
            ErrorExit(ERROR_LEXICAL, "Line %d: Invalid token %s%c", *line_number, actual_token, c);
        }

        else
            actual_token[i] = c;
    }

    actual_token[i] = '\0';

    // copy the u8[] string to the token
    if ((token->attribute = malloc((length + 1) * sizeof(char))) == NULL)
    {
        DestroyToken(token);
        ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
    }

    strcpy(token->attribute, actual_token);

    token->line_number = *line_number;
    token->token_type = KEYWORD;
    token->keyword_type = U8;
}

KEYWORD_TYPE IsKeyword(char *attribute)
{
    // create a array of keywords(sice it's constant)
    const char keyword_strings[][KEYWORD_COUNT] = {
        "const",
        "else",
        "fn",
        "if",
        "i32",
        "f64",
        "null",
        "pub",
        "return",
        "u8",
        "var",
        "void",
        "while"};

    if (attribute[0] == '?')
    {
        return IsKeyword(attribute + 1);
    }

    for (int i = 0; i < KEYWORD_COUNT; i++)
    {
        if (!strcmp(attribute, keyword_strings[i]))
        {
            return i;
        }
    }

    return NONE;
}

bool IsValidPrefix(char *identifier)
{
    return IsKeyword(identifier + 1) == NONE ? false : true;
}

Token *LoadTokenFromStream(int *line_number)
{
    // initial variables
    int c;
    char next;
    Token *token = InitToken();

    // skip all the whitespace character and
    // return the first non-whitespace character or end the function at the end of the file
    if ((c = ConsumeWhitespace(line_number)) == EOF)
    {
        token->token_type = EOF_TOKEN;
        return token;
    }

    while (true)
    {
        switch (c = (isspace(c)) ? getchar() : c)
        {
        /*operator tokens*/
        case '=': // valid tokens are = and also ==
            if ((c = NextChar()) == '=')
            {
                getchar();
                token->attribute = strdup("==");
                token->token_type = EQUAL_OPERATOR;
            }

            else
            {
                token->attribute = strdup("=");
                token->token_type = ASSIGNMENT;
            }

            token->line_number = *line_number;
            return token;

        case '+':
            token->attribute = strdup("+");
            token->token_type = ADDITION_OPERATOR;
            token->line_number = *line_number;
            return token;

        case '-':
            token->attribute = strdup("-");
            token->token_type = SUBSTRACTION_OPERATOR;
            token->line_number = *line_number;
            return token;

        case '*':
            token->attribute = strdup("*");
            token->token_type = MULTIPLICATION_OPERATOR;
            token->line_number = *line_number;
            return token;

        case '/': // can also signal the start of a comment
            if ((next = NextChar()) != '/')
            {
                token->attribute = strdup("/");
                token->token_type = DIVISION_OPERATOR;
                token->line_number = *line_number;
                return token;
            }

            // indicates the start of a comment --> consume the second '/' character and skip to the end of the line/file
            getchar();
            c = ConsumeComment(line_number);

            // run the switch again with the first character after the comment ends
            continue;

        case '!': //! by itself isn't a valid token, however != is
            if ((next = NextChar()) != '=')
            {
                DestroyToken(token);
                ErrorExit(ERROR_LEXICAL, "Line %d: Invalid token !%c", *line_number, next);
            }

            else
            {
                token->attribute = strdup("!=");
                getchar();
                token->line_number = *line_number;
                token->token_type = NOT_EQUAL_OPERATOR;
            }

            return token;

        case '<': //< is a valid token, but so is <=
            if ((next = NextChar()) != '=')
            {
                token->attribute = strdup("<");
                token->token_type = LESS_THAN_OPERATOR;
            }

            else
            {
                token->attribute = strdup("<=");
                getchar(); // consume the = character
                token->token_type = LESSER_EQUAL_OPERATOR;
            }

            token->line_number = *line_number;
            return token;

        case '>': // analogous to <
            if ((next = NextChar()) != '=')
            {
                token->attribute = strdup(">");
                token->token_type = LARGER_THAN_OPERATOR;
            }

            else
            {
                token->attribute = strdup(">=");
                getchar();
                token->token_type = LARGER_EQUAL_OPERATOR;
            }

            token->line_number = *line_number;
            return token;

        /*bracket tokens and array symbol*/
        case '(':
            token->attribute = strdup("(");
            token->token_type = L_ROUND_BRACKET;
            token->line_number = *line_number;
            return token;

        case ')':
            token->attribute = strdup(")");
            token->token_type = R_ROUND_BRACKET;
            token->line_number = *line_number;
            return token;

        case '{':
            token->attribute = strdup("{");
            token->token_type = L_CURLY_BRACKET;
            token->line_number = *line_number;
            return token;

        case '}':
            token->attribute = strdup("}");
            token->token_type = R_CURLY_BRACKET;
            token->line_number = *line_number;
            return token;

        case '[': // A bit of a special case, []u8 is a keyword that can't be mistaken for a identifier but u8 can but u8 by itself is a invalid token
            ungetc(c, stdin);
            ConsumeU8Token(token, line_number);
            return token;

        case '|':
            token->attribute = strdup("|");
            token->token_type = VERTICAL_BAR_TOKEN;
            token->line_number = *line_number;
            return token;

        /*special symbols*/
        case '?':
            next = NextChar();
            ungetc(c, stdin);

            if (isalnum(next))
                ConsumeIdentifier(token, line_number);
            else if (next == '[')
                ConsumeU8Token(token, line_number);
            else
                ErrorExit(ERROR_LEXICAL, "Line %d: Invalid token '?'", *line_number);

            return token;

        case EOF:
            token->token_type = EOF_TOKEN;
            token->line_number = *line_number;
            return token;

        case ';':
            token->attribute = strdup(";");
            token->token_type = SEMICOLON;
            token->line_number = *line_number;
            return token;

        case '_':
            ungetc(c, stdin);
            ConsumeIdentifier(token, line_number);
            token->line_number = *line_number;
            return token;

        case '@':
            ungetc(c, stdin);
            ConsumeImportToken(token, line_number);
            token->line_number = *line_number;
            return token;

        case ':':
            token->attribute = strdup(":");
            token->token_type = COLON_TOKEN;
            token->line_number = *line_number;
            return token;

        case '.':
            token->attribute = strdup(".");
            token->token_type = DOT_TOKEN;
            token->line_number = *line_number;
            return token;

        case ',':
            token->attribute = strdup(",");
            token->token_type = COMMA_TOKEN;
            token->line_number = *line_number;
            return token;

        /*Beginning of a string*/
        case '"':
            token->token_type = LITERAL_TOKEN;
            ConsumeLiteral(token, line_number);
            token->line_number = *line_number;
            return token;

        /*multiple 0's are an invalid token*/
        case '0':
            if (isdigit(next = NextChar()))
                ErrorExit(ERROR_LEXICAL, "Line %d: Invalid token '0%c'", *line_number, next);

            ungetc(c, stdin);
            ConsumeNumber(token, line_number);

            token->line_number = *line_number;
            return token;

        // Signals the start of a multiline string
        case '\\':
            if ((next = NextChar()) == '\\')
            {
                getchar();
                token->token_type = LITERAL_TOKEN;
                token->line_number = *line_number;
                ConsumeMultiLineLiteral(token, line_number);

                return token;
            }

            else
            {
                DestroyToken(token);
                ErrorExit(ERROR_LEXICAL, "Line %d: Invalid token '\\%c'", *line_number, next);
            }

            break;

        /*call GetSymbolType to determine next token*/
        default:
            // return the character back, since the consume functions parse the whole token
            ungetc(c, stdin);

            // call a sub-FSM function depending on the char type
            switch (GetCharType(c))
            {
            /*beginning of a identifier*/
            case CHARACTER:
                ConsumeIdentifier(token, line_number);
                token->line_number = *line_number;
                return token;

            case NUMBER:
                ConsumeNumber(token, line_number);
                token->line_number = *line_number;
                return token;

            case WHITESPACE:
                c = ConsumeWhitespace(line_number);
                continue;

            default:
                DestroyToken(token);
                ErrorExit(ERROR_LEXICAL, "Line %d: Invalid token %c", *line_number, c);
            }
        }
    }
}

void PrintToken(Token *token)
{
    switch (token->token_type)
    {
    case IDENTIFIER_TOKEN:
        fprintf(stderr, "Type: Identifier token, attribute: %s", token->attribute);
        break;

    case INTEGER_32:
        fprintf(stderr, "Type: Int32 token, attribute: %s", token->attribute);
        break;

    case DOUBLE_64:
        fprintf(stderr, "Type: F64 token, attribute: %s", token->attribute);
        break;

    case ASSIGNMENT:
        fprintf(stderr, "Type: =");
        break;

    case MULTIPLICATION_OPERATOR:
        fprintf(stderr, "Type: *");
        break;

    case DIVISION_OPERATOR:
        fprintf(stderr, "Type: /");
        break;

    case ADDITION_OPERATOR:
        fprintf(stderr, "Type: +");
        break;

    case SUBSTRACTION_OPERATOR:
        fprintf(stderr, "Type: -");
        break;

    case EQUAL_OPERATOR:
        fprintf(stderr, "Type: ==");
        break;

    case NOT_EQUAL_OPERATOR:
        fprintf(stderr, "Type: !=");
        break;

    case LESS_THAN_OPERATOR:
        fprintf(stderr, "Type: <");
        break;

    case LARGER_THAN_OPERATOR:
        fprintf(stderr, "Type: >");
        break;

    case LESSER_EQUAL_OPERATOR:
        fprintf(stderr, "Type: <=");
        break;

    case LARGER_EQUAL_OPERATOR:
        fprintf(stderr, "Type: >=");
        break;

    case L_ROUND_BRACKET:
        fprintf(stderr, "Type: (");
        break;

    case R_ROUND_BRACKET:
        fprintf(stderr, "Type: )");
        break;

    case L_CURLY_BRACKET:
        fprintf(stderr, "Type: {");
        break;

    case R_CURLY_BRACKET:
        fprintf(stderr, "Type: }");
        break;

    case SEMICOLON:
        fprintf(stderr, "Type: ;");
        break;

    case EOF_TOKEN:
        fprintf(stderr, "Type: EOF");
        break;

    case KEYWORD:
        fprintf(stderr, "Type: Keyword. Keyword type: %s", token->attribute);
        break;

    case UNDERSCORE_TOKEN:
        fprintf(stderr, "Type: _");
        break;

    case IMPORT_TOKEN:
        fprintf(stderr, "Type: @import");
        break;

    case VERTICAL_BAR_TOKEN:
        fprintf(stderr, "Type: |");
        break;

    case COLON_TOKEN:
        fprintf(stderr, "Type: :");
        break;

    case DOT_TOKEN:
        fprintf(stderr, "Type: .");
        break;

    case COMMA_TOKEN:
        fprintf(stderr, "Type: ,");
        break;

    case LITERAL_TOKEN:
        fprintf(stderr, "Type: string literal, value: %s", token->attribute);
        break;

    default:
        fprintf(stderr, "PrintToken() default case");
    }
    fprintf(stderr, " Line number: %d\n", token->line_number);
}
