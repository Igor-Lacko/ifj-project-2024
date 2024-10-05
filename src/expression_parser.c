#include <stdio.h>
#include <stdlib.h>

#include "expression_parser.h"
#include "vector.h"
#include "scanner.h"
#include "stack.h"
#include "parser.h"
#include "string.h"
#include "math.h"

PrecedenceTable InitPrecedenceTable(void) {
    PrecedenceTable table;

    // highest priority operators
    table.PRIORITY_HIGHEST[0] = MULTIPLICATION_OPERATOR;
    table.PRIORITY_HIGHEST[1] = DIVISION_OPERATOR;

    // middle priority operators
    table.PRIORITY_MIDDLE[0] = ADDITION_OPERATOR;
    table.PRIORITY_MIDDLE[1] = SUBSTRACTION_OPERATOR;

    // lowest priority operators
    table.PRIORITY_LOWEST[0] = EQUAL_OPERATOR;
    table.PRIORITY_LOWEST[1] = NOT_EQUAL_OPERATOR;
    table.PRIORITY_LOWEST[2] = LESS_THAN_OPERATOR;
    table.PRIORITY_LOWEST[3] = LARGER_THAN_OPERATOR;
    table.PRIORITY_LOWEST[4] = LESSER_EQUAL_OPERATOR;
    table.PRIORITY_LOWEST[5] = LARGER_EQUAL_OPERATOR;

    return table;
}

TokenVector *InitTokenVector() {
    TokenVector *vector; if((vector = calloc(1, sizeof(TokenVector))) == NULL){
        ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
    }

    return vector;
}

void AppendToken(TokenVector *vector, Token *input_token) {
    if((vector -> length) + 1 > (vector -> capacity)){
        int new_capacity = ALLOC_CHUNK(vector -> length); // compute the new capacity

        if((vector -> token_string = realloc(vector -> token_string, new_capacity * sizeof(Token*))) == NULL){
            DestroyToken(input_token);
            ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
        }

        vector -> capacity = new_capacity;

        // initalize all pointers in the interval (current, capacity) to NULL
        for(int i = vector -> length; i < vector -> capacity; i++){
            vector -> token_string[i] = NULL;
        }
    }

    (vector -> token_string)[vector -> length ++] = input_token;
}

void DestroyTokenVector(TokenVector *vector) {
    if(vector -> length != 0){
        for(int i = 0; i < vector -> capacity; i++){
            if(vector -> token_string[i] != NULL){
                DestroyToken(vector -> token_string[i]);
            }
        }

    }

    free(vector -> token_string);
    free(vector);
}

int ComparePriority(TOKEN_TYPE operator_1, TOKEN_TYPE operator_2) {
    OPERATOR_PRIORITY priority_1 = LOWEST, priority_2 = LOWEST;

    // first operator check
    if(operator_1 == MULTIPLICATION_OPERATOR || operator_1 == DIVISION_OPERATOR)
        priority_1 = HIGHEST;

    else if(operator_1 == ADDITION_OPERATOR || operator_1 == SUBSTRACTION_OPERATOR)
        priority_1 = MIDDLE;

    // by default it's lowest

    // second operator check
    if(operator_2 == MULTIPLICATION_OPERATOR || operator_2 == DIVISION_OPERATOR)
        priority_2 = HIGHEST;

    else if(operator_2 == ADDITION_OPERATOR || operator_2 == SUBSTRACTION_OPERATOR)
        priority_2 = MIDDLE;

    // compute the return type
    if(priority_1 < priority_2) return -1;
    else if(priority_1 > priority_2) return 1;
    return 0;
}


int GetIntResult(Token *operand_1, Token *operand_2, TOKEN_TYPE operator, Symtable *symtable, bool *zero_division, bool *type_incompatibility) {
    // declare variables to store the values later on
    int value_1, value_2;

    // check for type compatibility
    if(AreTypesIncompatible(operand_1, operand_2, symtable)){
        *type_incompatibility = true;
        return 0.0;
    }

    if(operand_1 -> token_type == IDENTIFIER_TOKEN){
        // check if we don't have to convert it
        VariableSymbol *symbol_1 = FindVariableSymbol(symtable, operand_1 -> attribute);
        if(symbol_1 -> type == DOUBLE64_TYPE) value_1 = (int)*(double *)(symbol_1 -> value);
        else value_1 = *(int *)(symbol_1 -> value);
    }

    else value_1 = strtol(operand_1 -> attribute, NULL, 10);

    // same but for value 2
    if(operand_2 -> token_type == IDENTIFIER_TOKEN){
        VariableSymbol *symbol_2 = FindVariableSymbol(symtable, operand_2 -> attribute);
        if(symbol_2 -> type == DOUBLE64_TYPE) value_2 = (int)*(double *)(symbol_2 -> value);
        else value_2 = *(int *)(symbol_2 -> value);
    }

    else value_2 = strtol(operand_2 -> attribute, NULL, 10);

    // now we can compute the values
    switch(operator){
        case ADDITION_OPERATOR:
            return value_1 + value_2;

        case SUBSTRACTION_OPERATOR:
            return value_2 - value_1; // since they were popped from the stack the second operand is actually first

        case MULTIPLICATION_OPERATOR:
            return value_1 * value_2;

        case DIVISION_OPERATOR:
            if(value_1 == 0.0){ // division by zero
                *zero_division = true;
                return -1;
            }
            return value_2 / value_1;

        default:
            ErrorExit(ERROR_INTERNAL, "Non-arithmetic operator passed to GetIntResult, this should never happen!");
    }

    return 0;
}

double GetDoubleResult(Token *operand_1, Token *operand_2, TOKEN_TYPE operator, Symtable *symtable, bool *zero_division, bool *type_incompatibility) {
    // variables to store the results
    double value_1, value_2;

    // check for type compatibility
    if(AreTypesIncompatible(operand_1, operand_2, symtable)){
        *type_incompatibility = true;
        return 0.0;
    }

    if(operand_1 -> token_type == IDENTIFIER_TOKEN){
        // check if we don't have to convert it
        VariableSymbol *symbol_1 = FindVariableSymbol(symtable, operand_1 -> attribute);
        if(symbol_1 -> type == INT32_TYPE) value_1 = (double)*(int *)(symbol_1 -> value);
        else value_1 = *(double *)(symbol_1 -> value);
    }

    else value_1 = strtod(operand_1 -> attribute, NULL);

    if(operand_2 -> token_type == IDENTIFIER_TOKEN){
        VariableSymbol *symbol_2 = FindVariableSymbol(symtable, operand_2 -> attribute);
        if(symbol_2 -> type == INT32_TYPE) value_2 = (double)*(int *)(symbol_2 -> value);
        else value_2 = *(double *)(symbol_2 -> value);
    }

    else value_2 = strtod(operand_2 -> attribute, NULL);

    // now we can compute the values
    switch(operator){
        case ADDITION_OPERATOR:
            return value_1 + value_2;

        case SUBSTRACTION_OPERATOR:
            return value_2 - value_1; // since they were popped from the stack the second operand is actually first

        case MULTIPLICATION_OPERATOR:
            return value_1 * value_2;

        case DIVISION_OPERATOR:
            if(fabs(value_1) < EPSILON){ // division by zero
                *zero_division = true;
                return -1.0;
            }

            return value_2 / value_1;

        default:
            ErrorExit(ERROR_INTERNAL, "Non-arithmetic operator passed to GetDoubleResult, this should never happen!");
    }

    return 0.0;
}

bool GetBooleanResult(Token *operand_1, Token *operand_2, TOKEN_TYPE operator, Symtable *symtable, bool float_expression, bool *nullable, bool *incompaible_types) {
    // check for type compatibility
    // if one of them is nullable
    if(IsNullable(operand_1, symtable) || IsNullable(operand_2, symtable)){
        *nullable = true;
        return false;
    }

    // if their types aren't compatible
    if(AreTypesIncompatible(operand_1, operand_2, symtable)){
        *incompaible_types = true;
        return false;
    }

    // we have to essentialy go two paths (either the operands are ints or doubles)
    double double_1, double_2;
    int int_1, int_2;

    // compute the double values, TODO: type compatibility check
    // note to type checking: we can compare f64 < 2 where 2 is converted to 2.0, but we can't compare i32 < 1.5 (if we don't know the i32 value on compile-time)
    if(float_expression){
        // compute the first value
        if(operand_1 -> token_type != IDENTIFIER_TOKEN) double_1 = strtod(operand_1 -> attribute, NULL);
        else if(operand_1 -> token_type == INTEGER_32) double_1 = (double)*(int *)(FindVariableSymbol(symtable, operand_1 -> attribute));
        else double_1 = *(double *)(FindVariableSymbol(symtable, operand_1 -> attribute));

        // compute the second value
        if(operand_2 -> token_type != IDENTIFIER_TOKEN) double_2 = strtod(operand_2 -> attribute, NULL);
        else if(operand_2 -> token_type == INTEGER_32) double_2 = (double)*(int *)(FindVariableSymbol(symtable, operand_2 -> attribute));
        else double_2 = *(double *)(FindVariableSymbol(symtable, operand_2 -> attribute));
    }

    else{
        // compute the first value
        if(operand_1 -> token_type != IDENTIFIER_TOKEN) int_1 = strtol(operand_1 -> attribute, NULL, 10);
        else int_1 = *(int *)(FindVariableSymbol(symtable, operand_1 -> attribute));

        // compute the second value
        if(operand_2 -> token_type != IDENTIFIER_TOKEN) int_2 = strtol(operand_2 -> attribute, NULL, 10);
        else int_2 = *(int *)(FindVariableSymbol(symtable, operand_2 -> attribute));
    }

    // return value depending on the operator
    switch(operator){
        case EQUAL_OPERATOR:
            return float_expression ? double_1 == double_2 : int_1 == int_2;

        case NOT_EQUAL_OPERATOR:
            return float_expression ? double_1 != double_2 : int_1 != int_2;

        case LESS_THAN_OPERATOR:
            return float_expression ? double_2 < double_1 : int_2 < int_1; // since the operands are actually in the reverse order (popped from the stack)

        case LARGER_THAN_OPERATOR:
            return float_expression ? double_2 > double_1 : int_2 > int_1;

        case LESSER_EQUAL_OPERATOR:
            return float_expression ? double_2 <= double_1 : int_2 <= int_1;

        case LARGER_EQUAL_OPERATOR:
            return float_expression ? double_2 >= double_1 : int_2 >= int_1;

        default: // this should really never happen because of the explicit case in EvaluatePostfixExpression(), if it does we screwed something up
            ErrorExit(ERROR_INTERNAL, "Non-relational operator passed to GetBooleanResult, this should never happen!");
    }

    return false;
}

bool IsNullable(Token *operand, Symtable *symtable) {
    if(operand -> token_type != IDENTIFIER_TOKEN) return false; // a constant literal
    else if(!FindVariableSymbol(symtable, operand -> attribute) -> nullable) return false; // non-nullable variable
    return true; // nullable variable
}

bool AreTypesIncompatible(Token *operand_1, Token *operand_2, Symtable *symtable) {
    if(operand_1 -> token_type != IDENTIFIER_TOKEN && operand_2 -> token_type != IDENTIFIER_TOKEN) return false; // constants, we know their value

    // help var symbols
    VariableSymbol *symbol_1 = NULL, *symbol_2 = NULL;

    // get the symbols (if they exist)
    if(operand_1 -> token_type == IDENTIFIER_TOKEN) symbol_1 = FindVariableSymbol(symtable, operand_1 -> attribute);
    if(operand_2 -> token_type == IDENTIFIER_TOKEN) symbol_2 = FindVariableSymbol(symtable, operand_2 -> attribute);

    // they are incompatible if they are symbols of different types, or if one is a int variable and the other is a float constant
    // the first case
    if(symbol_1 != NULL && symbol_2 != NULL){
        if(symbol_1 -> type != symbol_2 -> type) return true;
    }

    // the second case
    else if(symbol_1 != NULL){ // only symbol_1 is an variable
        if(symbol_1 -> type == INT32_TYPE && operand_2 -> token_type == DOUBLE_64) return true;
    }

    else if(symbol_2 != NULL){ // only symbol 2 is an variable
        if(symbol_2 -> type == INT32_TYPE && operand_1 -> token_type == DOUBLE_64) return true;
    }

    return false; // both are constants, we can safely compare them by either normal float/int comparision or converting the int operand to float
}

TokenVector *InfixToPostfix(Parser *parser) {
    int line_start = parser -> line_number; // In case we encounter a newline, multi-line expressions are (probably not supported)
    int bracket_count = 0; // In case the expression ends
    int priority_difference; // Token priority difference


    // needed structures
    Token *token; 
    TokenVector *postfix = InitTokenVector();
    ExpressionStack *stack = ExpressionStackInit();

    while(((token = GetNextToken(&parser -> line_number)) -> token_type) != SEMICOLON){
        if(parser -> line_number > line_start){
            DestroyToken(token);
            DestroyTokenVector(postfix);
            ExpressionStackDestroy(stack);
            ErrorExit(ERROR_SYNTACTIC, "Line %d: Expression not ended correctly", line_start);
        }

        switch(token -> token_type){
            // operand tokens
            case(INTEGER_32):       case(IDENTIFIER_TOKEN):     case(DOUBLE_64):
                AppendToken(postfix, token);
                break;


            // left bracket
            case(L_ROUND_BRACKET):
                ExpressionStackPush(stack, token);
                bracket_count ++;
                break;
            
            // operator tokens
            case(MULTIPLICATION_OPERATOR):      case(DIVISION_OPERATOR):
            case(ADDITION_OPERATOR):            case(SUBSTRACTION_OPERATOR):        
            case(EQUAL_OPERATOR):               case(NOT_EQUAL_OPERATOR):
            case(LESS_THAN_OPERATOR):           case(LESSER_EQUAL_OPERATOR):
            case(LARGER_THAN_OPERATOR):         case(LARGER_EQUAL_OPERATOR):


                if(ExpressionStackIsEmpty(stack) ||
                ExpressionStackTop(stack) -> token_type == L_ROUND_BRACKET ||
                (priority_difference = ComparePriority(token -> token_type, ExpressionStackTop(stack) -> token_type)) == 1){
                    ExpressionStackPush(stack, token);
                    break;
                }

                else if(priority_difference == 0 || priority_difference == 1 || !ExpressionStackIsEmpty(stack)){
                    // pop the operators with higher/equal priority from the stack to the end of the postfix string
                    //while((priority_difference = ComparePriority(token -> token_type, ExpressionStackTop(stack) -> token_type)) <= 0 && !ExpressionStackIsEmpty(stack) && ExpressionStackTop(stack) -> token_type != L_ROUND_BRACKET){
                    while(true){
                        if(!ExpressionStackIsEmpty(stack)){
                            if(ExpressionStackTop(stack) -> token_type != L_ROUND_BRACKET && (priority_difference = ComparePriority(token -> token_type, ExpressionStackTop(stack) -> token_type)) <= 0){
                                Token *top = ExpressionStackPop(stack);
                                AppendToken(postfix, top);
                            }

                            else{
                                ExpressionStackPush(stack, token);
                                break;
                            }
                        }

                        else{
                            ExpressionStackPush(stack, token);
                            break;
                        }
                    }

                }

                else{ // invalid symbol sequence
                    fprintf(stderr, RED"Error in syntax analysis: Line %d: Unexpected symbol '%s' in expression\n"RESET, parser->line_number, token -> attribute);
                    DestroyToken(token);
                    DestroyStackAndVector(postfix, stack);
                    exit(ERROR_SYNTACTIC);

                    // TODO: Possible memory leak: Other structures such as parser aren't freed
                    // TODO 2: Add attributes for all symbols
                }

                break;

            // right bracket
            case(R_ROUND_BRACKET):
                // expression over case
                if(--bracket_count < 0){
                    DestroyToken(token);
                    ExpressionStackDestroy(stack);
                    ungetc(')', stdin);
                    return postfix;
                }

                // pop the characters from the stack until we encounter a left bracket
                Token *top = ExpressionStackPop(stack);
                while(top -> token_type != L_ROUND_BRACKET) {
                    // invalid end of expression
                    if(ExpressionStackIsEmpty(stack)){
                        DestroyToken(token);
                        DestroyToken(top);
                        DestroyTokenVector(postfix);
                        ExpressionStackDestroy(stack);
                        ErrorExit(ERROR_SYNTACTIC, "Line %d: Expression not ended correctly", line_start);
                    }


                    // append the token to the vector
                    AppendToken(postfix, top);
                    top = ExpressionStackPop(stack);

                }

                DestroyToken(token);
                DestroyToken(top);
                break;

            default:
                fprintf(stderr, RED"Error in syntax analysis: Line %d: Unexpected symbol in expression\n"RESET, parser -> line_number);
                DestroyToken(token);
                DestroyStackAndVector(postfix, stack);
                exit(ERROR_SYNTACTIC);
        }
    }


    while(!ExpressionStackIsEmpty(stack)){
        Token *top = ExpressionStackPop(stack);
        AppendToken(postfix, top); 
    }

    AppendToken(postfix, token);
    ungetc(';', stdin);

    ExpressionStackDestroy(stack);
    return postfix;
}

ExpressionReturn *InitExpressionReturn(void) {
    ExpressionReturn *retvalue;
    if((retvalue = calloc(1, sizeof(ExpressionReturn))) == NULL) {
        ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
    }

    return retvalue;
}

void DestroyExpressionReturn(ExpressionReturn *return_value) {
    ///if(return_value -> value != NULL) free(return_value -> value); since this is usually either not allocated or given to the symbol
    free(return_value);
}

bool IsTokenInString(TokenVector *postfix, Token *token) {
    for(int i = 0; i < postfix -> length; i++){
        if(token == postfix -> token_string[i]) return true;
    }

    return false;
}

void DestroyStackAndVector(TokenVector *postfix, ExpressionStack *stack) {
    while(stack -> size != 0) {
        Token *top = ExpressionStackPop(stack);
        if(!IsTokenInString(postfix, top)) DestroyToken(top);
    }

    DestroyTokenVector(postfix);
    ExpressionStackDestroy(stack);
}

ExpressionReturn *EvaluatePostfixExpression(TokenVector *postfix, Symtable *symtable, Parser parser) {
    // later return value
    ExpressionReturn *return_value = InitExpressionReturn();

    // expression type flag and variable symbols for checking if tokens are in symtable, also a stack which we will need
    bool float_expression = false;
    ExpressionStack *stack = ExpressionStackInit();

    // operand and result tokens
    Token *operand_1 = NULL, *operand_2 = NULL, *result = NULL;

    // variable symbol instances if needed
    VariableSymbol *var_1 = NULL, *var_2 = NULL;

    // loop through the string from left to right
    for(int i = 0; i < postfix -> length; i++){
        Token *current_token = postfix -> token_string[i]; // access the current token

        switch(current_token -> token_type){
            case INTEGER_32: case DOUBLE_64:
                ExpressionStackPush(stack, current_token);
                // check if we don't need to update the boolean/floating flags
                if(current_token -> token_type == DOUBLE_64) float_expression = true;
                break;

            case IDENTIFIER_TOKEN: // put on top of stack
                // check if the token is in the symtable
                if((var_1 = FindVariableSymbol(symtable, current_token -> attribute)) == NULL){
                    // error message
                    fprintf(stderr, RED"Error in semantic analysis: Line %d: Undefined variable %s\n"RESET, parser.line_number, current_token -> attribute);
                    // free allocated resources
                    if(!IsTokenInString(postfix, operand_1)) DestroyToken(operand_1);
                    if(!IsTokenInString(postfix, operand_2)) DestroyToken(operand_2);
                    DestroySymtable(symtable);
                    DestroyStackAndVector(postfix, stack);
                    DestroyExpressionReturn(return_value);
                    exit(ERROR_SEMANTIC_UNDEFINED);
                }

                if(var_1 -> type == DOUBLE64_TYPE) float_expression = true;

                ExpressionStackPush(stack, current_token);

                // check if we don't need to update the boolean/floating flags
                if(current_token -> token_type == DOUBLE_64) float_expression = true;
                break;

            // arithmetic operators, push out 2 tokens from the stack and evaluate the result
            case MULTIPLICATION_OPERATOR: case DIVISION_OPERATOR:
            case ADDITION_OPERATOR: case SUBSTRACTION_OPERATOR:
                // pop the operands from the stack
                operand_1 = ExpressionStackPop(stack);
                operand_2 = ExpressionStackPop(stack);

                // if the operands are identifiers, we need to check for them in the symtable
                if(operand_1 -> token_type == IDENTIFIER_TOKEN){
                    if((var_1 = FindVariableSymbol(symtable, operand_1 -> attribute)) == NULL){
                        fprintf(stderr, RED"Error in semantic analysis: Line %d: Undefined variable %s\n"RESET, parser.line_number, operand_1 -> attribute);
                        if(!IsTokenInString(postfix, operand_1)) DestroyToken(operand_1);
                        if(!IsTokenInString(postfix, operand_2)) DestroyToken(operand_2);
                        DestroySymtable(symtable);
                        DestroyStackAndVector(postfix, stack);
                        DestroyExpressionReturn(return_value);
                        exit(ERROR_SEMANTIC_UNDEFINED);
                    }

                    // check if we don't need to update the float_expression flag
                    if(var_1 -> type == DOUBLE64_TYPE) float_expression = true;
                }

                if(operand_2 -> token_type == IDENTIFIER_TOKEN){
                    // have to do it separately, otherwise i wouldn't know which one is NULL :(
                    if((var_2 = FindVariableSymbol(symtable, operand_2 -> attribute)) == NULL){
                        fprintf(stderr, RED"Error in semantic analysis: Line %d: Undefined variable %s\n"RESET, parser.line_number, operand_1 -> attribute);
                        if(!IsTokenInString(postfix, operand_1)) DestroyToken(operand_1);
                        if(!IsTokenInString(postfix, operand_2)) DestroyToken(operand_2);
                        DestroySymtable(symtable);
                        DestroyStackAndVector(postfix, stack);
                        DestroyExpressionReturn(return_value);
                        exit(ERROR_SEMANTIC_UNDEFINED);
                    }

                    if(var_2 -> type == DOUBLE64_TYPE) float_expression = true;
                }

                // update the type flag if it is needed
                if(operand_1 -> token_type == DOUBLE_64 || operand_2 -> token_type == DOUBLE_64) float_expression = true;

                // compute the result
                bool zero_division = false, type_incompatiblity = false; // help flags

                if(!float_expression){ // int result
                    int res = GetIntResult(operand_1, operand_2, current_token -> token_type, symtable, &zero_division, &type_incompatiblity);

                    // check for errors
                    if(zero_division){
                        fprintf(stderr, RED"Error in semantic analysis: Line %d: Division by zero\n"RESET, parser.line_number);
                        if(!IsTokenInString(postfix, operand_1)) DestroyToken(operand_1);
                        if(!IsTokenInString(postfix, operand_2)) DestroyToken(operand_2);
                        DestroySymtable(symtable);
                        DestroyStackAndVector(postfix, stack);
                        DestroyExpressionReturn(return_value);
                        exit(ERROR_SEMANTIC_OTHER); // TODO: check if correct exit code
                    }

                    else if(type_incompatiblity){
                        fprintf(stderr, RED"Error in semantic analysis: Line %d: Incompatible operands '%s' and '%s'\n"RESET, parser.line_number, operand_1 -> attribute, operand_1 -> attribute);
                        if(!IsTokenInString(postfix, operand_1)) DestroyToken(operand_1);
                        if(!IsTokenInString(postfix, operand_2)) DestroyToken(operand_2);
                        DestroySymtable(symtable);
                        DestroyStackAndVector(postfix, stack);
                        DestroyExpressionReturn(return_value);
                        exit(ERROR_SEMANTIC_TYPE_COMPATIBILITY); // TODO: check if correct exit code
                    }

                    Token *result_token = InitToken();
                    result_token -> token_type = INTEGER_32;
                    // get the string length of the result
                    if((result_token -> attribute = calloc(1, snprintf(NULL, 0, "%d", res) + 1)) == NULL){
                        DestroyToken(result_token);
                        if(!IsTokenInString(postfix, operand_1)) DestroyToken(operand_1);
                        if(!IsTokenInString(postfix, operand_2)) DestroyToken(operand_2);
                        DestroySymtable(symtable);
                        DestroyStackAndVector(postfix, stack);
                        DestroyExpressionReturn(return_value);
                        ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
                    }

                    // convert the result back to a string
                    sprintf(result_token -> attribute, "%d", res);
                    ExpressionStackPush(stack, result_token);
                }

                else{
                    double res = GetDoubleResult(operand_1, operand_2, current_token -> token_type, symtable, &zero_division, &type_incompatiblity);

                    // check for errors
                    if(zero_division){
                        fprintf(stderr, RED"Error in semantic analysis: Line %d: Division by zero\n"RESET, parser.line_number);
                        if(!IsTokenInString(postfix, operand_1)) DestroyToken(operand_1);
                        if(!IsTokenInString(postfix, operand_2)) DestroyToken(operand_2);
                        DestroySymtable(symtable);
                        DestroyStackAndVector(postfix, stack);
                        DestroyExpressionReturn(return_value);
                        exit(ERROR_SEMANTIC_OTHER); // TODO: check if correct exit code
                    }

                    else if(type_incompatiblity){
                        fprintf(stderr, RED"Error in semantic analysis: Line %d: Incompatible operands '%s' and '%s'\n"RESET, parser.line_number, operand_1 -> attribute, operand_2 -> attribute);
                        if(!IsTokenInString(postfix, operand_1)) DestroyToken(operand_1);
                        if(!IsTokenInString(postfix, operand_2)) DestroyToken(operand_2);
                        DestroySymtable(symtable);
                        DestroyStackAndVector(postfix, stack);
                        DestroyExpressionReturn(return_value);
                        exit(ERROR_SEMANTIC_TYPE_COMPATIBILITY); // TODO: check if correct exit code
                    }

                    Token *result_token = InitToken();
                    result_token -> token_type = DOUBLE_64;

                    // string length
                    if((result_token -> attribute = calloc(1, snprintf(NULL, 0, "%lf", res) + 1)) == NULL){
                        DestroyToken(result_token);
                        DestroySymtable(symtable);
                        if(!IsTokenInString(postfix, operand_1)) DestroyToken(operand_1);
                        if(!IsTokenInString(postfix, operand_2)) DestroyToken(operand_2);
                        DestroyStackAndVector(postfix, stack);
                        DestroyExpressionReturn(return_value);
                        ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
                    }
                    // convert the result back to a string
                    sprintf(result_token -> attribute, "%lf", res);
                    ExpressionStackPush(stack, result_token);
                }

                if(!IsTokenInString(postfix, operand_1)) DestroyToken(operand_1);
                if(!IsTokenInString(postfix, operand_2)) DestroyToken(operand_2);
                break;

            // Boolean operators
            case LESS_THAN_OPERATOR: case LESSER_EQUAL_OPERATOR:
            case LARGER_THAN_OPERATOR: case LARGER_EQUAL_OPERATOR:
            case EQUAL_OPERATOR: case NOT_EQUAL_OPERATOR:

                // so far do the same as with arithmetic operators, pop two operands from the stack
                operand_1 = ExpressionStackPop(stack);
                operand_2 = ExpressionStackPop(stack);

                // if the operands are identifiers, we need to check for them in the symtable
                if(operand_1 -> token_type == IDENTIFIER_TOKEN){
                    if((var_1 = FindVariableSymbol(symtable, operand_1 -> attribute)) == NULL){
                        fprintf(stderr, RED"Error in semantic analysis: Line %d: Undefined variable %s\n"RESET, parser.line_number, operand_1 -> attribute);
                        if(!IsTokenInString(postfix, operand_1)) DestroyToken(operand_1);
                        if(!IsTokenInString(postfix, operand_2)) DestroyToken(operand_2);
                        DestroySymtable(symtable);
                        DestroyStackAndVector(postfix, stack);
                        DestroyExpressionReturn(return_value);
                        exit(ERROR_SEMANTIC_UNDEFINED);
                    }

                    // check if we don't need to update the float_expression flag
                    if(var_1 -> type == DOUBLE64_TYPE) float_expression = true;
                }

                if(operand_2 -> token_type == IDENTIFIER_TOKEN){
                    // have to do it separately, otherwise i wouldn't know which one is NULL :(
                    if((var_2 = FindVariableSymbol(symtable, operand_2 -> attribute)) == NULL){
                        fprintf(stderr, RED"Error in semantic analysis: Line %d: Undefined variable %s\n"RESET, parser.line_number, operand_1 -> attribute);
                        if(!IsTokenInString(postfix, operand_1)) DestroyToken(operand_1);
                        if(!IsTokenInString(postfix, operand_2)) DestroyToken(operand_2);
                        DestroySymtable(symtable);
                        DestroyStackAndVector(postfix, stack);
                        DestroyExpressionReturn(return_value);
                        exit(ERROR_SEMANTIC_UNDEFINED);
                    }

                    if(var_2 -> type == DOUBLE64_TYPE) float_expression = true;
                }

                // update the type flag if it is needed
                if(operand_1 -> token_type == DOUBLE_64 || operand_2 -> token_type == DOUBLE_64) float_expression = true;

                // error flags for GetBooleanResult()
                bool nullable = false, incompatible_types = false;

                // comical amount of arguments
                bool boolexpr_result = GetBooleanResult(operand_1, operand_2, 
                current_token -> token_type, symtable, float_expression, 
                &nullable, &incompatible_types);

                (void)boolexpr_result;

                // check for errors
                if(nullable){
                    if(!IsTokenInString(postfix, operand_1)) DestroyToken(operand_1);
                    if(!IsTokenInString(postfix, operand_2)) DestroyToken(operand_2);
                    DestroySymtable(symtable);
                    DestroyStackAndVector(postfix, stack);
                    DestroyExpressionReturn(return_value);
                    ErrorExit(ERROR_SEMANTIC_TYPE_COMPATIBILITY, "Line %d: Nullable type in relational expression", parser.line_number);
                }

                if(incompatible_types){
                    if(!IsTokenInString(postfix, operand_1)) DestroyToken(operand_1);
                    if(!IsTokenInString(postfix, operand_2)) DestroyToken(operand_2);
                    DestroySymtable(symtable);
                    DestroyStackAndVector(postfix, stack);
                    DestroyExpressionReturn(return_value);
                    ErrorExit(ERROR_SEMANTIC_TYPE_COMPATIBILITY, "Line %d: Incompatible types in relational expression", parser.line_number);
                }

                // since IFJ24 doesn't support AND/OR operators, if we reach a boolean operator it's the end
                // otherwise it would be a syntax error (probably)
                if(!ExpressionStackIsEmpty(stack) || (postfix -> token_string[++i] -> token_type != SEMICOLON && postfix -> token_string[i] -> token_type != R_ROUND_BRACKET)){
                    if(!IsTokenInString(postfix, operand_1)) DestroyToken(operand_1);
                    if(!IsTokenInString(postfix, operand_2)) DestroyToken(operand_2);
                    DestroySymtable(symtable);
                    DestroyStackAndVector(postfix, stack);
                    DestroyExpressionReturn(return_value);
                    ErrorExit(ERROR_SYNTACTIC, "Line %d: Invalid expression construction", parser.line_number);
                }

                if(!IsTokenInString(postfix, operand_1)) DestroyToken(operand_1);
                if(!IsTokenInString(postfix, operand_2)) DestroyToken(operand_2);
                DestroyStackAndVector(postfix, stack);

                return_value -> type = BOOLEAN;
                return_value -> value = (bool *)(&boolexpr_result);
                return return_value;

            case SEMICOLON: // end condition
                result = ExpressionStackPop(stack);

                if(float_expression) {
                    double res_value = strtod(result -> attribute, NULL);
                    if((return_value -> value = malloc(sizeof(double))) == NULL) {
                        ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
                    }

                    return_value -> type = DOUBLE64_TYPE;
                    *(double *)(return_value -> value) = res_value;
                }

                else {
                    int res_value = strtol(result -> attribute, NULL, 10);
                    if((return_value -> value = malloc(sizeof(int))) == NULL) {
                        ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
                    }

                    return_value -> type = INT32_TYPE;
                    *(int *)(return_value -> value) = res_value;
                }

                if(!IsTokenInString(postfix, result)) DestroyToken(result);
                DestroyStackAndVector(postfix, stack);

                return return_value;

            default: // TODO: Error
                printf("default aaayy\n\n");
                return return_value;

        }
    }

    return return_value;
}

// debug funcs, remove later
void PrintPostfix(TokenVector *postfix){
    for(int i = 0; i < postfix->length; i++){
        printf("%s ",postfix->token_string[i]->attribute);
    }

    printf("\n");
}

void PrintResult(ExpressionReturn *return_value){
    switch(return_value -> type){
        case(INT32_TYPE):
            printf("returned value is: %d\n", *(int *)return_value -> value);
            break;
        case(DOUBLE64_TYPE):
            printf("returned value is: %lf\n", *(double *)return_value -> value);
            break;

        case(BOOLEAN):
            printf("returned value is: ");
            *(bool *)return_value -> value == true ? printf("true\n") : printf("false\n");
            break;

        default:
            printf("Ta so\n");
            break;
    }
}

#ifdef IFJ24_DEBUG



int main(void){
    Parser parser = {.has_main = false, .in_function = false, .line_number = 1, .symtable = NULL};
    TokenVector *postfix = InfixToPostfix(&parser);
    PrintPostfix(postfix);
    Symtable *table = InitSymtable(109);
    ExpressionReturn *retval = EvaluatePostfixExpression(postfix, table, parser);
    PrintResult(retval);
    DestroySymtable(table);
    DestroyExpressionReturn(retval);
}

#endif