// Implementations of operations on stacks for expression parsing and scope parsing
#include <stdio.h>
#include <stdlib.h>

#include "stack.h"
#include "error.h"


// Symtable stack operations
SymtableStack *SymtableStackInit(void) {
    SymtableStack *stack; if((stack = calloc(1, sizeof(SymtableStack))) == NULL){
        ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
    }

    return stack;
}

Symtable *SymtableStackTop(SymtableStack *stack) {
    return stack -> size == 0 ? NULL : stack -> top -> table;
}

void SymtableStackRemoveTop(SymtableStack *stack) {
    if(stack -> size != 0){ // do nothing if stack is empty
        SymtableStackNode *previous_top = stack -> top;
        stack -> top = previous_top -> next; // can also be NULL

        // free resources
        DestroySymtable(previous_top -> table);
        free(previous_top);

        --(stack -> size);
    }
}

Symtable *SymtableStackPop(SymtableStack *stack) {
    // look if the stack is not empty
    if(stack -> size == 0) return NULL;

    // retrieve the top and pop it from the symtable
    Symtable *symtable = SymtableStackTop(stack);
    SymtableStackNode *previous_top = stack -> top;
    stack -> top = previous_top -> next; // can also be NULL

    free(previous_top);

    return symtable;
}

void SymtableStackPush(SymtableStack *stack, Symtable *symtable) {
    SymtableStackNode *node; if((node = malloc(sizeof(SymtableStackNode))) == NULL){
        ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
    }

    // add data
    node -> next = stack -> top;
    node -> table = symtable;

    // push onto the stack and increase size
    stack -> top = node;
    ++(stack -> size);
}

void SymtableStackDestroy(SymtableStack *stack) {
    while(stack -> size != 0){
        SymtableStackRemoveTop(stack);
    }

    free(stack);
}

bool SymtableStackIsEmpty(SymtableStack *stack) {
    return (stack -> size) == 0;
}



// Expression stack operations
ExpressionStack *ExpressionStackInit(void) {
    ExpressionStack *stack; if((stack = calloc(1, sizeof(ExpressionStack))) == NULL){
        ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
    }

    return stack;
}

Token *ExpressionStackTop(ExpressionStack *stack) {
    return stack -> size == 0 ? NULL : stack -> top -> token;
}

void ExpressionStackRemoveTop(ExpressionStack *stack) {
    if(stack -> size != 0){
        ExpressionStackNode *previous_top = stack -> top;
        stack -> top = previous_top -> next;
        // free allocated memory resources
        DestroyToken(previous_top -> token);
        free(previous_top);

        --(stack -> size);
    }
}

Token *ExpressionStackPop(ExpressionStack *stack) {
    // look if the stack is not empty
    if(stack -> size == 0) return NULL;

    // retrieve the top and pop it from the symtable
    Token *token = ExpressionStackTop(stack);
    ExpressionStackNode *previous_top = stack -> top;
    stack -> top = previous_top -> next; // can also be NULL
    --(stack -> size);

    free(previous_top);
    return token;
}

void ExpressionStackPush(ExpressionStack *stack, Token *token) {
    ExpressionStackNode *node; if((node = malloc(sizeof(ExpressionStackNode))) == NULL){
        ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
    }

    // add data
    node -> next = stack -> top;
    node -> token = token;

    // push to the stack and increase size
    stack -> top = node;
    ++(stack -> size);
}

void ExpressionStackDestroy(ExpressionStack *stack) {
    while(stack -> size != 0){
        ExpressionStackRemoveTop(stack);
    }

    free(stack);
}

bool ExpressionStackIsEmpty(ExpressionStack *stack) {
    return (stack -> size) == 0;
}