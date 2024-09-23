#ifndef SCANNER_H
#define SCANNER_H


// enum for scanner FSM states
typedef enum
{
    WHITESPACE,
    IDENTIIFIER,
    LITERAL,
    READY
} SCANNER_STATE;

// enum for token types
typedef enum
{
    IDENTIIFIER_TOKEN,
    KEYWORD,
    ASSIGNMENT,
    TYPE_IDENTIFIER,    // for example i32, or ?[]u8
    LITERAL_TOKEN,      // strings
    SEMICOLON,          // signals end of expression

    //number tokens
    I32_TOKEN,
    F64_TOKEN,
    UINT8_TOKEN,

    //escape sequences
    NEWLINE,
    CARRIAGE_RETURN,    // the '\r' literal
    TAB,
    BACKSLASH,          // two backslashes('\\') --> literal backslash
    DOUBLE_QUOTE,       // the '\"' literal --> literal double quote

    // operators
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

    EOF_TOKEN,

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

// token structure
typedef struct
{
    TOKEN_TYPE token_type;
    KEYWORD_TYPE keyword_type; // KEYWORD_TYPE.NONE if token_type != KEYWORD
    void *attribute;           // null if the token has no attributre (e.g operators)
} Token;

// scanner structure to keep track of the state and line number
typedef struct
{
    SCANNER_STATE state;
    int line_number;
} Scanner;

/**
 * @brief Gets next token starting at pos (skipping whitespace)
 *
 * @param scanner scanner instance
 * @return Token* Pointer to the initialized token structure
 */
Token *GetNextToken(Scanner *scanner);

/**
 * @brief Gets the length of the next token
 *
 * @param token points to the first character of the next token
 * @return int: length of the token including characters such as '/0' or newlines in case of multi-line strings
 */
int GetTokenLength(char *token);


//Token constructor
Token *InitToken();

//Token destructor
void DestroyToken();

/**
 * @brief Called after encountering a '=' character in the READY state
 * 
 * @param token the token instance to set type for, depending on the number of '=' characters
 */
void ConsumeEqualOperator(Token *token);

//Scanner constructor
Scanner *InitScanner();

//Scanner destructor
void DestroyScanner(Scanner *scanner);

#endif