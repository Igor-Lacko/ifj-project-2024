/**
 * @file stack.h
 * @brief Stack-based data structures for expression parsing and scope recognition.
 *
 * This file defines stack operations used for expression parsing, managing
 * symbol tables for scope recognition, and evaluating expressions during
 * parsing. It includes the implementation of the symbol table stack,
 * expression stack, and evaluation stack, which are used to manage scopes,
 * handle operator precedence, and evaluate intermediate expressions. The
 * operations allow for pushing, popping, and querying elements from these
 * stacks, as well as clearing, printing, and destroying them.
 *
 * Authors:
 * - Igor Lacko [xlackoi00]
 * - Boris Semanco [xseman06]
 * - Jakub Pogádl [xpogad00]
 */

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
 * @brief Same as above but used just for destroying symtable
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
 * @brief Returns the distance to the handle closest to the stack top
 *
 * @param stack Stack instance
 *
 * @return int Distance from the top of the stack to the handle
 */
int TopmostHandleDistance(ExpressionStack *stack);

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

// Removes the tokens that are not in the postfix string from the stack
void ExpressionStackClear(ExpressionStack *stack, TokenVector *postfix);

// To preserve the ADT type of the stack
bool ExpressionStackIsEmpty(ExpressionStack *stack);

// Debug
void ExpressionStackPrint(ExpressionStack *stack);

// ----Evaluation stack operations---- //

/**
 * @brief Evaluation stack constructor
 *
 * @return EvaluationStack* Initialized stack instance
 */
EvaluationStack *EvaluationStackInit(void);

/**
 * @brief Returns the element at the top of the stack
 *
 * @param stack Stack instance
 * @return Token* Token at the top of the stack, or NULL if stack is empty
 */
Token *EvaluationStackTop(EvaluationStack *stack);

/**
 * @brief Removes the token at the top of the stack, or does nothing if stack is empty
 *
 * @param stack Stack instance
 */
void EvaluationStackRemoveTop(EvaluationStack *stack);

/**
 * @brief Pushes a new token onto the top of the stack
 *
 * @param stack Stack instance
 * @param token Token to be put onto the top
 */
void EvaluationStackPush(EvaluationStack *stack, Token *token);

/**
 * @brief Pops the top element from the stack
 *
 * @param stack Stack instance
 */
Token *EvaluationStackPop(EvaluationStack *stack);

// Evaluation stack destructor
void EvaluationStackDestroy(EvaluationStack *stack);

// To preserve the ADT type of the stack
bool EvaluationStackIsEmpty(EvaluationStack *stack);

#endif