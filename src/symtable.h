/*Module containing everything related to the symtable's implementation*/

/*Also contains linked list declarations and structures, since a hash table is an array of linked lists*/
#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <stdbool.h>

#define TABLE_COUNT 1009 //first prime over 1000, todo: change this


//symbol type enumeration
typedef enum{
    FUNCTION_SYMBOL,
    VARIABLE_SYMBOL
} SYMBOL_TYPE;


//IFJ24 data types
typedef enum{
    U8_ARRAY_TYPE,
    INT32_TYPE,
    DOUBLE64_TYPE
} DATA_TYPE;


//structure of a function symbol
typedef struct{
    char *name;
    void **parameters;
    void *return_value;
} Function;


//structure of a variable symbol
typedef struct{
    char *name;
    DATA_TYPE type;
    bool is_declared;
    void *value;
} Variable;


//structure of a symbol linked list node
typedef struct node{
    struct node *next;
    SYMBOL_TYPE symbol_type;
    void *symbol;
} SymtableListNode;


//symtable structure (a hash table --> an array of linked lists)
typedef struct{
    unsigned long size; //so the count of lists we have active (non-NULL/length 0)
    SymtableListNode **table;
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


//list node destructor
void DestroyNode(SymtableListNode *node);


//appends a node to the end of a symtable list
void AppendNode(SymtableListNode *list, SymtableListNode *node);


//removes a node from the end of the list
void PopNode(SymtableListNode *list);


//removes a specific node from the list
void RemoveNode(SymtableListNode *list, SymtableListNode *node);


//symtable list destructor, frees each node recursively
void DestroyList(SymtableListNode *list);


//symtable constructor with the specified size of linked lists
Symtable *InitSymtable(size_t size);


//symtable destructor, calls DestroyList on every SymtableLinkedList
void DestroySymtable(Symtable *symtable);


/**
 * @brief Hash function for the symtable (which is a Hash table)
 * 
 * @param symbol_name name of the symbol, used as an input to the hash function
 * @param modulo size of the symtable, the index into the symtable is H(x) % size
 * @return int index into the symtable
 */
int GetSymtableHash(char *symbol_name, int modulo);


//Function symbol constructor
//TODO: (goes for all the symbols) work out whether to pass already params here
Function *FunctionSymbolInit(void);


//Variable symbol constructor
Variable *VariableSymbolInit();


//Function symbol destructor
void DestroyFunctionSymbol(Function *function_symbol);


//Variable symbol destructor
void DestroyVariableSymbol(Variable *variable_symbol);

#endif