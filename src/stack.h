// Contains stack structures for expression parsing and scope recognition

#ifndef STACK_H
#define STACK_H


#include "scanner.h" // Tokens for expression parsing
#include "symtable.h" // Symtable stack for scope recognition

typedef struct SymStackNode { // Linked list for symtable stack implementation
    Symtable *table;
    struct SymStackNode *next;
} SymtableStackNode;

typedef struct ExprStackNode { // Linked list for expression parser stack implementation
    Token *token;
    struct ExprStackNode *next;
} ExpressionStackNode;

typedef struct { // Stack for symtables
    unsigned long size;
    SymtableStackNode *top;
} SymtableStack;

typedef struct { // Stack for expression parsing
    unsigned long size;
    ExpressionStackNode *top;
} ExpressionStack;


// ----Symtable stack operations---- //

/**
 * @brief Symtable stack constructor
 * 
 * @return SymtableStack* Initialized stack instance
 */
SymtableStack *SymtableStackInit(void);

/**
 * @brief Returns the element at the top of the stack
 * 
 * @param stack Stack instance
 * @return Symtable* Symtable at the top of the stack, or NULL if stack is empty
 */
Symtable *SymtableStackTop(SymtableStack *stack);

/**
 * @brief Removes the symtable at the top of the stack, or does nothing if stack is empty
 * 
 * @param stack Stack instance
 */
void SymtableStackRemoveTop(SymtableStack *stack);

/**
 * @brief Removes an element at the top of the stack and returns it, or returns NULL if stack is empty
 * 
 * @param stack Stack instance
 * @return Symtable* The top element, or NULL if stack is empty
 */
Symtable *SymtableStackPop(SymtableStack *stack);

/**
 * @brief Pushes a new symtable onto the top of the stack
 * 
 * @param stack Stack instance
 * @param symtable Table to be put onto the top
 */
void SymtableStackPush(SymtableStack *stack, Symtable *symtable);

// Symtable stack destructor
void SymtableStackDestroy(SymtableStack *stack);

// To preserve the ADT type of the stack
bool SymtableStackIsEmpty(SymtableStack *stack);




// ----Operations for expression stack (basically the same as for the symtable stack, just different data types)---- //

/**
 * @brief Expression stack constructor
 * 
 * @return ExpressionStack* Initialized stack instance
 */
ExpressionStack *ExpressionStackInit(void);

/**
 * @brief Returns the element at the top of the stack
 * 
 * @param stack Stack instance
 * @return Token* Token at the top of the stack, or NULL if stack is empty
 */
Token *ExpressionStackTop(ExpressionStack *stack);

/**
 * @brief Removes the token at the top of the stack, or does nothing if stack is empty
 * 
 * @param stack Stack instance
 */
void ExpressionStackRemoveTop(ExpressionStack *stack);

/**
 * @brief Removes an element at the top of the stack and returns it, or returns NULL if stack is empty
 * 
 * @param stack Stack instance
 * @return Symtable* The top element, or NULL if stack is empty
 */
Token *ExpressionStackPop(ExpressionStack *stack);

/**
 * @brief Pushes a new token onto the top of the stack
 * 
 * @param stack Stack instance
 * @param token Token to be put onto the top
 */
void ExpressionStackPush(ExpressionStack *stack, Token *token);

// Expression stack destructor
void ExpressionStackDestroy(ExpressionStack *stack);

// To preserve the ADT type of the stack
bool ExpressionStackIsEmpty(ExpressionStack *stack);

// Expression stack destructor
void ExpressionStackDestroy(ExpressionStack *stack);

#endif