#ifndef CONDITIONALS_H
#define CONDITIONALS_H

#include "types.h"


/**
 * @brief Checks if the if statement is a nullable type, so if(nullable_expression) |non_nullable_id|{...} else{...}
 * 
 * @param parser Parser instance
 * 
 * @note This function basically works in the same way as the corresponding one in the loop module
 * 
 * @return true If the if is a nullable type
 * @return false If it's not
 */
bool IsIfNullableType(Parser *parser);


/**
 * @brief Parses a nullable if statement (if(nullable_expression) |non_nullable_id|{...} else{...}) and generates the code
 * 
 * @param parser Parser instance.
 */
void ParseNullableIfStatement(Parser *parser);

/**
 * @brief Parses a normal if statement (if(boolean_expression){...} else{...}) and generates the code
 * 
 * @param parser Parser instance
 */
void ParseIfStatement(Parser *parser);

#endif