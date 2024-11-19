#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "symtable.h"
#include "error.h"

Symtable *InitSymtable(unsigned long size)
{
    Symtable *symtable = malloc(sizeof(Symtable));
    if (!symtable)
    {
        ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
    }

    symtable->capacity = size;
    symtable->size = 0;
    symtable->table = calloc(size, sizeof(HashEntry));
    if (!symtable->table)
    {
        free(symtable);
        ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
    }

    for (unsigned long i = 0; i < size; i++)
    {
        symtable->table[i].is_occupied = false;
        symtable->table[i].symbol = NULL;
    }

    return symtable;
}

void DestroySymtable(Symtable *symtable)
{
    for (unsigned long i = 0; i < symtable->capacity; i++)
    {
        if (symtable->table[i].is_occupied && symtable->table[i].symbol)
        {
            if (symtable->table[i].symbol_type == FUNCTION_SYMBOL)
            {
                DestroyFunctionSymbol((FunctionSymbol *)symtable->table[i].symbol);
            }
            else if (symtable->table[i].symbol_type == VARIABLE_SYMBOL)
            {
                DestroyVariableSymbol((VariableSymbol *)symtable->table[i].symbol);
            }
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

    variable_symbol->type = VOID_TYPE; // Default

    return variable_symbol;
}

VariableSymbol *VariableSymbolCopy(VariableSymbol *var)
{
    VariableSymbol *copy = VariableSymbolInit();

    copy->defined = var->defined;
    copy->is_const = var->is_const;
    copy->name = var->name == NULL ? NULL : strdup(var->name);
    copy->nullable = var->nullable;
    copy->type = var->type;

    return copy;
}

void DestroyFunctionSymbol(FunctionSymbol *function_symbol)
{
    if (function_symbol == NULL)
        return; // just in case

    // if (function_symbol->name != NULL)
    //     free(function_symbol->name);

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

    // if (variable_symbol->name != NULL)
    //     free(variable_symbol->name);

    free(variable_symbol);
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
    return symtable->size == 0;
}

FunctionSymbol *FindFunctionSymbol(Symtable *symtable, char *function_name)
{
    unsigned long index = GetSymtableHash(function_name, symtable->capacity);
    unsigned long start_index = index;

    while (symtable->table[index].is_occupied)
    {
        if (symtable->table[index].symbol_type == FUNCTION_SYMBOL &&
            strcmp(((FunctionSymbol *)symtable->table[index].symbol)->name, function_name) == 0)
        {
            return (FunctionSymbol *)symtable->table[index].symbol;
        }

        index = (index + 1) % symtable->capacity;
        if (index == start_index)
        {
            break;
        }
    }

    return NULL;
}

VariableSymbol *FindVariableSymbol(Symtable *symtable, char *variable_name)
{
    unsigned long index = GetSymtableHash(variable_name, symtable->capacity);
    unsigned long start_index = index;

    while (symtable->table[index].is_occupied)
    {
        if (symtable->table[index].symbol_type == VARIABLE_SYMBOL &&
            strcmp(((VariableSymbol *)symtable->table[index].symbol)->name, variable_name) == 0)
        {
            return (VariableSymbol *)symtable->table[index].symbol;
        }

        index = (index + 1) % symtable->capacity;
        if (index == start_index)
        {
            break; // Avoid infinite loop
        }
    }

    return NULL; // Symbol not found
}

bool InsertVariableSymbol(Symtable *symtable, VariableSymbol *variable_symbol)
{
    unsigned long index = GetSymtableHash(variable_symbol->name, symtable->capacity);
    unsigned long start_index = index;

    if (FindVariableSymbol(symtable, variable_symbol->name) != NULL || FindFunctionSymbol(symtable, variable_symbol->name) != NULL)
    {
        return false; // Symbol already in table
    }

    while (symtable->table[index].is_occupied)
    {
        if (symtable->table[index].symbol_type == VARIABLE_SYMBOL &&
            strcmp(((VariableSymbol *)symtable->table[index].symbol)->name, variable_symbol->name) == 0)
        {
            return false; // Symbol already exists
        }

        index = (index + 1) % symtable->capacity;
        if (index == start_index)
        {
            return false; // Table is full
        }
    }

    symtable->table[index].symbol_type = VARIABLE_SYMBOL;
    symtable->table[index].symbol = variable_symbol;
    symtable->table[index].is_occupied = true;
    symtable->size++;

    return true;
}

bool InsertFunctionSymbol(Symtable *symtable, FunctionSymbol *function_symbol)
{
    unsigned long index = GetSymtableHash(function_symbol->name, symtable->capacity);
    unsigned long start_index = index;

    if (FindVariableSymbol(symtable, function_symbol->name) != NULL || FindFunctionSymbol(symtable, function_symbol->name) != NULL)
    {
        return false; // Symbol already in table
    }

    while (symtable->table[index].is_occupied)
    {
        if (symtable->table[index].symbol_type == FUNCTION_SYMBOL &&
            strcmp(((FunctionSymbol *)symtable->table[index].symbol)->name, function_symbol->name) == 0)
        {
            return false; // Symbol already exists
        }

        index = (index + 1) % symtable->capacity;
        if (index == start_index)
        {
            return false; // Table is full
        }
    }

    symtable->table[index].symbol_type = FUNCTION_SYMBOL;
    symtable->table[index].symbol = function_symbol;
    symtable->table[index].is_occupied = true;
    symtable->size++;

    return true;
}

void PrintTable(Symtable *symtable)
{
    printf("Symbol Table:\n");
    for (unsigned long i = 0; i < symtable->capacity; i++)
    {
        if (symtable->table[i].is_occupied)
        {
            printf("Index %lu: ", i);
            if (symtable->table[i].symbol_type == FUNCTION_SYMBOL)
            {
                FunctionSymbol *func = (FunctionSymbol *)symtable->table[i].symbol;
                printf("Function - Name: %s, Return Type: %d, Parameters: %d\n",
                       func->name, func->return_type, func->num_of_parameters);
            }
            else if (symtable->table[i].symbol_type == VARIABLE_SYMBOL)
            {
                VariableSymbol *var = (VariableSymbol *)symtable->table[i].symbol;
                printf("Variable - Name: %s, Type: %d, Is Const: %s, Nullable: %s, Defined: %s\n",
                       var->name,
                       var->type,
                       var->is_const ? "true" : "false",
                       var->nullable ? "true" : "false",
                       var->defined ? "true" : "false");
            }
        }
        else
        {
            printf("Index %lu: EMPTY\n", i);
        }
    }
}
