#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "scanner.h"
#include "error.h"
#include "vector.h"


Scanner *InitScanner(){
    Scanner *scanner;
    if ((scanner = malloc(sizeof(scanner))) == NULL){
        ErrorExit("Internal compiler error: Memory allocation failed.", ERROR_INTERNAL);
    }

    //default values && return
    scanner -> state = READY;
    scanner -> line_number = 1;

    return scanner;
}

Token *InitToken(){
    Token *token;
    if ((token = calloc(1, sizeof(token))) == NULL){
        ErrorExit("Internal compiler error: Memory allocation failed.", ERROR_INTERNAL);
    }

    //set default value and return
    token -> keyword_type = NONE;
    return token;
}

void ConsumeEqualOperator(Token *token){
    Vector *vector = InitVector(); int c;

    //get the number of equal operators
    while((c = getchar()) == '=')
        AppendChar(vector, c);

    //terminate the string
    AppendChar(vector, '\0');

    //check if the token was valid (so a '=' or '==' operator)
    //if not, free the allocated memory and end the program
    if(!isspace(c) && c != EOF){
        DestroyVector(vector);
        ErrorExit("Error in lexical analysis: invalid token structure.", ERROR_LEXICAL);
    }

    //if the token is valid, return the character to stdin
    ungetc(c, stdin);

    //act depending on the number of counts
    switch(vector -> length){
        case 1:
            token -> token_type = ASSIGNMENT;
            return;

        case 2:
            token -> token_type = EQUAL_OPERATOR;
            return;

        default:
            DestroyVector(vector);
            ErrorExit("Error in lexical analysis: invalid token structure.", ERROR_LEXICAL);

    }
}


Token *GetNextToken(Scanner *scanner){
    //initial variables
    int c; SCANNER_STATE state = READY; Token *token = InitToken();
    
    //skip all the whitespace characters
    while(isspace(c=getchar())){
        continue;
    }

    //return the first non-whitespace character or end the function at the end of the file
    if(c == EOF){
        token -> token_type = EOF_TOKEN;
        token -> keyword_type = NONE;
        return token;
    }

    ungetc(c, stdin);

    switch(c){
        case '=':
            //return the '=' and let the corresponding sub-FSM deal with it
            ungetc(c, stdin);
            ConsumeEqualOperator(token);
    }


}

int main(){
    printf("dfferwf");
}