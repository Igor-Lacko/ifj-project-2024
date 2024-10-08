#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "symtable.h"
#include "error.h"

SymtableListNode *InitNode(SYMBOL_TYPE symbol_type, void *symbol)
{
    SymtableListNode *node;
    if ((node = malloc(sizeof(SymtableListNode))) == NULL)
    {
        ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
    }

    node->symbol_type = symbol_type;
    node->symbol = symbol;
    node->next = NULL;

    return node;
}

void DestroyNode(SymtableListNode *node)
{
    // just in case, check if it wasn't called on an empty node
    if (node == NULL)
        return;

    if (node->symbol != NULL)
    {
        // I LOVE THE TERNARY OPERATOR
        node->symbol_type == FUNCTION_SYMBOL ? DestroyFunctionSymbol((FunctionSymbol *)node->symbol) : DestroyVariableSymbol((VariableSymbol *)node->symbol);
    }

    node->next = NULL;

    free(node);
}

void AppendNode(Symtable *symtable, SymtableListNode *node, unsigned long hash)
{
    // if the list is empty, set the node to the first item
    if ((symtable->table[hash]) == NULL)
    {
        symtable->table[hash] = node;
        ++(symtable->size);
        return;
    }

    SymtableListNode *tmp = symtable->table[hash];

    // jump through the nodes until the next one is empty
    while (tmp->next != NULL)
    {
        tmp = tmp->next;
    }
    tmp->next = node;
}

void PopNode(int *symtable_size, SymtableListNode *list)
{
    SymtableListNode *current_node = list;

    // look if the item to pop isn't the list head
    if (list->next == NULL)
    {
        --(*symtable_size);
        DestroyNode(list);
        list = NULL;
    }

    // we need to look 2 nodes forward, since we need to free current_node -> next and set it to NULL
    while (current_node->next->next != NULL)
    {
        current_node = current_node->next;
    }

    DestroyNode(current_node->next);
    current_node->next = NULL;
}

void DestroyList(SymtableListNode *list)
{
    // recursion woohoo
    if (list->next != NULL)
        DestroyList(list->next);
    DestroyNode(list);
}

Symtable *InitSymtable(size_t size)
{
    Symtable *symtable;
    if ((symtable = calloc(1, sizeof(Symtable))) == NULL)
    {
        ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
    }

    // allocate size linked lists and set each one to NULL
    for (unsigned long i = 0; i < symtable->capacity; i++)
    {
        symtable->table[i] = NULL;
    }

    symtable->capacity = size;
    return symtable;
}

void DestroySymtable(Symtable *symtable)
{
    for (unsigned long i = 0; i < symtable->capacity; i++)
    {
        if (symtable->table[i] != NULL)
        {
            DestroyList(symtable->table[i]);
            symtable->size--;
        }
    }

    free(symtable);
}

FunctionSymbol *FunctionSymbolInit(void)
{
    FunctionSymbol *function_symbol;
    if ((function_symbol = calloc(1, sizeof(FunctionSymbol))) == NULL)
    {
        ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
    }

    return function_symbol;
}

VariableSymbol *VariableSymbolInit(void)
{
    VariableSymbol *variable_symbol;
    if ((variable_symbol = calloc(1, sizeof(VariableSymbol))) == NULL)
    {
        ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
    }

    variable_symbol->type = VOID_TYPE; // Default

    return variable_symbol;
}

void DestroyFunctionSymbol(FunctionSymbol *function_symbol)
{
    if (function_symbol == NULL)
        return; // just in case

    if (function_symbol->name != NULL)
        free(function_symbol->name);
    if (function_symbol->return_value != NULL)
        free(function_symbol->return_value);

    // free all parameters
    for (int i = 0; i < function_symbol->num_of_parameters; i++)
    {
        if (function_symbol->parameters[i] != NULL)
        {
            DestroyVariableSymbol(function_symbol->parameters[i]);
        }
    }

    if (function_symbol->parameters != NULL)
        free(function_symbol->parameters);

    free(function_symbol);
}

void DestroyVariableSymbol(VariableSymbol *variable_symbol)
{
    if (variable_symbol == NULL)
        return;

    if (variable_symbol->name != NULL)
        free(variable_symbol->name);

    free(variable_symbol);
}

FunctionSymbol *GetFunctionSymbol(SymtableListNode *node)
{
    return (FunctionSymbol *)(node->symbol);
}

VariableSymbol *GetVariableSymbol(SymtableListNode *node)
{
    return (VariableSymbol *)(node->symbol);
}

unsigned long GetSymtableHash(char *symbol_name, unsigned long modulo)
{
    unsigned int hash = 0;
    const unsigned char *p;
    for (p = (const unsigned char *)symbol_name; *p != '\0'; p++)
    {
        hash = 65599 * hash + *p;
    }

    return hash % modulo;
}

bool IsSymtableEmpty(Symtable *symtable)
{
    return !(symtable->size);
}

FunctionSymbol *FindFunctionSymbol(Symtable *symtable, char *function_name)
{
    // index into the table using a hash function on the function name
    SymtableListNode *symtable_row = symtable->table[GetSymtableHash(function_name, symtable->capacity)];

    // we will store/commpare here
    FunctionSymbol *function_symbol;

    while (symtable_row != NULL)
    {
        // check if the node's symbol is a function
        if (symtable_row->symbol_type == FUNCTION_SYMBOL)
        {
            if (!strcmp((function_symbol = GetFunctionSymbol(symtable_row))->name, function_name))
                return function_symbol;
        }

        symtable_row = symtable_row->next;
    }
    // symbol not found
    return NULL;
}

VariableSymbol *FindVariableSymbol(Symtable *symtable, char *variable_name)
{
    // index into the table using a hash function on the variable name
    SymtableListNode *symtable_row = symtable->table[GetSymtableHash(variable_name, symtable->capacity)];

    // we will store/commpare here
    VariableSymbol *variable_symbol;

    while (symtable_row != NULL)
    {
        // check if the node's symbol is a function
        if (symtable_row->symbol_type == VARIABLE_SYMBOL)
        {
            if (!strcmp((variable_symbol = GetVariableSymbol(symtable_row))->name, variable_name))
                return variable_symbol;
        }

        symtable_row = symtable_row->next;
    }
    // symbol not found
    return NULL;
}

bool InsertVariableSymbol(Symtable *symtable, VariableSymbol *variable_symbol)
{
    // check if the symbol isn't in the table already
    if (FindVariableSymbol(symtable, variable_symbol->name) != NULL || FindFunctionSymbol(symtable, variable_symbol->name) != NULL)
        return false;

    // if not, create a new node and add it to the end
    SymtableListNode *node = InitNode(VARIABLE_SYMBOL, (void *)(variable_symbol));
    AppendNode(symtable, node, GetSymtableHash(variable_symbol->name, symtable->capacity));

    return true;
}

bool InsertFunctionSymbol(Symtable *symtable, FunctionSymbol *function_symbol)
{
    // the same principle as InsertVariableSymbol()
    if (FindFunctionSymbol(symtable, function_symbol->name) != NULL || FindVariableSymbol(symtable, function_symbol->name) != NULL)
        return false;

    SymtableListNode *node = InitNode(FUNCTION_SYMBOL, (void *)(function_symbol));
    AppendNode(symtable, node, GetSymtableHash(function_symbol->name, symtable->capacity));

    return true;
}

void PrintTable(Symtable *symtable)
{
    printf("Table:\n");
    for (int i = 0; i < (int)symtable->capacity; i++)
    {
        SymtableListNode *current = symtable->table[i];

        while (current != NULL)
        {
            printf("Hash %d : ", i);

            if (current->symbol_type == FUNCTION_SYMBOL)
            {
                printf("Function: %s ", ((FunctionSymbol *)current->symbol)->name);
                printf("Return type: %d\n", ((FunctionSymbol *)current->symbol)->return_type);
                for (int j = 0; j < ((FunctionSymbol *)current->symbol)->num_of_parameters; j++)
                {
                    printf("Parameter: %s ", ((FunctionSymbol *)current->symbol)->parameters[j]->name);
                    printf("Type: %d\n", ((FunctionSymbol *)current->symbol)->parameters[j]->type);
                }
            }
            else
            {
                printf("Variable: %s\n", ((VariableSymbol *)current->symbol)->name);
            }
            current = current->next;
        }
    }
}