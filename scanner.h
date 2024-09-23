#ifndef SCANNER_H
#define SCANNER_H


//enum for scanner FSM states
typedef enum
{
    IDENTIIFIER,
    LITERAL,
    READY
} SCANNER_STATE;

//enum for token types
typedef enum
{
    /*Tokens that are treated as identifiers until they are loaded
    -After, if the identifier is in the keyword table the token type is changed to keyword*/
    IDENTIFIER_TOKEN,
    KEYWORD,

    //number tokens, floats contain in them a '.' character
    INTEGER_32,
    DOUBLE_64,

    //u8 and relevant tokens
    U8_TOKEN,
    ARRAY_TOKEN,

    //strings
    LITERAL_TOKEN,

    //operators
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

    //parentheses types used in expressions or fuction definitions/implementations
    L_ROUND_BRACKET,    // the '(' character
    R_ROUND_BRACKET,    // the ')' character
    L_CURLY_BRACKET,    // the '{' character
    R_CURLY_BRACKET,    // the '}' character

    //special symbols
    PREFIX_TOKEN,
    SEMICOLON,
    EOF_TOKEN,

} TOKEN_TYPE;

//keyword enum
typedef enum{
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
    NONE, //not a keyword, assigned to token if the token's type != KEYWORD
} KEYWORD_TYPE;

//enum for symbol types
typedef enum{
    NUMBER,
    CHARACTER,
    WHITESPACE,
    OTHER,
} CHAR_TYPE;

//token structure
typedef struct{
    TOKEN_TYPE token_type;
    KEYWORD_TYPE keyword_type; // KEYWORD_TYPE.NONE if token_type != KEYWORD
    void *attribute;           // null if the token has no attributre (e.g operators)
} Token;

//scanner structure to keep track of the state and line number
typedef struct{
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


//Token constructor
Token *InitToken();

//Token destructor
void DestroyToken(Token *token);


//Scanner constructor
Scanner *InitScanner();

//Scanner destructor
void DestroyScanner(Scanner *scanner);

//Returns the next character from stdin without moving forward (it returns the character back)
char NextChar();

/**
 * @brief Gets the next character's type
 * 
 * @param c : a character that isn't unique, e.g it belongs to a range of characters which can't be handled by a switch case (e.g [A-Z], etc.)
 * @return CHAR_TYPE Whitespace/Characyer/Number or Other (probably invalid token and error)
 */
CHAR_TYPE GetCharType(char c);

/**
 * @brief Handles a numeric literal token
 * 
 * @param token Token instance, it's attribute is filled with the number value
 * @param scanner For line length print in case of a error
 * @return TOKEN_TYPE The type of the number. If type can't be determined, returns WHOLE_NUMBER
 */
TOKEN_TYPE ConsumeNumber(Token *token, Scanner *scanner);


/**
 * @brief Handles a string literal
 * 
 * @param token loaded token with attribute: "string", type LITERAL and keyword NONE
 * @param scanner to keep track of the current line number
 */
void ConsumeLiteral(Token *token, Scanner *scanner);

/**
 * @brief Handles a identifier/keyword
 * 
 * @param token loaded token with attribute: "name" (so int i: attribute would be i), type KEYWORD/IDENTIFIER and if KEYWORD, a non empty keyword type
 * @param scanner for getting number of current line for error messages
 */
void ConsumeIdentifier(Token *token, Scanner *scanner);

/*continues until it reaches the end of the line or EOF, returns the first character after the comment ends
-Also increments the scanner's line number when it encounters a newline character*/
int ConsumeComment(Scanner *scanner);

//Similar to ConsumeComment(), skips whitespace, increments the line_number if a '\n' is encountered and returns the first non-whitespace character
int ConsumeWhitespace(Scanner *scanner);

#endif