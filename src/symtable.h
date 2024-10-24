/*Module containing everything related to the symtable's implementation*/

/*Also contains linked list declarations and structures, since a hash table is an array of linked lists*/
#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <stdbool.h>


#define TABLE_COUNT 1009 // first prime over 1000, todo: change this

// symbol type enumeration
typedef enum
{
    FUNCTION_SYMBOL,
    VARIABLE_SYMBOL
} SYMBOL_TYPE;

// IFJ24 data types
typedef enum
{
    U8_ARRAY_TYPE,
    U8_ARRAY_NULLABLE_TYPE,
    INT32_TYPE,
    INT32_NULLABLE_TYPE,
    DOUBLE64_TYPE,
    DOUBLE64_NULLABLE_TYPE,
    BOOLEAN,
    VOID_TYPE,
    TERM_TYPE // For example ifj.write(term) --> can be a float, integer or even null
} DATA_TYPE;

// structure of a variable symbol
typedef struct
{
    char *name;
    DATA_TYPE type;
    bool is_const;
    bool nullable;
    bool defined;
} VariableSymbol;

// structure of a function symbol
typedef struct
{
    char *name;
    int num_of_parameters;
    VariableSymbol **parameters;
    DATA_TYPE return_type;
    bool was_called;
} FunctionSymbol;

// structure of a symbol linked list node
typedef struct node
{
    struct node *next;
    SYMBOL_TYPE symbol_type;
    void *symbol;
} SymtableListNode;

// symtable structure (a hash table --> an array of linked lists)
typedef struct
{
    unsigned long capacity; // the maximum total number of lists
    unsigned long size;     // so the count of lists we have active (non-NULL/length 0)
    SymtableListNode *table[TABLE_COUNT];
} Symtable;

/**
 * @brief Initializes one node of a SymtableLinkedList
 *
 * @param symbol_type Type of the symbol
 * @param symbol Symbol structure. Passed as a void* since it can be one of two types, the type is inferred from symbol_type
 * @return SymtableListNode*
 * @note not a calloc but initialized with parameters since i assume we will be creating a node while already having necessary
 */
SymtableListNode *InitNode(SYMBOL_TYPE symbol_type, void *symbol);

// list node destructor
void DestroyNode(SymtableListNode *node);

// appends a node to the end of a symtable list
void AppendNode(Symtable *symtable, SymtableListNode *node, unsigned long hash);

// removes a node from the end of the list
void PopNode(int *symtable_size, SymtableListNode *list);

// symtable list destructor, frees each node recursively
void DestroyList(SymtableListNode *list);

// symtable constructor with the specified size of linked lists
Symtable *InitSymtable(unsigned long size);

// symtable destructor, calls DestroyList on every SymtableLinkedList
void DestroySymtable(Symtable *symtable);

/**
 * @brief Hash function for the symtable (which is a Hash table)
 *
 * @param symbol_name name of the symbol, used as an input to the hash function
 * @param modulo size of the symtable, the index into the symtable is H(x) % size
 * @note This function is from http://www.cse.yorku.ca/~oz/hash.html -- sdbm variant - also used in my IJC project :)
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

// symbol getters from nodes to avoid the weird void* notation
FunctionSymbol *GetFunctionSymbol(SymtableListNode *node);
VariableSymbol *GetVariableSymbol(SymtableListNode *node);

// better than if(symtable -> size == 0)
bool IsSymtableEmpty(Symtable *symtable);

// looks if the symbol is in the symtable and returns a pointer to it if yes, else returns NULL
FunctionSymbol *FindFunctionSymbol(Symtable *symtable, char *function_name);

// the same but for variables
VariableSymbol *FindVariableSymbol(Symtable *symtable, char *variable_name);

/**
 * @brief Inserts a variable symbol into the symtable
 *
 * @param symtable Pointer to the symtable instance
 * @param variable_symbol Pointer to the symbol to insert
 * @return true If the symbol wasn't in the symtable, the symbol is inserted in this case
 * @return false If the symbol already was in the sytmtable, it's not inserted in this case
 */
bool InsertVariableSymbol(Symtable *symtable, VariableSymbol *variable_symbol);

// The same but for functions
bool InsertFunctionSymbol(Symtable *symtable, FunctionSymbol *function_symbol);

/**
 * @brief Prints the symtable to the stdout
 *
 * @param symtable Pointer to the symtable instance
 * @note This function is for debugging purposes
 */
void PrintTable(Symtable *symtable);

#endif