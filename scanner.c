#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include "scanner.h"
#include "error.h"
#include "vector.h"


Token *InitToken(){
    Token *token;
    if ((token = calloc(1, sizeof(Token))) == NULL){
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
    if(isspace(c)) return WHITESPACE;
    return OTHER;
}

TOKEN_TYPE ConsumeNumber(Token *token, int *line_number){
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
                /*Don't use ErrorExit(here since we need to free memory AFTER printing the message)*/
                AppendChar(vector, '\0'); //terminate the string for printing
                fprintf(stderr, "Error in lexical analysis: Line %d: Invalid token %s.\n", *line_number, vector -> value);
                
                //free all allocated resources
                DestroyToken(token);
                DestroyVector(vector);

                exit(ERROR_LEXICAL);
            }
        }

        //c is a number
        else AppendChar(vector, c);
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


void ConsumeIdentifier(Token *token, int *line_number){
    //needed variables
    int c; Vector *vector = InitVector();

    //lone '_' identifier case
    if((c = getchar()) == '_' && !isalnum(NextChar())){
        token -> token_type = UNDERSCORE_TOKEN;
        DestroyVector(vector);
    }

    //append the characters until we reach the end of the identifier
    AppendChar(vector, c);
    while(isalnum(c = getchar()) || c == '_'){
        AppendChar(vector, c);
    }

    if(c == '\n') ++(*line_number);


    //terminate the vector
    AppendChar(vector, '\0');

    //copy the vector's value to the token's attribute (the identifier name)
    if((token -> attribute = malloc(vector -> length * sizeof(char))) == NULL){
        DestroyVector(vector);
        DestroyToken(token);
        ErrorExit(ERROR_INTERNAL, "Compiler internal error: Memory allocation failed");
    }


    strcpy((char *)(token -> attribute), vector -> value);
    DestroyVector(vector);

    //check if the identifier isn't actually a keyword
    KEYWORD_TYPE keyword_type;
    if((keyword_type = IsKeyword(token)) == NONE){
        token -> token_type = IDENTIFIER_TOKEN;
    }

    else{
        token -> token_type = KEYWORD;
        token -> keyword_type = keyword_type;
    }

}


void ConsumeLiteral(Token *token, int *line_number){
    int c; Vector *vector = InitVector();

    //loop until we encounter another " character
    while((c = getchar()) != '"' && c != '\n' && c != EOF){

        AppendChar(vector, c);
        if(c == '\\'){ //possible escape sequence
            switch(c = getchar()){
                //all possible \x characters
                case '"': case 'n': case: 'r' case: 't': case: '\\':
                    AppendChar(c);
                    break;

                default:
                    
            }
        }
    }
}

int ConsumeComment(int *line_number){
    int c;
    while((c = getchar()) != '\n' && c != EOF){
        continue;
    }

    if(c == '\n') ++(*line_number);
    return c;
}

int ConsumeWhitespace(int *line_number){
    int c;
    while(isspace(c = getchar()) && c != EOF){
        continue;
    }

    if(c == '\n') ++(*line_number);
    return c;
}

KEYWORD_TYPE IsKeyword(Token *token){
    //create a array of keywords(sice it's constant)
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
        "while"
    };

    for(int i = 0; i < KEYWORD_COUNT; i++){
        if(!strcmp(token -> attribute, keyword_strings[i])){
            return i;
        }
    }

    return NONE;
}



Token *GetNextToken(int *line_number){
    //initial variables
    int c; char next; Token *token = InitToken();

    //skip all the whitespace character and
    //return the first non-whitespace character or end the function at the end of the file
    if((c = ConsumeWhitespace(line_number)) == EOF){
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

            case '-': 
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
                getchar(); c = ConsumeComment(line_number);

                //run the switch again with the first character after the comment ends
                continue;

            case '!': //! by itself isn't a valid token, however != is
                if((next = NextChar()) != '='){
                    DestroyToken(token);
                    ErrorExit(ERROR_LEXICAL, "Error in lexical analysis: Line %d: Invalid token !%c", *line_number, next);
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
                    ErrorExit(ERROR_LEXICAL, "Error in lexical analysis: Line %d: Invalid token [%c", *line_number, next);
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

            /*Beginning of a string*/
            case '"':
                token -> token_type = LITERAL_TOKEN;
                ConsumeLiteral(token, line_number);
                return token;

            /*call GetSymbolType to determine next token*/
            default:
                //return the character back, since the consume functions parse the whole token
                ungetc(c, stdin);

                //call a sub-FSM function depending on the char type
                switch(GetCharType(c)){
                    /*beginning of a identifier*/
                    case CHARACTER:
                        ConsumeIdentifier(token, line_number);
                        return token;

                    case NUMBER:
                        token -> token_type = ConsumeNumber(token, line_number);
                        return token;

                    case WHITESPACE:
                        c = ConsumeWhitespace(line_number);
                        continue;

                    default:
                        DestroyToken(token);
                        ErrorExit(ERROR_LEXICAL, "Error in lexical analysis: Line %d: Invalid token %c", *line_number, c);
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

        case KEYWORD:
            printf("Type: Keyword. Keyword type: %s", (char * )(token -> attribute));
            break;

        default:
            printf("PrintToken() default case");
        
        
    }
}

int main(){
    Token *token; int cnt = 0; int line_number = 1;
    while((token = GetNextToken(&line_number)) -> token_type != EOF_TOKEN){
        cnt ++; if(cnt >= 100) break;
        printf("Token on line %d. Printing token...\n", line_number);
        PrintToken(token);
        printf("\n\n\n\n");
    DestroyToken(token); 
    }

    DestroyToken(token);
}

#endif