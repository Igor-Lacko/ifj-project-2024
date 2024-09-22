#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "scanner.h"

int GetTokenLength(char* token){
    //pointer to the starting position should remain unchanged
    char* current; int length = 0;

    //loop through the token until we encounter a delimeter (whitespace)
    while(!isspace(*(current = token))){
        length ++;
        current ++; //move one char forward
    }

    return length;
}

int main(){
    int c;
    while((c = getchar()) != EOF){
        while(isspace(c)) continue; //consume whitespace
    }
}