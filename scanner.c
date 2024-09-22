#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "scanner.h"

int GetTokenLength(char* token){
    //pointer to the starting position should remain unchanged
    char* current; int length;

    //loop through the token until we encounter a delimeter (whitespace)
    while(!isspace(*(current = token))){
        
    }
}