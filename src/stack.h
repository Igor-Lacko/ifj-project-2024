// Contains stack structures for expression parsing and scope recognition

#ifndef STACK_H
#define STACK_H

#include "types.h" // Data types for the stack

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
 * @param parser Parser state
 */
void SymtableStackRemoveTop(Parser *parser);

/**
 * @brief Same as abov but used just for destroying symtable
 *
 * @param stack Stack instance
 */
void SymtableStackPop(SymtableStack *stack);

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

// Function for finding a variable in the stack
VariableSymbol *SymtableStackFindVariable(SymtableStack *stack, char *name);

void SymtableStackPrint(SymtableStack *stack);

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
ExpressionStackNode *ExpressionStackTop(ExpressionStack *stack);

/**
 * @brief Returns the terminal symbol closest to the stack top
 * 
 * @param stack Stack instance
 * @return ExpressionStackNode* Node containing the terminal symbol 
 */
ExpressionStackNode *TopmostTerminal(ExpressionStack *stack);

/**
 * @brief Returns the handle closest to the stack top
 * 
 * @param stack Stack instance
 * @param distance Filled with the distance from the stack top to the topmost handle
 * 
 * @return ExpressionStackNode* Node containing the handle
 */
ExpressionStackNode *TopmostHandle(ExpressionStack *stack, int *distance);

/**
 * @brief Pushes the handle after the topmost terminal symbol
 * 
 * @param stack Stack instance
 */
void PushHandleAfterTopmost(ExpressionStack *stack);

/**
 * @brief Initializes a new expression stack node
 * 
 * @param token Token contained in the node
 * @param type Node type (terminal/non-terminal/handle)
 * @param key The key type to index into the table
 * @return ExpressionStackNode* Initialized node
 */
ExpressionStackNode *ExpressionStackNodeInit(Token *token, STACK_NODE_TYPE type, PtableKey key);

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
ExpressionStackNode *ExpressionStackPop(ExpressionStack *stack);

/**
 * @brief Pushes a new token onto the top of the stack
 *
 * @param stack Stack instance
 * @param node Non-terminakl/Terminal to be put onto the top
 */
void ExpressionStackPush(ExpressionStack *stack, ExpressionStackNode *node);

// Expression stack destructor
void ExpressionStackDestroy(ExpressionStack *stack);

// To preserve the ADT type of the stack
bool ExpressionStackIsEmpty(ExpressionStack *stack);

// Expression stack destructor
void ExpressionStackDestroy(ExpressionStack *stack);

#endif