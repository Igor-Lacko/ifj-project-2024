/**
 * @file symtable.h
 * @brief Header file for the symbol table module.
 *
 * This file provides declarations for the symbol table (symtable), implemented as
 * a hash table using open addressing for collision resolution. It includes
 * functions for managing symbols (both variables and functions).
 *
 * ### Notes:
 * - The hash function used for the symtable is based on the sdbm variant from
 *   http://www.cse.yorku.ca/~oz/hash.html.
 *
 * @authors
 * - Boris Semanco [xseman06]
 */

#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "types.h"

// symtable constructor with the specified size of linked lists
Symtable *InitSymtable(unsigned long size);

// symtable destructor, calls DestroyList on every SymtableLinkedList
void DestroySymtable(Symtable *symtable);

/**
 * @brief Hash function for the symtable (which is a Hash table)
 *
 * @param symbol_name name of the symbol, used as an input to the hash function
 * @param modulo size of the symtable, the index into the symtable is H(x) % size
 * @note This function is from http://www.cse.yorku.ca/~oz/hash.html -- sdbm variant
 * @return unsigned long index into the symtable
 */
unsigned long GetSymtableHash(char *symbol_name, unsigned long modulo);

// FunctionSymbol symbol constructor
FunctionSymbol *FunctionSymbolInit(void);

// VariableSymbol symbol constructor
VariableSymbol *VariableSymbolInit(void);

// Returns a copy of the variable symbol passed as a param to avoid double frees
VariableSymbol *VariableSymbolCopy(VariableSymbol *var);

// FunctionSymbol symbol destructor
void DestroyFunctionSymbol(FunctionSymbol *function_symbol);

// VariableSymbol symbol destructor
void DestroyVariableSymbol(VariableSymbol *variable_symbol);

// better than if(symtable -> size == 0)
bool IsSymtableEmpty(Symtable *symtable);

// looks if the symbol is in the symtable and returns a pointer to it if yes, else returns NULL
FunctionSymbol *FindFunctionSymbol(Symtable *symtable, char *function_name);

// the same but for variables
VariableSymbol *FindVariableSymbol(Symtable *symtable, char *variable_name);

/**
 * @brief Inserts a variable symbol into the symtable
 *
 * @param parser Pointer to parser structure
 * @param variable_symbol Pointer to the symbol to insert
 */
void InsertVariableSymbol(Parser *parser, VariableSymbol *variable_symbol);

// The same but for functions
bool InsertFunctionSymbol(Parser *parser, FunctionSymbol *function_symbol);

/**
 * @brief Prints the symtable to the stdout
 *
 * @param symtable Pointer to the symtable instance
 * @note This function is for debugging purposes
 */
void PrintTable(Symtable *symtable);

// Initializes a string array
void InitStringArray(StringArray *string_array);
// Appends a string to the string array
void AppendStringArray(StringArray *string_array, char *string);
#endif