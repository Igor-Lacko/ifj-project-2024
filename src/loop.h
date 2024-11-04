#ifndef LOOP_H
#define LOOP_H

#include "types.h"


/**
 * @brief Checks if the loop is of a type while(id_with_null) |id_without_null| and returns to before the L_ROUND_BRACKET
 * 
 * @note Doesn't really check for syntax errors outside of the basic stuff such as brackets
 * 
 * @param parser Pointer to the parser structure
 * @return true The loop is a nullable type
 * @return false The loop is not a nullable type!
 */
bool IsLoopNullableType(Parser *parser);


/**
 * @brief Parses a nullable type loop (a loop of type while(nullable_expression) |non_nullable_id|)
 * 
 * @param parser Pointer to the parser structure.
 */
void ParseNullableWhileLoop(Parser *parser);

/**
 * @brief Parses a standard while loop (a loop of type while(boolean_expression){ run code})
 * 
 * @note The two functions will be pretty much equivalent except for some different stuff at the beginning and different codegen
 * @note Maybe merge them into one? So far i like this one better
 * 
 * @param parser Pointer to the parser structure
 */
void ParseWhileLoop(Parser *parser);

#endif