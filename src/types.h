#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>

// Symtable size
#define TABLE_COUNT 1009 // first prime over 1000, todo: change this

// For embedded functions
#define EMBEDDED_FUNCTION_COUNT 13
#define MAXLENGTH_EMBEDDED_FUNCTION 10
#define MAXPARAM_EMBEDDED_FUNCTION 3

// Scanner structures
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
    KEYWORD_TYPE keyword_type;  // KEYWORD_TYPE.NONE if token_type != KEYWORD
    char *attribute;            // String representation of the token
    int line_number;            // Useful when ungetting tokens
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

// Vector (dynamic array) structures
typedef struct
{
    int length;
    int max_length;
    char *value;
} Vector;

// Vector of tokens
typedef struct { // Same as vector from vector.h, but with tokens
    Token **token_string; // string of input tokens
    int length; // current
    int capacity; // max
} TokenVector;

// Stack structures
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

// Structures for precedential analysis
typedef struct { // simple precedence table struct
    TOKEN_TYPE PRIORITY_HIGHEST[2]; // * and /
    TOKEN_TYPE PRIORITY_MIDDLE[2]; // + and -
    TOKEN_TYPE PRIORITY_LOWEST[6]; // ==, !=, >, <, >=, <=
} PrecedenceTable;

typedef enum { // helper enum for operator priority (probably only used once)
    HIGHEST = 3,
    MIDDLE = 2,
    LOWEST = 1
} OPERATOR_PRIORITY;

// Enum for IFJCode24 frames
typedef enum
{
    GLOBAL_FRAME,
    LOCAL_FRAME,
    TEMPORARY_FRAME
} FRAME;

// Core parser structure
typedef struct
{
    int nested_level;
    int line_number;
    bool has_main;
    bool parsing_functions;
    bool end_of_program;
    FunctionSymbol *current_function;
    Symtable *symtable; // Current symtable (top of the stack), local variables
    Symtable *global_symtable; // Only for functions
    SymtableStack *symtable_stack;
} Parser;

// Extern variables to keep track of labels for easier jumping
extern int if_label_count;
extern int while_label_count;

// Contains the names of all embedded function names
extern const char embedded_names[EMBEDDED_FUNCTION_COUNT][MAXLENGTH_EMBEDDED_FUNCTION];

// Contains the return values of all embedded functions
extern DATA_TYPE embedded_return_types[EMBEDDED_FUNCTION_COUNT];

// Contains the parameters for all embedded functions
extern DATA_TYPE embedded_parameters[EMBEDDED_FUNCTION_COUNT][MAXPARAM_EMBEDDED_FUNCTION];

// Contains the entire stream stored in a dynamic array of tokens
extern TokenVector *stream;

// Index to access the stream
extern int stream_index;

#endif