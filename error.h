/*Module containing error exit code macros and declarations for error.c (error handling functions)*/
#ifndef ERROR_H
#define ERROR_H

// success return code
#define SUCCESS 0

// Return codes in case of a error
#define ERROR_LEXICAL 1
#define ERROR_SYNTACTIC 2

/*Semantic error exit codes*/
#define ERROR_SEMANTIC_UNDEFINED 3          // undefined variable/function
#define ERROR_SEMANTIC_TYPECOUNT_FUNCTION 4 // wrong count/type of function parameters, or wrong type of func return value (and nepovolene zahozeni?)
#define ERROR_SEMANTIC_REDEFINED 5          // redefinition of a variable/function, re-assignment of a constant
#define ERROR_SEMANTIC_MISSING_EXPR 6       // i don't know what this means
#define ERROR_SEMANTIC_TYPE_COMPATIBILITY 7 // wrong types in arithmetic/string/relational expressions or wrong expression type (e.g. in assignments)
#define ERROR_SEMANTIC_TYPE_DERIVATION 8    // variable type not given and can't be derived from the used expression
#define ERROR_SEMANTIC_UNUSED_VARIABLE 9    // other than unused variable, i don't know what this means
#define ERROR_SEMANTIC_OTHER 10             // other semantic errors
/*End of semantic error codes*/

#define ERROR_INTERNAL 99 // internal compiler error, for example a failed malloc call... etc.

// Ends the compiler with the given exit code
void ErrorExit(int exit_code);

#endif