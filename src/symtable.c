/**
 * @file symtable.c
 * @brief Implementation of the symbol table (symtable) module.
 *
 * This file contains the implementation of the symtable, which uses a hash table
 * with open addressing for symbol management. It provides functions for initializing,
 * inserting, searching, and destroying symbols.
 *
 * @authors
 * - Boris Semanco [xseman06]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "symtable.h"
#include "error.h"
#include "types.h"
#include "stack.h"
#include "shared.h"
#include "vector.h"

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

void InitStringArray(StringArray *string_array)
{
    string_array->count = 0;
    string_array->capacity = 10; // Initial capacity
    string_array->strings = calloc(string_array->capacity, sizeof(char *));
    if (!string_array->strings)
    {
        ErrorExit(ERROR_INTERNAL, "Memory allocation for string array failed");
    }
}

void AppendStringArray(StringArray *string_array, char *string)
{
    // Check if the vector needs to be resized
    if (string_array->count >= string_array->capacity)
    {
        size_t new_capacity = string_array->capacity * 2;
        char **new_strings = realloc(string_array->strings, new_capacity * sizeof(char *));
        if (!new_strings)
        {
            ErrorExit(ERROR_INTERNAL, "Memory reallocation for string array failed");
        }
        string_array->strings = new_strings;
        string_array->capacity = new_capacity;
    }

    // Check if the string is already in the array
    for (int i = 0; i < string_array->count; i++)
    {
        if (strcmp(string_array->strings[i], string) == 0)
        {
            return; // String already in the array
        }
    }

    // Add the string to the vector
    string_array->strings[string_array->count] = strdup(string);
    if (!string_array->strings[string_array->count])
    {
        ErrorExit(ERROR_INTERNAL, "Memory allocation for string failed");
    }
    string_array->count++;
}

FunctionSymbol *FunctionSymbolInit(void)
{
    FunctionSymbol *function_symbol;
    if ((function_symbol = calloc(1, sizeof(FunctionSymbol))) == NULL)
    {
        ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
    }
    InitStringArray(&function_symbol->variables);

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

    if (function_symbol->name != NULL)
        free(function_symbol->name);

    // free all parameters
    for (int i = 0; i < function_symbol->num_of_parameters; i++)
    {
        if (function_symbol->parameters[i] != NULL)
        {
            DestroyVariableSymbol(function_symbol->parameters[i]);
        }
    }

    // free the array of variables
    for (int i = 0; i < function_symbol->variables.count; i++)
    {
        free(function_symbol->variables.strings[i]);
    }

    free(function_symbol->variables.strings);

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

    if (variable_symbol->value != NULL)
        free(variable_symbol->value);

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

void InsertVariableSymbol(Parser *parser, VariableSymbol *variable_symbol)
{
    unsigned long index = GetSymtableHash(variable_symbol->name, parser->symtable->capacity);
    unsigned long start_index = index;

    if (SymtableStackFindVariable(parser->symtable_stack, variable_symbol->name) != NULL || FindFunctionSymbol(parser->global_symtable, variable_symbol->name) != NULL)
    {
        // Symbol already in table
        SymtableStackDestroy(parser->symtable_stack);
        DestroySymtable(parser->global_symtable);
        DestroyTokenVector(stream);
        ErrorExit(ERROR_SEMANTIC_REDEFINED, "Variable already %s declared on line %d", variable_symbol->name, parser->line_number);
    }

    while (parser->symtable->table[index].is_occupied)
    {
        if (parser->symtable->table[index].symbol_type == VARIABLE_SYMBOL &&
            strcmp(((VariableSymbol *)parser->symtable->table[index].symbol)->name, variable_symbol->name) == 0)
        {
            // Symbol already exists
            SymtableStackDestroy(parser->symtable_stack);
            DestroySymtable(parser->global_symtable);
            DestroyTokenVector(stream);
            ErrorExit(ERROR_SEMANTIC_REDEFINED, "Variable already %s declared on line %d", variable_symbol->name, parser->line_number);
        }

        index = (index + 1) % parser->symtable->capacity;
        if (index == start_index)
        {
            // Table is full
            SymtableStackDestroy(parser->symtable_stack);
            DestroySymtable(parser->global_symtable);
            DestroyTokenVector(stream);
            ErrorExit(ERROR_INTERNAL, "Symbol table is full");
        }
    }

    parser->symtable->table[index].symbol_type = VARIABLE_SYMBOL;
    parser->symtable->table[index].symbol = variable_symbol;
    parser->symtable->table[index].is_occupied = true;
    parser->symtable->size++;
}

bool InsertFunctionSymbol(Parser *parser, FunctionSymbol *function_symbol)
{
    unsigned long index = GetSymtableHash(function_symbol->name, parser->global_symtable->capacity);
    unsigned long start_index = index;

    if (SymtableStackFindVariable(parser->symtable_stack, function_symbol->name) != NULL || FindFunctionSymbol(parser->global_symtable, function_symbol->name) != NULL)
    {
        return false; // Symbol already in table
    }

    while (parser->global_symtable->table[index].is_occupied)
    {
        if (parser->global_symtable->table[index].symbol_type == FUNCTION_SYMBOL &&
            strcmp(((FunctionSymbol *)parser->global_symtable->table[index].symbol)->name, function_symbol->name) == 0)
        {
            return false; // Symbol already exists
        }

        index = (index + 1) % parser->global_symtable->capacity;
        if (index == start_index)
        {
            return false; // Table is full
        }
    }

    parser->global_symtable->table[index].symbol_type = FUNCTION_SYMBOL;
    parser->global_symtable->table[index].symbol = function_symbol;
    parser->global_symtable->table[index].is_occupied = true;
    parser->global_symtable->size++;

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
