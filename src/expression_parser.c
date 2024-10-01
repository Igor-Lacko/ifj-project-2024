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

Vector *InfixToPostfix(Parser *parser) {
    int line_start = parser -> line_number; // In case we encounter a newline, multi-line expressions are (probably not supported)
    int bracket_count = 0; // In case the expression ends
    int priority_difference; // Token priority difference

    bool is_boolean = false; // If a boolean operation is encountered
    bool convert_to_int = false; // For type compatibility

    // needed structures
    Token *token;
    Vector *postfix = InitVector();
    ExpressionStack *stack = ExpressionStackInit();

    while(((token = GetNextToken(&parser -> line_number)) -> token_type) != SEMICOLON){
        if(parser -> line_number > line_start){
            DestroyToken(token);
            DestroyVector(postfix);
            ExpressionStackDestroy(stack);
            ErrorExit(ERROR_SYNTACTIC, "Line %d: Expression not ended correctly", line_start);
        }

        switch(token -> token_type){
            // operand tokens
            case(INTEGER_32):       case(IDENTIFIER_TOKEN):     case(DOUBLE_64):
                for(unsigned i = 0; i < strlen(token -> attribute); i++){
                    AppendChar(postfix, token -> attribute[i]);
                }

                if(token -> token_type == DOUBLE_64) convert_to_int = true;
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
                        while(priority_difference <= 0 && !ExpressionStackIsEmpty(stack) && ExpressionStackTop(stack) -> token_type != L_ROUND_BRACKET){
                            Token *top = ExpressionStackPop(stack);

                            //check 
                            // copy all characters from the input token to the vector
                            for(unsigned i = 0; i < strlen(top -> attribute); i++){
                                AppendChar(postfix, top -> attribute[i]);
                            }
                        }

                    }

                    else{ // invalid symbol sequence
                        DestroyToken(token);
                        DestroyVector(postfix);
                        ExpressionStackDestroy(stack);

                        // TODO: Possible memory leak: Other structures such as parser aren't freed
                        // TODO 2: Add attributes for all symbols
                        ErrorExit(ERROR_SYNTACTIC, "Line %d: Unexpected symbol in expression", parser -> line_number);
                    }

                }
                    break;

                // right bracket
                case(R_ROUND_BRACKET):
                    do{ // pop the characters from the stack until we encounter a left bracket
                        // expression over case
                        if(--bracket_count < 0){
                            DestroyToken(token);
                            ExpressionStackDestroy(stack);
                            ungetc(')', stdin);
                            return postfix;
                        }

                        // invalid end of expression
                        if(token -> token_type == EOF_TOKEN || ExpressionStackIsEmpty(stack)){
                            DestroyToken(token);
                            DestroyVector(postfix);
                            ExpressionStackDestroy(stack);
                            ErrorExit(ERROR_SYNTACTIC, "Line %d: Expression not ended correctly", line_start);
                        }

                        Token *top = ExpressionStackPop(stack);

                        // append the token to the vector
                        for(unsigned i = 0; i < strlen(top -> attribute); i++){
                            AppendChar(postfix, top -> attribute[i]);
                        }
                    } while(token -> token_type != R_ROUND_BRACKET);

                    ExpressionStackRemoveTop(stack); // remove the right bracket from the stack
                    break;

                default:
                    DestroyToken(token);
                    DestroyVector(postfix);
                    ExpressionStackDestroy(stack);
                    ErrorExit(ERROR_SYNTACTIC, "Line %d: Unexpected symbol in expression", parser -> line_number);
        }
    }

    ungetc(';', stdin);
    DestroyToken(token);

    while(!ExpressionStackIsEmpty(stack)){
        Token *top = ExpressionStackPop(stack);
        for(unsigned i = 0; i < strlen(top -> attribute); i++){
            AppendChar(postfix, top -> attribute[i]);
        }
    }

    ExpressionStackDestroy(stack);
    if(convert_to_int || is_boolean) printf("dhasuhgdushdaiu\n");
    return postfix;
}

#ifdef IFJ24_DEBUG

int main(void){
    Parser parser = {.has_main = false, .in_function = false, .line_number = 1, .symtable = NULL};
    Vector *postfix = InfixToPostfix(&parser);
    printf("postfix value: %s\n", postfix -> value);
    DestroyVector(postfix);
}

#endif