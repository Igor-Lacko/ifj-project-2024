#include <stdio.h>

#include "expression_parser.h"
#include "vector.h"
#include "scanner.h"

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

