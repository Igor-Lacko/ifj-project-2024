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

void AppendNode(int *symtable_size, SymtableListNode *list, SymtableListNode *node)
{
    SymtableListNode *current_node = list;

    // if the list is empty, set the node to the first item
    if (list == NULL)
    {
        ++(*symtable_size);
        list = node;
        return;
    }

    // jump through the nodes until the next one is empty
    while (current_node->next != NULL)
    {
        current_node = current_node->next;
    }

    current_node->next = node;
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
    if ((symtable->table = calloc(size, sizeof(SymtableListNode *))) == NULL)
    {
        ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
    }

    symtable->capacity = size;
    return symtable;
}

void DestroySymtable(Symtable *symtable)
{
    for (unsigned long i = 0; i < symtable->size; i++)
    {
        if (symtable->table[i] != NULL)
        {
            DestroyList(symtable->table[i]);
            symtable->size--;
        }
    }

    free(symtable->table);
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

    return variable_symbol;
}

void DestroyFunctionSymbol(FunctionSymbol *function_symbol)
{
    if (function_symbol == NULL)
        return; // just in case

    free(function_symbol->name);
    free(function_symbol->return_value);

    // free all parameters
    for (int i = 0; i < function_symbol->num_of_parameters; i++)
    {
        free(function_symbol->parameters[i]);
    }

    free(function_symbol->parameters);

    free(function_symbol);
}

void DestroyVariableSymbol(VariableSymbol *variable_symbol)
{
    if (variable_symbol == NULL)
        return;

    free(variable_symbol->name);
    free(variable_symbol->value);

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

bool InsertVariableSymbol(Symtable *symtable, VariableSymbol *variable_symbol){
    if(FindVariableSymbol(symtable, variable_symbol -> name) != NULL) return false;

    SymtableListNode *node = InitNode(FUNCTION_SYMBOL, (void *)(variable_symbol));
    AppendNode(symtable -> size, symtable -> table[GetSymtableHash(variable_symbol -> name, symtable -> capacity)], node);

    return true;
}


bool InsertFunctionSymbol(Symtable *symtable, FunctionSymbol *function_symbol){
    if(FindVariableSymbol(symtable, function_symbol -> name) != NULL) return false;

    SymtableListNode *node = InitNode(VARIABLE_SYMBOL, (void *)(function_symbol));
    AppendNode(symtable -> size, symtable -> table[GetSymtableHash(function_symbol -> name, symtable -> capacity)], node);

    return true;
}