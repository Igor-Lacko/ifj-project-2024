/**
 * @file stack.c
 * @brief Stack operations for expression parsing and scope management.
 *
 * This file contains the implementation of stack-based data structures
 * used for expression parsing and scope management. It includes operations
 * for managing symbol tables (for scope management), expression parsing
 * (handling operator precedence and evaluation), and evaluation of
 * expressions during parsing.
 *
 * Authors:
 * - Igor Lacko [xlackoi00]
 * - Boris Semanco [xseman06]
 * - Jakub Pogádl [xpogad00]
 */

// Implementations of operations on stacks for expression parsing and scope parsing
#include <stdio.h>
#include <stdlib.h>

#include "stack.h"
#include "error.h"
#include "symtable.h"
#include "scanner.h"
#include "shared.h"
#include "vector.h"
#include "expression_parser.h"

/*********************** SYMTABLE STACK OPERATIONS ********************/

SymtableStack *SymtableStackInit(void)
{
    SymtableStack *stack;
    if ((stack = calloc(1, sizeof(SymtableStack))) == NULL)
    {
        ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
    }

    return stack;
}

Symtable *SymtableStackTop(SymtableStack *stack)
{
    return stack->size == 0 ? NULL : stack->top->table;
}

void SymtableStackRemoveTop(Parser *parser)
{
    SymtableStack *stack = parser->symtable_stack;
    if (stack->size == 0)
        return; // do nothing if stack is empty

    SymtableStackNode *previous_top = stack->top;
    Symtable *s = previous_top->table;
    for (unsigned long i = 0; i < s->capacity; i++)
    {
        if (s->table[i].is_occupied)
        {
            // Check if the symbol is a variable
            if (s->table[i].symbol_type == VARIABLE_SYMBOL)
            {
                VariableSymbol *var = (VariableSymbol *)s->table[i].symbol;

                if (!var->was_used)
                {
                    PrintError("Warning: Variable '%s' was declared but never used. %d", var->name, parser->line_number);
                    SymtableStackDestroy(parser->symtable_stack);
                    DestroySymtable(parser->global_symtable);
                    DestroyTokenVector(stream);
                    exit(ERROR_SEMANTIC_UNUSED_VARIABLE);
                }
            }
        }
    }

    stack->top = previous_top->next; // can also be NULL

    // Free resources
    DestroySymtable(previous_top->table);
    free(previous_top);

    --(stack->size);
}

void SymtableStackPop(SymtableStack *stack)
{

    if (stack->size != 0)
    { // do nothing if stack is empty
        SymtableStackNode *previous_top = stack->top;
        stack->top = previous_top->next; // can also be NULL

        // free resources
        DestroySymtable(previous_top->table);
        free(previous_top);

        --(stack->size);
    }
}

void SymtableStackPush(SymtableStack *stack, Symtable *symtable)
{
    SymtableStackNode *node;
    if ((node = malloc(sizeof(SymtableStackNode))) == NULL)
    {
        ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
    }

    // add data
    node->next = stack->top;
    node->table = symtable;

    // push onto the stack and increase size
    stack->top = node;
    ++(stack->size);
}

void SymtableStackDestroy(SymtableStack *stack)
{
    while (stack->size != 0)
    {
        SymtableStackPop(stack);
    }

    free(stack);
}

bool SymtableStackIsEmpty(SymtableStack *stack)
{
    return (stack->size) == 0;
}

// function for finding a variable in the stack
VariableSymbol *SymtableStackFindVariable(SymtableStack *stack, char *name)
{
    // For expression intermediate results which don't have a name
    if (name == NULL)
        return NULL;

    SymtableStackNode *current = stack->top;
    while (current != NULL)
    {
        VariableSymbol *var = FindVariableSymbol(current->table, name);
        if (var != NULL)
            return var;
        current = current->next;
    }
    return NULL;
}

void SymtableStackPrint(SymtableStack *stack)
{
    fprintf(stdout, "Symtable stack:\n");
    SymtableStackNode *current = stack->top;
    while (current != NULL)
    {
        PrintTable(current->table);
        current = current->next;
    }
}

/********************** EXPRESSION STACK OPERATIONS *********************/

ExpressionStack *ExpressionStackInit(void)
{
    ExpressionStack *stack;
    if ((stack = calloc(1, sizeof(ExpressionStack))) == NULL)
    {
        ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
    }

    return stack;
}

ExpressionStackNode *ExpressionStackNodeInit(Token *token, STACK_NODE_TYPE node_type, PtableKey key)
{
    ExpressionStackNode *node;
    if ((node = calloc(1, sizeof(ExpressionStackNode))) == NULL)
    {
        ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
    }

    node->token = token;
    node->node_type = node_type;
    node->key_type = key;

    return node;
}

ExpressionStackNode *ExpressionStackTop(ExpressionStack *stack)
{
    return stack->size == 0 ? NULL : stack->top;
}

void ExpressionStackRemoveTop(ExpressionStack *stack)
{
    if (stack->size != 0)
    {
        ExpressionStackNode *previous_top = stack->top;
        stack->top = previous_top->next;
        if (previous_top->token != NULL)
            DestroyToken(previous_top->token);
        // free allocated memory resources
        free(previous_top);

        --(stack->size);
    }
}

ExpressionStackNode *ExpressionStackPop(ExpressionStack *stack)
{
    // look if the stack is not empty
    if (stack->size == 0)
        return NULL;

    // retrieve the top and pop it from the symtable
    ExpressionStackNode *previous_top = stack->top;
    stack->top = previous_top->next; // can also be NULL
    --(stack->size);

    return previous_top;
}

ExpressionStackNode *TopmostTerminal(ExpressionStack *stack)
{
    ExpressionStackNode *current = stack->top;
    while (current != NULL)
    {
        if (current->node_type == TERMINAL)
            return current;
        current = current->next;
    }
    return NULL;
}

int TopmostHandleDistance(ExpressionStack *stack)
{
    int distance = 0; // Should be already set to 0, but just in case
    ExpressionStackNode *current = stack->top;
    while (current != NULL)
    {
        if (current->node_type == HANDLE)
            return distance;

        ++distance;
        current = current->next;
    }
    return -1; // No handle found, probably an error anyway?
}

void ExpressionStackPush(ExpressionStack *stack, ExpressionStackNode *node)
{
    // add data
    node->next = stack->top;

    // push to the stack and increase size
    stack->top = node;
    ++(stack->size);
}

void PushHandleAfterTopmost(ExpressionStack *stack)
{
    // Get the topmost terminal
    ExpressionStackNode *topmost = TopmostTerminal(stack);
    if (topmost == NULL)
        ErrorExit(ERROR_INTERNAL, "Stack has no terminals!");

    // Create a new handle node
    ExpressionStackNode *handle = ExpressionStackNodeInit(NULL, HANDLE, GetPtableKey(topmost->token, 1));

    if (stack->top == topmost)
    {
        (++stack->size);
        handle->next = stack->top;
        stack->top = handle;
        return;
    }

    // Insert the handle after the topmost terminal
    ExpressionStackNode *current = stack->top;

    while ((current->next != NULL) && (current->next != topmost))
    {
        current = current->next;
    }

    if (current->next == NULL)
        ErrorExit(ERROR_INTERNAL, "Topmost terminal not found in the stack!");

    current->next = handle;
    handle->next = topmost;
    ++(stack->size);
}

void ExpressionStackDestroy(ExpressionStack *stack)
{
    while (stack->size != 0)
    {
        ExpressionStackRemoveTop(stack);
    }

    free(stack);
}

void ExpressionStackClear(ExpressionStack *stack, TokenVector *postfix)
{
    ExpressionStackNode *current = stack->top;
    while (current != NULL)
    {
        if (current->token != NULL && !IsTokenInString(postfix, current->token))
        {
            DestroyToken(current->token);
            current->token = NULL;
        }
        current = current->next;
    }
}

bool ExpressionStackIsEmpty(ExpressionStack *stack)
{
    return (stack->size) == 0;
}

void ExpressionStackPrint(ExpressionStack *stack)
{
    fprintf(stderr, "Expression stack:\n");
    ExpressionStackNode *current = stack->top;
    while (current != NULL)
    {
        if (current->node_type == TERMINAL)
        {
            fprintf(stderr, "Terminal: ");
            current->token == NULL ? fprintf(stderr, "$\n") : fprintf(stderr, "%s\n", current->token->attribute);
        }
        else if (current->node_type == HANDLE)
            fprintf(stderr, "Handle\n");
        else
            fprintf(stderr, "Non-terminal\n");
        current = current->next;
    }

    fprintf(stderr, "\n");
}

/********************** EVALUATION STACK OPERATIONS *********************/

EvaluationStack *EvaluationStackInit(void)
{
    EvaluationStack *stack;
    if ((stack = calloc(1, sizeof(EvaluationStack))) == NULL)
    {
        ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
    }

    return stack;
}

Token *EvaluationStackTop(EvaluationStack *stack)
{
    return stack->size == 0 ? NULL : stack->top->token;
}

void EvaluationStackRemoveTop(EvaluationStack *stack)
{
    if (stack->size != 0)
    {
        EvaluationStackNode *previous_top = stack->top;
        stack->top = previous_top->next;
        // free allocated memory resources
        if (previous_top->token != NULL)
            DestroyToken(previous_top->token);
        free(previous_top);

        --(stack->size);
    }
}

void EvaluationStackPush(EvaluationStack *stack, Token *token)
{
    EvaluationStackNode *node;
    if ((node = calloc(1, sizeof(EvaluationStackNode))) == NULL)
    {
        ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
    }

    // add data
    node->next = stack->top;
    node->token = token;

    // push to the stack and increase size
    stack->top = node;
    ++(stack->size);
}

Token *EvaluationStackPop(EvaluationStack *stack)
{
    // look if the stack is not empty
    if (stack->size == 0)
        return NULL;

    // retrieve the top
    EvaluationStackNode *previous_top = stack->top;
    stack->top = previous_top->next; // can also be NULL
    --(stack->size);

    Token *token = previous_top->token;
    free(previous_top);

    return token;
}

void EvaluationStackDestroy(EvaluationStack *stack)
{
    while (stack->size != 0)
    {
        EvaluationStackRemoveTop(stack);
    }

    free(stack);
}

bool EvaluationStackIsEmpty(EvaluationStack *stack)
{
    return (stack->size) == 0;
}