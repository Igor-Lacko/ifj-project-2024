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

void DestroyScanner(Scanner *scanner){
    free(scanner);
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

CHAR_TYPE GetCharType(char c){
    if(isdigit(c)) return NUMBER;
    if(isalpha(c) || c == '_') return CHARACTER;
    return OTHER;
}

TOKEN_TYPE ConsumeNumber(Token *token, Scanner *scanner){
    //tracking variables
    int c; TOKEN_TYPE type = INTEGER_32; 

    //vector to put the string value of the token and int/doubles to put the actual value
    Vector *vector = InitVector(); int i32; double f64;

    while(isdigit((c = getchar())) || c == '.'){ //number can be int/double (double has a '.')
        if(c == '.'){ //check if we already have a exponent (in case of doubles)

            //if not, switch the type to float. if yes, invalid token --> error --> exit the program with lexical error code
            if(type != DOUBLE_64){
                type = DOUBLE_64;
                AppendChar(vector, c);
            }

            else{
                //copy the invalid token to a static array for error message
                char invalid_token[vector -> length];
                strcpy(invalid_token, vector -> value);

                //free all allocated resources
                DestroyToken(token);
                DestroyVector(vector);

                ErrorExit(ERROR_LEXICAL, "Error in lexical analysis: Line %d: Invalid token %s", scanner -> line_number, invalid_token);
            }
        }

        //c is a number
        AppendChar(vector, c);
    }

    //terminate the vector
    AppendChar(vector, '\0');

    //c is the first character after the number, so put it back to the stream
    ungetc(c, stdin);

    //copy the number to the token's value
    if(type == INTEGER_32){
        i32 = atoi(vector -> value);
        if((token -> attribute = malloc(sizeof(int))) == NULL)
            ErrorExit(ERROR_INTERNAL, "Compiler internal error: Memory allocation failed");

        *(int*)(token -> attribute) = i32;
    }

    else{
        f64 = atof(vector -> value);
        if((token -> attribute = malloc(sizeof(double))) == NULL)
            ErrorExit(ERROR_INTERNAL, "Compiler internal error: Memory allocation failed");

        *(double*)(token -> attribute) = f64;
    }

    DestroyVector(vector);


    return type;
}


void ConsumeIdentifier(Token *token, Scanner *scanner){
    //needed variables
    int c; Vector *vector = InitVector();

    //lone '_' identifier case
    if((c = getchar()) == '_' && !isalnum(NextChar())){
        //free allocated resources
        DestroyVector(vector);
        DestroyToken(token);

        ErrorExit(ERROR_LEXICAL, "Error in lexical analysis: Line %d: Invalid token _", scanner -> line_number);
    }

    //append the characters until we reach the end of the identifier
    AppendChar(vector, c);
    while(isalnum(c = getchar()) || c == '_'){
        AppendChar(vector, c);
    }

    //terminate the vector
    AppendChar(vector, '\0');

    //copy the vector's value to the token's attribute (the identifier name)
    if((token -> attribute = malloc(vector -> length * sizeof(char))) == NULL)
            ErrorExit(ERROR_INTERNAL, "Compiler internal error: Memory allocation failed");

    strcpy(vector -> value, (char *)(token -> attribute));

    //TODO: implement list of keywords to check if the type isn't keyword
    token -> token_type = IDENTIFIER_TOKEN;
}


//TODO
/*void ConsumeLiteral(Token *token, Scanner *scanner){
    return;
}*/

int ConsumeComment(Scanner *scanner){
    int c;
    while((c = getchar()) != '\n' && c != EOF){
        continue;
    }

    if(c == '\n') scanner -> line_number ++;
    return c;
}

int ConsumeWhitespace(Scanner *scanner){
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
                //TODO: work this out

                /*if(isdigit(next = NextChar())){
                    token -> token_type = ConsumeNumber(token, scanner);
                }*/

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
                //return the character back, since the consume functions parse the whole token
                ungetc(c, stdin);

                //call a sub-FSM function depending on the char type
                switch(GetCharType(c)){
                    /*beginning of a identifier*/
                    case CHARACTER:
                        ConsumeIdentifier(token, scanner);
                        return token;

                    case NUMBER:
                        token -> token_type = ConsumeNumber(token, scanner);
                        return token;

                    default:
                        ErrorExit(ERROR_LEXICAL, "Error in lexical analysis: Line %d: Invalid token %c", scanner -> line_number, c);
                }



        }
    }
}

#ifdef IFJ24_DEBUG

void PrintToken(Token *token){
    switch (token -> token_type){
        case IDENTIFIER_TOKEN:
            printf("Type: Identifier token, attribute: %s", (char *)(token -> attribute));
            break;
        
        case INTEGER_32:
            printf("Type: Int32 token, attribute: %d", *(int *)(token -> attribute));
            break;

        case DOUBLE_64:
            printf("Type: F64 token, attribute: %lf", *(double *)(token -> attribute));
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

        default:
            printf("FAYHUSFGUYFSGAUHFAG");
        
        
    }
}

int main(){
    Token *token; Scanner *scanner = InitScanner(); int cnt = 0;
    while((token = GetNextToken(scanner)) -> token_type != EOF_TOKEN){
        cnt ++; if(cnt >= 100) break;
        printf("Token on line %d. Printing token...\n", scanner -> line_number);
        PrintToken(token);
        printf("\n\n\n\n");
    }

    DestroyToken(token); DestroyScanner(scanner);
}

#endif