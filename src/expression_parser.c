#include <stdio.h>
#include <stdlib.h>

#include "expression_parser.h"
#include "vector.h"
#include "scanner.h"
#include "stack.h"
#include "parser.h"
#include "string.h"

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


int GetIntResult(Token *operand_1, Token *operand_2, TOKEN_TYPE operator, Symtable *symtable, bool *zero_division) {
    int value_1 = operand_1 -> token_type == IDENTIFIER_TOKEN ? GetIntValue(operand_1, symtable) : strtol(operand_1 -> attribute, NULL, 10);
    int value_2 = operand_2 -> token_type == IDENTIFIER_TOKEN ? GetIntValue(operand_2, symtable) : strtol(operand_2 -> attribute, NULL, 10);

    // now we can compute the values
    switch(operator){
        case ADDITION_OPERATOR:
            return value_1 + value_2;

        case SUBSTRACTION_OPERATOR:
            return value_2 - value_1; // since they were popped from the stack the second operand is actually first

        case MULTIPLICATION_OPERATOR:
            return value_1 * value_2;

        case DIVISION_OPERATOR:
            if(value_1 == 0){ // division by zero
                *zero_division = true;
                return -1;
            }
            return value_2 / value_1;

        default:
            printf("Daco daco GetIntResult default case\n");
            return 0;
    }
}

double GetDoubleResult(Token *operand_1, Token *operand_2, TOKEN_TYPE operator, Symtable *symtable, bool *zero_division) {
    double value_1 = operand_1 -> token_type == IDENTIFIER_TOKEN ? GetDoubleValue(operand_1, symtable) : strtod(operand_1 -> attribute, NULL);
    double value_2 = operand_2 -> token_type == IDENTIFIER_TOKEN ? GetDoubleValue(operand_2, symtable) : strtod(operand_2 -> attribute, NULL);

    // now we can compute the values
    switch(operator){
        case ADDITION_OPERATOR:
            return value_1 + value_2;

        case SUBSTRACTION_OPERATOR:
            return value_2 - value_1; // since they were popped from the stack the second operand is actually first

        case MULTIPLICATION_OPERATOR:
            return value_1 * value_2;

        case DIVISION_OPERATOR:
            if(value_1 == 0){ // division by zero
                *zero_division = true;
                return -1.0;
            }
            return value_2 / value_1;

        default:
            printf("Daco daco GetIntResult default case\n");
            return 0.0;
    }
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
        if(token == NULL) ErrorExit(ERROR_INTERNAL, "");
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

                while(true){
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
                                if(ExpressionStackTop(stack) -> token_type != L_ROUND_BRACKET && (priority_difference = ComparePriority(token -> token_type, ExpressionStackTop(stack) -> token_type))){
                                    Token *top = ExpressionStackPop(stack);
                                    AppendToken(postfix, top);
                                }

                                else break;
                            }

                            else break;
                        }

                    }

                    else{ // invalid symbol sequence
                        DestroyToken(token);
                        DestroyTokenVector(postfix);
                        ExpressionStackDestroy(stack);

                        // TODO: Possible memory leak: Other structures such as parser aren't freed
                        // TODO 2: Add attributes for all symbols
                        ErrorExit(ERROR_SYNTACTIC, "Line %d: Unexpected symbol in expression", parser -> line_number);
                    }

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
                    DestroyToken(token);
                    DestroyTokenVector(postfix);
                    ExpressionStackDestroy(stack);
                    ErrorExit(ERROR_SYNTACTIC, "Line %d: Unexpected symbol in expression", parser -> line_number);
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
    if(return_value -> value != NULL) free(return_value -> value);
    free(return_value);
}

bool IsTokenInString(TokenVector *postfix, Token *token) {
    for(int i = 0; i < postfix -> length; i++){
        if(token == postfix -> token_string[i]) return true;
    }

    return false;
}

ExpressionReturn *EvaluatePosfixExpression(TokenVector *postfix, Symtable *symtable, Parser parser) {
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
                    DestroySymtable(symtable);
                    ExpressionStackDestroy(stack);
                    DestroyTokenVector(postfix);
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

                        DestroySymtable(symtable);
                        ExpressionStackDestroy(stack);
                        DestroyTokenVector(postfix);
                        exit(ERROR_SEMANTIC_UNDEFINED);
                    }

                    // check if we don't need to update the float_expression flag
                    if(var_1 -> type == DOUBLE64_TYPE) float_expression = true;
                }

                if(operand_2 -> token_type == IDENTIFIER_TOKEN){
                    // have to do it separately, otherwise i wouldn't know which one is NULL :(
                    if((var_2 = FindVariableSymbol(symtable, operand_2 -> attribute)) == NULL){
                        fprintf(stderr, RED"Error in semantic analysis: Line %d: Undefined variable %s\n"RESET, parser.line_number, operand_1 -> attribute);

                        DestroySymtable(symtable);
                        ExpressionStackDestroy(stack);
                        DestroyTokenVector(postfix);
                        exit(ERROR_SEMANTIC_UNDEFINED);
                    }

                    if(var_2 -> type == DOUBLE64_TYPE) float_expression = true;
                }

                // update the type flag if it is needed
                if(operand_1 -> token_type == DOUBLE_64 || operand_2 -> token_type == DOUBLE_64) float_expression = true;

                // compute the result
                bool zero_division = false; // help flag

                if(!float_expression){ // int result
                    int res = GetIntResult(operand_1, operand_2, current_token -> token_type, symtable, &zero_division);

                    // check for errors
                    if(zero_division){
                        fprintf(stderr, RED"Error in semantic analysis: Line %d: Division by zero\n"RESET, parser.line_number);
                        DestroySymtable(symtable);
                        ExpressionStackDestroy(stack);
                        DestroyTokenVector(postfix);
                        exit(ERROR_SEMANTIC_UNDEFINED); // TODO: check if correct exit code
                    }

                    Token *result_token = InitToken();
                    result_token -> token_type = INTEGER_32;
                    // get the string length of the result
                    if((result_token -> attribute = calloc(1, snprintf(NULL, 0, "%d", res) + 1)) == NULL){
                        DestroyToken(result_token);
                        DestroySymtable(symtable);
                        ExpressionStackDestroy(stack);
                        DestroyTokenVector(postfix);
                        ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
                    }

                    // convert the result back to a string
                    sprintf(result_token -> attribute, "%d", res);
                    ExpressionStackPush(stack, result_token);
                }

                else{
                    double res = GetDoubleResult(operand_1, operand_2, current_token -> token_type, symtable, &zero_division);

                    // check for errors
                    if(zero_division){
                        fprintf(stderr, RED"Error in semantic analysis: Line %d: Division by zero\n"RESET, parser.line_number);
                        DestroySymtable(symtable);
                        ExpressionStackDestroy(stack);
                        DestroyTokenVector(postfix);
                        exit(ERROR_SEMANTIC_UNDEFINED); // TODO: check if correct exit code
                    }

                    Token *result_token = InitToken();
                    result_token -> token_type = DOUBLE_64;

                    // string length
                    if((result_token -> attribute = calloc(1, snprintf(NULL, 0, "%lf", res) + 1)) == NULL){
                        DestroyToken(result_token);
                        DestroySymtable(symtable);
                        ExpressionStackDestroy(stack);
                        DestroyTokenVector(postfix);
                        ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
                    }
                    // convert the result back to a string
                    sprintf(result_token -> attribute, "%lf", res);
                    ExpressionStackPush(stack, result_token);
                }

                if(!IsTokenInString(postfix, operand_1)) DestroyToken(operand_1);
                if(!IsTokenInString(postfix, operand_2)) DestroyToken(operand_2);
                break;

            case SEMICOLON: // end condition
                result = ExpressionStackPop(stack);
                ExpressionStackDestroy(stack);
                DestroyTokenVector(postfix);

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

                DestroyToken(result);
                return return_value;

            default:
                printf("default aaayy\n\n");
                return return_value;

        }
    }

    return return_value;
}

#ifdef IFJ24_DEBUG



int main(void){
    Parser parser = {.has_main = false, .in_function = false, .line_number = 1, .symtable = NULL};
    TokenVector *postfix = InfixToPostfix(&parser);
    Symtable *table = InitSymtable(109);
    printf("\n");
    ExpressionReturn *return_value = EvaluatePosfixExpression(postfix, table, parser);
    switch(return_value -> type){
        case INT32_TYPE:
            printf("%d\n", *(int *)(return_value -> value));
            break;
        case DOUBLE64_TYPE:
            printf("%lf\n", *(double *)(return_value -> value));
            break;
        default:
            printf("dfsahudhusa");
    }
    DestroySymtable(table);
    DestroyExpressionReturn(return_value);
}

#endif