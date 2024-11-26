// Implementations of operations on stacks for expression parsing and scope parsing
#include <stdio.h>
#include <stdlib.h>

#include "stack.h"
#include "error.h"
#include "symtable.h"
#include "scanner.h"
#include "shared.h"
#include "vector.h"

// Symtable stack operations
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
                    // SymtableStackDestroy(parser->symtable_stack);
                    DestroySymtable(parser->global_symtable);
                    DestroyTokenVector(stream);
                    ErrorExit(ERROR_SEMANTIC_UNUSED_VARIABLE, "Warning: Variable '%s' was declared but never used. %d", var->name, parser->line_number);
                    return;
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

// Expression stack operations
ExpressionStack *ExpressionStackInit(void)
{
    ExpressionStack *stack;
    if ((stack = calloc(1, sizeof(ExpressionStack))) == NULL)
    {
        ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
    }

    return stack;
}

Token *ExpressionStackTop(ExpressionStack *stack)
{
    return stack->size == 0 ? NULL : stack->top->token;
}

void ExpressionStackRemoveTop(ExpressionStack *stack)
{
    if (stack->size != 0)
    {
        ExpressionStackNode *previous_top = stack->top;
        stack->top = previous_top->next;
        // free allocated memory resources
        DestroyToken(previous_top->token);
        free(previous_top);

        --(stack->size);
    }
}

Token *ExpressionStackPop(ExpressionStack *stack)
{
    // look if the stack is not empty
    if (stack->size == 0)
        return NULL;

    // retrieve the top and pop it from the symtable
    Token *token = ExpressionStackTop(stack);
    ExpressionStackNode *previous_top = stack->top;
    stack->top = previous_top->next; // can also be NULL
    --(stack->size);

    free(previous_top);
    return token;
}

void ExpressionStackPush(ExpressionStack *stack, Token *token)
{
    ExpressionStackNode *node;
    if ((node = malloc(sizeof(ExpressionStackNode))) == NULL)
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

void ExpressionStackDestroy(ExpressionStack *stack)
{
    while (stack->size != 0)
    {
        ExpressionStackRemoveTop(stack);
    }

    free(stack);
}

bool ExpressionStackIsEmpty(ExpressionStack *stack)
{
    return (stack->size) == 0;
}