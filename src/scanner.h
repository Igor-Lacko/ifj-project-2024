#ifndef SCANNER_H
#define SCANNER_H

#include <stdbool.h>
#include "vector.h"

#define KEYWORD_COUNT 13

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
    char *attribute;           // null if the token has no attributre (e.g operators)
} Token;

/**
 * @brief Gets next token starting at pos (skipping whitespace)
 *
 * @param int keeps track of the line number
 * @return Token* Pointer to the initialized token structure
 */
Token *GetNextToken(int *line_number);

// Token constructor
Token *InitToken();

// Token destructor
void DestroyToken(Token *token);

// Returns the next character from stdin without moving forward (it returns the character back)
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
 * @param int For line length print in case of a error
 */
void ConsumeNumber(Token *token, int *line_number);

/**
 * @brief Helper function for ConsumeNumber, consumes the number's exponent
 *
 * @param vector Vector to add characters to
 * @param token To change the type if needed
 * @param has_floating_point a flag if the token is a valid DOUBLE_64 token even without the exponent
 * @return bool A flag to let ConsumeNumber know whether to end the loop or not
 */
bool ConsumeExponent(Vector *vector, Token *token, bool has_floating_point);

/**
 * @brief Handles a string literal
 *
 * @param token loaded token with attribute: "string", type LITERAL and keyword NONE
 * @param scanner to keep track of the current line number
 */
void ConsumeLiteral(Token *token, int *line_number);

/**
 * @brief Handles a identifier/keyword
 *
 * @param token loaded token with attribute: "name" (so int i: attribute would be i), type KEYWORD/IDENTIFIER and if KEYWORD, a non empty keyword type
 * @param line_number for getting number of current line for error messages
 */
void ConsumeIdentifier(Token *token, int *line_number);

/**
 * @brief called upon encountering a '@'
 * 
 * @param token token to add to
 * @param line_number for error messages
 */
void ConsumeImportToken(Token *token, int *line_number);

/*continues until it reaches the end of the line or EOF, returns the first character after the comment ends
-Also increments the scanner's line number when it encounters a newline character*/
int ConsumeComment(int *line_number);

// Similar to ConsumeComment(), skips whitespace, increments the line_number if a '\n' is encountered and returns the first non-whitespace character
int ConsumeWhitespace(int *line_number);

/*A bit of a special case -- []u8 is always written as such, so check if the next tokens match this pattern
-also u8 by ITSELF is a keyword, so a variable can't be u8 but that is checked by ConsumeIdentifier and then IsKeyword*/
void ConsumeU8Token(Token *token, int *line_number);

/**
 * @brief Allocates the space for the operator (as a string) and copies it to token -> attribute
 * 
 * @param token Token instance
 * @param operator Operator which is a string
 */
void AllocateAttribute(Token *token, const char *operator);

// checks if a identifier with a prefix at the start is valid or not (so if it's a keyword)
bool IsValidPrefix(char *identifier);

/**
 * @brief Checks if the token passed is a keyword
 *
 * @param attribute The given string
 * @return KEYWORD_TYPE NONE if not a keyword, otherwise the which one it is
 */
KEYWORD_TYPE IsKeyword(char *attribute);

// debug function
void PrintToken(Token *token);

#endif