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
    DOUBLE64_TYPE,
    VOID_TYPE
} DATA_TYPE;


//structure of a variable symbol
typedef struct{
    char *name;
    DATA_TYPE type;
    bool is_declared;
    void *value;
} Variable;


//structure of a function symbol
typedef struct{
    char *name;
    int num_of_parameters;
    Variable **parameters;
    void *return_value;
    DATA_TYPE return_type;
} Function;


//structure of a symbol linked list node
typedef struct node{
    struct node *next;
    SYMBOL_TYPE symbol_type;
    void *symbol;
} SymtableListNode;


//symtable structure (a hash table --> an array of linked lists)
typedef struct{
    unsigned long capacity; //the maximum total number of lists
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
void AppendNode(int *symtable_size, SymtableListNode *list, SymtableListNode *node);


//removes a node from the end of the list
void PopNode(int *symtable_size, SymtableListNode *list);


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
 * @return unsigned long index into the symtable
 */
/*This function is from http://www.cse.yorku.ca/~oz/hash.html -- sdbm variant - also used in my IJC project :)*/
unsigned long GetSymtableHash(char *symbol_name, unsigned long modulo);


//Function symbol constructor
//TODO: (goes for all the symbols) work out whether to pass already params here
Function *FunctionSymbolInit(void);


//Variable symbol constructor
Variable *VariableSymbolInit(void);


//Function symbol destructor
void DestroyFunctionSymbol(Function *function_symbol);


//Variable symbol destructor
void DestroyVariableSymbol(Variable *variable_symbol);


//symbol getters from nodes to avoid the weird void* notation
Function *GetFunctionSymbol(SymtableListNode *node);
Variable *GetVariableSymbol(SymtableListNode *node);


//better than if(symtable -> size == 0)
bool IsSymtableEmpty(Symtable *symtable);


//looks if the symbol is in the symtable and returns a pointer to it if yes, else returns NULL
Function *FindFunctionSymbol(Symtable *symtable, char *function_name);

//the same but for variables
Variable *FindVariableSymbol(Symtable *symtable, char *variable_name);


#endif