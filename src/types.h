/**
 * @file types.h
 * @brief Definitions for token types, symbols, and data structures used in parsing and code generation.
 *
 * This header includes enums for token types (e.g., identifiers, keywords, operators), symbol tables, data types (e.g., integers, floats, boolean),
 * and structures for managing symbols, expressions, and precedence analysis.
 * It also defines structures for handling dynamic arrays (vectors) and the various stack implementations used in expression parsing and evaluation.
 *
 * @authors:
 * - Igor Lacko [xlackoi00]
 * - Jakub Pogádl [xpogad00]
 * - Boris Semanco [xseman06]
 * - Rudolf Baumgartner [xbaumg01]
 */

#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>

// Symtable size
#define TABLE_COUNT 5009 // first prime over 5000

// Number of precedence table rows/columns
#define PTABLE_SIZE 14

// enum for token types
typedef enum
{
    /*Tokens that are treated as identifiers until they are loaded
    -After, if the identifier is in the keyword table the token type is changed to keyword*/
    IDENTIFIER_TOKEN,
    UNDERSCORE_TOKEN,
    KEYWORD,

    // number tokens, floats contain in them a '.' character
    INTEGER_32,
    DOUBLE_64,

    // u8 and relevant tokens
    U8_TOKEN,

    // strings
    LITERAL_TOKEN,

    // @import
    IMPORT_TOKEN,

    // operators
    ASSIGNMENT,
    MULTIPLICATION_OPERATOR,
    DIVISION_OPERATOR,
    ADDITION_OPERATOR,
    SUBSTRACTION_OPERATOR,
    EQUAL_OPERATOR,
    NOT_EQUAL_OPERATOR,
    LESS_THAN_OPERATOR,
    LARGER_THAN_OPERATOR,
    LESSER_EQUAL_OPERATOR,
    LARGER_EQUAL_OPERATOR,

    // parentheses types used in expressions or fuction definitions/implementations
    L_ROUND_BRACKET,    // the '(' character
    R_ROUND_BRACKET,    // the ')' character
    L_CURLY_BRACKET,    // the '{' character
    R_CURLY_BRACKET,    // the '}' character
    VERTICAL_BAR_TOKEN, // the '|' character

    // special symbols
    SEMICOLON,
    COMMA_TOKEN,
    DOT_TOKEN,
    COLON_TOKEN,
    EOF_TOKEN,

    // special token used in expression evaluation for type compatibility with the stack
    BOOLEAN_TOKEN

} TOKEN_TYPE;

// keyword enum
typedef enum
{
    CONST,
    ELSE,
    FN,
    IF,
    I32,
    F64,
    NULL_TYPE,
    PUB,
    RETURN,
    U8,
    VAR,
    VOID,
    WHILE,
    NONE, // not a keyword, assigned to token if the token's type != KEYWORD
} KEYWORD_TYPE;

// enum for symbol types
typedef enum
{
    NUMBER,
    CHARACTER,
    WHITESPACE,
    OTHER,
} CHAR_TYPE;

// token structure
typedef struct
{
    TOKEN_TYPE token_type;
    KEYWORD_TYPE keyword_type; // KEYWORD_TYPE.NONE if token_type != KEYWORD
    char *attribute;           // String representation of the token
    int line_number;           // Useful when ungetting tokens
} Token;

// Symbols and symtables
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
    TERM_TYPE, // For example ifj.write(term) --> can be a float, integer or even null
    NULL_DATA_TYPE
} DATA_TYPE;

// ********************HASH TABLE****************************** //

// structure of a variable symbol
typedef struct
{
    char *name;
    char *value; // for constants
    DATA_TYPE type;
    bool is_const;
    bool nullable;
    bool defined;
    bool was_used;
} VariableSymbol;

// String array to store variables for functions
typedef struct
{
    char **strings;
    int count;
    int capacity;
} StringArray;

// structure of a function symbol
typedef struct
{
    char *name;
    int num_of_parameters;
    VariableSymbol **parameters;
    DATA_TYPE return_type;
    bool has_return;
    StringArray variables;
} FunctionSymbol;

typedef struct
{
    SYMBOL_TYPE symbol_type;
    void *symbol;
    bool is_occupied; // True if the slot is occupied
} HashEntry;

typedef struct
{
    unsigned long capacity; // Maximum number of entries
    unsigned long size;     // Current number of occupied entries
    HashEntry *table;       // Array of hash entries
} Symtable;

/******************** VECTOR (DYNAMIC ARRAY) STRUCTURES ********************/

// "Dynamic string"
typedef struct
{
    int length;
    int max_length;
    char *value;
} Vector;

// Vector of tokens
typedef struct
{                         // Same as vector from vector.h, but with tokens
    Token **token_string; // string of input tokens
    int length;           // current
    int capacity;         // max
} TokenVector;

/******************** STRUCTURES FOR PRECEDENTIAL ANALYSIS ********************/

// Enumeration of grammar rules for reduction in expressions
typedef enum
{
    IDENTIFIER_RULE,     // E -> id
    BRACKET_RULE,        // E -> (E)
    MULTIPLICATION_RULE, // E -> E * E
    DIVISION_RULE,       // E -> E / E
    ADDITION_RULE,       // E -> E + E
    SUBSTRACTION_RULE,   // E -> E - E
    EQUAL_RULE,          // E -> E == E
    NOT_EQUAL_RULE,      // E -> E != E
    LESS_THAN_RULE,      // E -> E < E
    LARGER_THAN_RULE,    // E -> E > E
    LESSER_EQUAL_RULE,   // E -> E <= E
    LARGER_EQUAL_RULE,   // E -> E >= E
    NOT_FOUND_RULE       // Rule not found, error
} EXPRESSION_RULE;

typedef enum
{
    PTABLE_ID,
    PTABLE_MULTIPLICATION,
    PTABLE_DIVISION,
    PTABLE_ADDITION,
    PTABLE_SUBSTRACTION,
    PTABLE_EQUAL,
    PTABLE_NOT_EQUAL,
    PTABLE_LESS_THAN,
    PTABLE_LARGER_THAN,
    PTABLE_LESSER_EQUAL,
    PTABLE_LARGER_EQUAL,
    PTABLE_LEFT_BRACKET,
    PTABLE_RIGHT_BRACKET,
    PTABLE_DOLLAR,
    PTABLE_ERROR, // Invalid token in expression
    PTABLE_NOKEY  // When pushing a Non-terminal/Handle to the stack
} PtableKey;

typedef enum
{
    SHIFT,
    REDUCE,
    MATCH,
    ACCEPT,
    INVALID
} PtableValue;

typedef PtableValue PrecedenceTable[PTABLE_SIZE][PTABLE_SIZE];

/********************* STACK STRUCTURES ********************/

typedef struct SymStackNode
{ // Linked list for symtable stack implementation
    Symtable *table;
    struct SymStackNode *next;
} SymtableStackNode;

typedef struct
{ // Stack for symtables
    unsigned long size;
    SymtableStackNode *top;
} SymtableStack;

typedef enum
{               // Expression stack node types
    HANDLE,     // Handle symbol, represents start of a reduction
    TERMINAL,   // Terminal symbol, represented by the token inside the node
    NONTERMINAL // There don't need to bne multiple types for this, since we only have one non-terminal -> E
} STACK_NODE_TYPE;

typedef struct ExprStackNode
{ // Linked list for expression parser stack implementation
    Token *token;
    STACK_NODE_TYPE node_type;
    PtableKey key_type;
    struct ExprStackNode *next;
} ExpressionStackNode;

typedef struct
{ // Stack for expression parsing
    unsigned long size;
    ExpressionStackNode *top;
} ExpressionStack;

typedef struct EvalStackNode
{ // Linked list for evaluation stack implementation
    Token *token;
    struct EvalStackNode *next;
} EvaluationStackNode;

typedef struct
{ // Stack for expression evaluation
    unsigned long size;
    EvaluationStackNode *top;
} EvaluationStack;

/******************** ENUM FOR IFJCODE24 FRAMES ********************/
typedef enum
{
    GLOBAL_FRAME,
    LOCAL_FRAME,
    TEMPORARY_FRAME
} FRAME;

/******************** CORE PARSER STRUCTURE ********************/
typedef struct
{
    int nested_level;
    int line_number;
    bool has_main;
    bool parsing_functions;
    bool end_of_program;
    FunctionSymbol *current_function;
    Symtable *symtable;        // Current symtable (top of the stack), local variables
    Symtable *global_symtable; // Only for functions
    SymtableStack *symtable_stack;
} Parser;

#endif