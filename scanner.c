#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include "scanner.h"
#include "error.h"
#include "vector.h"


Scanner *InitScanner(){
    Scanner *scanner;
    if ((scanner = malloc(sizeof(scanner))) == NULL){
        ErrorExit(ERROR_INTERNAL, "Internal compiler error: Memory allocation failed.");
    }

    //default values && return
    scanner -> state = READY;
    scanner -> line_number = 1;

    return scanner;
}

Token *InitToken(){
    Token *token;
    if ((token = calloc(1, sizeof(token))) == NULL){
        ErrorExit(ERROR_INTERNAL, "Internal compiler error: Memory allocation failed.");
    }

    //set default value and return
    token -> keyword_type = NONE;
    return token;
}

void DestroyToken(Token *token){
    if(token -> attribute != NULL)
        free(token -> attribute);

    free(token);
}

char NextChar(){
    int c = getchar();
    ungetc(c, stdin);
    return c;
}

CHAR_TYPE GetSymbolType(char c, Scanner *scanner){
    if(isdigit(c)) return NUMBER;
    if(isalpha(c)) return CHARACTER;
    return OTHER;
}

//TODO
TOKEN_TYPE ConsumeNumber(Token *token){
    return INTEGER_32; 
}

void ConsumeIdentifier(Token *token){
    return;
}

void ConsumeLiteral(Token *token, Scanner *scanner){
    return;
}

char ConsumeComment(Scanner *scanner){
    int c;
    while((c = getchar()) != '\n' && c != EOF){
        continue;
    }

    if(c == '\n') scanner -> line_number ++;
    return c;
}

char ConsumeWhitespace(Scanner *scanner){
    int c;
    while(isspace(c = getchar()) && c != EOF){
        continue;
    }

    if(c == '\n') scanner -> line_number ++;
    return c;
}



Token *GetNextToken(Scanner *scanner){
    //initial variables
    int c; char next; Token *token = InitToken();

    //skip all the whitespace character and
    //return the first non-whitespace character or end the function at the end of the file
    if((c = ConsumeWhitespace(scanner)) == EOF){
        token -> token_type = EOF_TOKEN;
        return token;
    }

    ungetc(c, stdin);

    while(true){
        switch(c){
            /*operator tokens*/
            case '=': //valid tokens are = and also ==
                if ((c = getchar()) == '=')
                    token -> token_type = EQUAL_OPERATOR;

                else{
                    ungetc(c, stdin);
                    token -> token_type = ASSIGNMENT;
                }

                return token;

            case '+':
                token -> token_type = ADDITION_OPERATOR;
                return token;

            case '-': //can signal the start of a negative number
                if(isdigit(next = NextChar())){
                    token -> token_type = ConsumeNumber(token);
                }
                token -> token_type = SUBSTRACTION_OPERATOR;
                return token;

            case '*':
                token -> token_type = MULTIPLICATION_OPERATOR;
                return token;

            case '/': //can also signal the start of a comment
                if((next = NextChar()) != '/'){
                    token -> token_type = DIVISION_OPERATOR;
                    return token;
                }

                //indicates the start of a comment --> consume the second '/' character and skip to the end of the line/file
                getchar(); c = ConsumeComment(scanner);

                //run the switch again with the first character after the comment ends
                continue;

            case '!': //! by itself isn't a valid token, however != is
                if((next = NextChar()) != '='){
                    DestroyToken(token);
                    ErrorExit(ERROR_LEXICAL, "Error in lexical analysis: Line %d: Invalid token !%c", scanner -> line_number, next);
                }

                token -> token_type = NOT_EQUAL_OPERATOR;
                return token;

            case '<': //< is a valid token, but so is <=
                if((next = NextChar()) != '='){
                    token -> token_type = LESS_THAN_OPERATOR;
                }

                token -> token_type = LESSER_EQUAL_OPERATOR;
                return token;

            case '>': //analogous to <
                if((next = NextChar()) != '='){
                    token -> token_type = LARGER_THAN_OPERATOR;
                }

                token -> token_type = LARGER_EQUAL_OPERATOR;
                return token;

            /*bracket tokens and array symbol*/
            case '(':
                token -> token_type = L_ROUND_BRACKET;
                return token;

            case ')':
                token -> token_type = R_ROUND_BRACKET;
                return token;

            case '{':
                token -> token_type = L_CURLY_BRACKET;
                return token;

            case '}':
                token -> token_type = R_CURLY_BRACKET;
                return token;

            case '[':
                if((next = NextChar()) != ']'){
                    DestroyToken(token);
                    ErrorExit(ERROR_LEXICAL, "Error in lexical analysis: Line %d: Invalid token [%c", scanner -> line_number, next);
                }

                token -> token_type = ARRAY_TOKEN;
                return token;

            /*special symbols*/
            case '?':
                token -> token_type = PREFIX_TOKEN;
                return token;

            case EOF:
                token -> token_type = EOF_TOKEN;
                return token;

            case ';':
                token -> token_type = SEMICOLON;
                return token;

            /*call GetSymbolType to determine next token*/
            default:
                switch(GetSymbolType(c, scanner)){
                    /*beginning of a identifier*/
                    case CHARACTER:
                        break;

                    case NUMBER:
                        token -> token_type = ConsumeNumber(token);
                        return token;

                    default:
                        ErrorExit(ERROR_LEXICAL, "Error in lexical analysis: Line %d: Invalid token %c", scanner -> line_number, c);
                }



        }
    }
}

int main(){
    printf("dfferwf");
}