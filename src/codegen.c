/**
 * @file codegen.c
 * @brief Code generation functions for IFJCode24 intermediate code.
 *
 * This file contains functions for generating IFJCode24 code from the parsed expressions, variable definitions,
 * control structures (like if and while), and function calls. It also includes functions for managing registers,
 * variables, and frames in the intermediate code.
 *
 * Authors:
 * - Igor Lacko [xlackoi00]
 * - Rudolf Baumgartner [xbaumg01]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shared.h"
#include "codegen.h"
#include "stack.h"
#include "error.h"
#include "scanner.h"
#include "vector.h"
#include "expression_parser.h"

void InitRegisters()
{
    // Result registers
    fprintf(stdout, "DEFVAR GF@$R0\n");
    fprintf(stdout, "DEFVAR GF@$F0\n");
    fprintf(stdout, "DEFVAR GF@$B0\n");
    fprintf(stdout, "DEFVAR GF@$S0\n");

    // Operand registers
    fprintf(stdout, "DEFVAR GF@$R1\n");
    fprintf(stdout, "DEFVAR GF@$R2\n");
    fprintf(stdout, "DEFVAR GF@$F1\n");
    fprintf(stdout, "DEFVAR GF@$F2\n");
    fprintf(stdout, "DEFVAR GF@$B1\n");
    fprintf(stdout, "DEFVAR GF@$B2\n");
    fprintf(stdout, "DEFVAR GF@$S1\n");
    fprintf(stdout, "DEFVAR GF@$S2\n");
}

void DefineVariable(const char *name, FRAME frame)
{
    switch (frame)
    {
    case GLOBAL_FRAME:
        fprintf(stdout, "DEFVAR GF@%s\n", name);
        break;

    case LOCAL_FRAME:
        fprintf(stdout, "DEFVAR LF@%s\n", name);
        break;

    case TEMPORARY_FRAME:
        fprintf(stdout, "DEFVAR TF@%s\n", name);
        break;
    }
}

void IfLabel(int count)
{
    fprintf(stdout, "LABEL $if%d\n", count);
}

void ElseLabel(int count)
{
    fprintf(stdout, "LABEL $else%d\n", count);
}

void EndIfLabel(int count)
{
    fprintf(stdout, "LABEL $endif%d\n", count);
}

void WhileLabel(int count)
{
    fprintf(stdout, "LABEL $while%d\n", count);
}

void EndWhileLabel(int count)
{
    fprintf(stdout, "LABEL $endwhile%d\n", count);
}

void PUSHS(const char *attribute, TOKEN_TYPE type, FRAME frame)
{
    // If the token is an identifier, push it from the correct frame
    if (type == IDENTIFIER_TOKEN)
    {
        char *frame_string = GetFrameString(frame);
        fprintf(stdout, "PUSHS %s%s\n", frame_string, attribute);
        free(frame_string);
        return;
    }

    // null case
    else if (type == KEYWORD)
    {
        fprintf(stdout, "PUSHS nil@nil\n");
        return;
    }

    char *type_string = GetTypeStringToken(type);
    fprintf(stdout, "PUSHS %s", type_string);

    // White space handling for string literals
    if (type == LITERAL_TOKEN)
        WriteStringLiteral(attribute);
    else if (type == INTEGER_32)
        fprintf(stdout, "%s", attribute);
    else
    {
        double val = strtod(attribute, NULL);
        fprintf(stdout, "%a", val);
    }

    fprintf(stdout, "\n");
    free(type_string);
}

void MOVE(Token *dst, Token *src, FRAME dst_frame)
{
    char *frame_string = GetFrameString(dst_frame);
    fprintf(stdout, "MOVE %s%s ", frame_string, dst->attribute);
    if (src->token_type == LITERAL_TOKEN)
        WriteStringLiteral(src->attribute);
    else if (src->token_type == DOUBLE_64)
        fprintf(stdout, "%a", strtod(src->attribute, NULL));
    else
        fprintf(stdout, "%s", src->attribute);

    fprintf(stdout, "\n");
}

void SETPARAM(int order, const char *value, TOKEN_TYPE type, FRAME frame)
{
    // Initial print of the target parameter variable
    fprintf(stdout, "MOVE TF@PARAM%d ", order);

    // Prefix is either GF@/LF@/TF@ or the type of the token (int@, float@0x, string@, bool@)
    char *prefix = type == IDENTIFIER_TOKEN ? GetFrameString(frame) : GetTypeStringToken(type);
    fprintf(stdout, "%s", prefix);

    // If the token is a string literal, call the WriteStringLiteral function to handle whitespaces accordingly
    if (type == LITERAL_TOKEN)
        WriteStringLiteral(value); // Why can't i use the ternary operator here :(((
    else if (type == DOUBLE_64)
        fprintf(stdout, "%a", strtod(value, NULL));
    else
        fprintf(stdout, "%s", value);

    fprintf(stdout, "\n");

    free(prefix);
}

char *GetFrameString(FRAME frame)
{
    switch (frame)
    {
    case GLOBAL_FRAME:
        return strdup("GF@");

    case LOCAL_FRAME:
        return strdup("LF@");

    case TEMPORARY_FRAME:
        return strdup("TF@");
    }

    return NULL; // Shut up GCC please
}

char *GetTypeStringToken(TOKEN_TYPE type)
{
    switch (type)
    {
    case INTEGER_32:
        return strdup("int@");

    case DOUBLE_64:
        return strdup("float@");

    case LITERAL_TOKEN:
        return strdup("string@");

    case BOOLEAN_TOKEN:
        return strdup("bool@");

    default:
        return NULL;
    }
}

char *GetTypeStringSymbol(DATA_TYPE type)
{
    switch (type)
    {
    case INT32_TYPE:
    case INT32_NULLABLE_TYPE:
        return strdup("int@");

    case DOUBLE64_TYPE:
    case DOUBLE64_NULLABLE_TYPE:
        return strdup("float@0x");

    case U8_ARRAY_TYPE:
    case U8_ARRAY_NULLABLE_TYPE:
        return strdup("string@");

    case BOOLEAN:
        return strdup("bool@");

    default:
        return NULL;
    }
}

// In case the variable has term_type, change it after
void READ(VariableSymbol *var, FRAME frame, DATA_TYPE read_type)
{
    if (var == NULL)
        return;
    char *type;
    switch (var->type)
    {
    case U8_ARRAY_TYPE:
    case U8_ARRAY_NULLABLE_TYPE:
        type = strdup("string");
        break;

    case INT32_TYPE:
    case INT32_NULLABLE_TYPE:
        type = strdup("int");
        break;

    case DOUBLE64_TYPE:
    case DOUBLE64_NULLABLE_TYPE:
        type = strdup("float");
        break;

    case BOOLEAN:
        type = strdup("bool");
        break;

    case VOID_TYPE:
        type = GetTypeStringSymbol(read_type);
        break;

    default:
        ErrorExit(ERROR_INTERNAL, "Calling read on a variable with wrong type. Fix your code!!!");
    }

    switch (frame)
    {
    case GLOBAL_FRAME:
        fprintf(stdout, "READ GF@%s %s\n", var->name, type);
        break;

    case LOCAL_FRAME:
        fprintf(stdout, "READ LF@%s %s\n", var->name, type);
        break;

    case TEMPORARY_FRAME:
        fprintf(stdout, "READ TF@%s %s\n", var->name, type);
        break;
    }

    free(type);
}

void WRITEINSTRUCTION(Token *token, FRAME frame)
{
    // Get the prefix and print it
    char *prefix = token->token_type == IDENTIFIER_TOKEN ? GetFrameString(frame) : GetTypeStringToken(token->token_type);
    fprintf(stdout, "WRITE %s", prefix);

    // Write the token attribute depenting on the type
    if (token->token_type == LITERAL_TOKEN)
        WriteStringLiteral(token->attribute);
    else if (token->token_type == DOUBLE_64)
        fprintf(stdout, "%a", strtod(token->attribute, NULL));
    else
        fprintf(stdout, "%s", token->attribute);
    fprintf(stdout, "\n");

    free(prefix);
}

void INT2FLOAT(VariableSymbol *dst, Token *value, FRAME dst_frame, FRAME src_frame)
{
    if (dst == NULL)
        return;
    char *src_prefix = value->token_type == IDENTIFIER_TOKEN ? GetFrameString(src_frame) : strdup("int@");
    char *dst_prefix = GetFrameString(dst_frame);

    fprintf(stdout, "INT2FLOAT %s%s %s%s\n", dst_prefix, dst->name, src_prefix, value->attribute);

    free(src_prefix);
    free(dst_prefix);
}

void FLOAT2INT(VariableSymbol *dst, Token *value, FRAME dst_frame, FRAME src_frame)
{
    if (dst == NULL)
        return;
    char *src_prefix = value->token_type == IDENTIFIER_TOKEN ? GetFrameString(src_frame) : strdup("float@0x");
    char *dst_prefix = GetFrameString(dst_frame);

    if (value->token_type == IDENTIFIER_TOKEN)
        fprintf(stdout, "FLOAT2INT %s%s %s%s\n", dst_prefix, dst->name, src_prefix, value->attribute);
    else
        fprintf(stdout, "FLOAT2INT %s%s %s%a\n", dst_prefix, dst->name, src_prefix, strtod(value->attribute, NULL));

    free(src_prefix);
    free(dst_prefix);
}

void STRLEN(VariableSymbol *var, Token *src, FRAME dst_frame, FRAME src_frame)
{
    if (var == NULL)
        return;
    // Get the prefixes
    char *dst_prefix = GetFrameString(dst_frame);
    char *src_prefix = src->token_type == IDENTIFIER_TOKEN ? GetFrameString(src_frame) : strdup("string@");
    fprintf(stdout, "STRLEN %s%s %s%s\n", dst_prefix, var->name, src_prefix, src->attribute);

    // Free the prefixes
    free(dst_prefix);
    free(src_prefix);
}

void CONCAT(VariableSymbol *dst, Token *prefix, Token *postfix, FRAME dst_frame, FRAME prefix_frame, FRAME postfix_frame)
{
    if (dst == NULL)
        return;
    // Get the prefixes (maybe choose different variable names)
    char *dst_prefix = GetFrameString(dst_frame);
    char *prefix_prefix = prefix->token_type == IDENTIFIER_TOKEN ? GetFrameString(prefix_frame) : strdup("string@");
    char *postfix_prefix = postfix->token_type == IDENTIFIER_TOKEN ? GetFrameString(postfix_frame) : strdup("string@");

    // Print the instruction
    fprintf(stdout, "CONCAT %s%s %s%s %s%s\n", dst_prefix, dst->name, prefix_prefix, prefix->attribute, postfix_prefix, postfix->attribute);

    // Free the prefixes
    free(dst_prefix);
    free(prefix_prefix);
    free(postfix_prefix);
}

void STRI2INT(VariableSymbol *var, Token *src, Token *position, FRAME dst_frame, FRAME src_frame, FRAME position_frame)
{
    if (var == NULL)
        return;
    // Get the prefixes first
    char *dst_prefix = GetFrameString(dst_frame);
    char *src_prefix = src->token_type == IDENTIFIER_TOKEN ? GetFrameString(src_frame) : strdup("string@");
    char *position_prefix = position->token_type == IDENTIFIER_TOKEN ? GetFrameString(position_frame) : strdup("int@");

    // We assume that the type-checking has already been done, so error 58 won't occur
    fprintf(stdout, "STRI2INT %s%s %s%s %s%s\n", dst_prefix, var->name, src_prefix, src->attribute, position_prefix, position->attribute);

    // Deallocate the prefixes
    free(dst_prefix);
    free(src_prefix);
    free(position_prefix);
}

void INT2CHAR(VariableSymbol *dst, Token *value, FRAME dst_frame, FRAME src_frame)
{
    if (dst == NULL)
        return;
    // Get prefixes
    char *dst_prefix = GetFrameString(dst_frame);
    char *src_prefix = value->token_type == IDENTIFIER_TOKEN ? GetFrameString(src_frame) : strdup("int@");
    fprintf(stdout, "INT2CHAR %s%s %s%s\n", dst_prefix, dst->name, src_prefix, value->attribute);

    // Free the prefixes
    free(dst_prefix);
    free(src_prefix);
}

void STRCMP(VariableSymbol *var, Token *str1, Token *str2, FRAME dst_frame, FRAME str1_frame, FRAME str2_frame)
{
    if (var == NULL)
        return;
    // If this is being called, we assume that the needed type-checking has already been done so it won't be done here
    char *dst_frame_str = GetFrameString(dst_frame);
    (void)dst_frame_str;
    char *str1_prefix = str1->token_type == IDENTIFIER_TOKEN ? GetFrameString(str1_frame) : GetTypeStringToken(str1->token_type);
    char *str2_prefix = str2->token_type == IDENTIFIER_TOKEN ? GetFrameString(str2_frame) : GetTypeStringToken(str2->token_type);

    // Compare the strings with IFJcode24 instructions
    // B1 will store the strings s1 > s2, B2 will store s2 > s1, if neither of those is true, the strings are equal
    fprintf(stdout, "GT GF@$B1 %s", str1_prefix);
    if (str1->token_type == LITERAL_TOKEN)
        WriteStringLiteral(str1->attribute);
    else
        fprintf(stdout, "%s", str1->attribute);

    // second operand
    fprintf(stdout, " %s", str2_prefix);
    if (str2->token_type == LITERAL_TOKEN)
        WriteStringLiteral(str2->attribute);
    else
        fprintf(stdout, "%s", str2->attribute);

    fprintf(stdout, "\n");

    // Do the same for B2
    fprintf(stdout, "GT GF@$B2 %s", str2_prefix);
    if (str2->token_type == LITERAL_TOKEN)
        WriteStringLiteral(str2->attribute);
    else
        fprintf(stdout, "%s", str2->attribute);

    // second operand
    fprintf(stdout, " %s", str1_prefix);
    if (str1->token_type == LITERAL_TOKEN)
        WriteStringLiteral(str1->attribute);
    else
        fprintf(stdout, "%s", str1->attribute);

    fprintf(stdout, "\n");

    // Jump to the corresponding labels for each situations
    /*
    B1 = str1 > str2
    B2 = str2 > str1
    */

    JUMPIFEQ("FIRSTGREATER", "GF@$B1", "bool@true", strcmp_count)
    JUMPIFEQ("SECONDGREATER", "GF@$B2", "bool@true", strcmp_count)
    fprintf(stdout, "JUMP AREEQUAL%d\n", strcmp_count);

    // LABEL FIRSTGREATER
    fprintf(stdout, "LABEL FIRSTGREATER%d\n", strcmp_count);
    fprintf(stdout, "MOVE %s%s int@1\n", dst_frame_str, var->name);
    fprintf(stdout, "JUMP ENDSTRCMP%d\n", strcmp_count);

    // LABEL SECONDGREATER
    fprintf(stdout, "LABEL SECONDGREATER%d\n", strcmp_count);
    fprintf(stdout, "MOVE %s%s int@-1\n", dst_frame_str, var->name);
    fprintf(stdout, "JUMP ENDSTRCMP%d\n", strcmp_count);

    // LABEL AREEQUAL
    fprintf(stdout, "LABEL AREEQUAL%d\n", strcmp_count);
    fprintf(stdout, "MOVE %s%s int@0\n", dst_frame_str, var->name);

    // LABEL ENDSTRCMP
    fprintf(stdout, "LABEL ENDSTRCMP%d\n", strcmp_count);

    // Increment the counter at the end of the function
    strcmp_count++;
}

void STRING(VariableSymbol *var, Token *src, FRAME dst_frame, FRAME src_frame)
{
    if (var == NULL)
        return;
    char *dst_prefix = GetFrameString(dst_frame);
    char *src_prefix = src->token_type == IDENTIFIER_TOKEN ? GetFrameString(src_frame) : strdup("string@");
    fprintf(stdout, "MOVE %s%s %s", dst_prefix, var->name, src_prefix);
    if (src->token_type == LITERAL_TOKEN)
    {
        WriteStringLiteral(src->attribute);
        fprintf(stdout, "\n");
    }
    else
        fprintf(stdout, "%s\n", src->attribute);
    free(dst_prefix);
    free(src_prefix);
}

void ORD(VariableSymbol *var, Token *string, Token *position, FRAME dst_frame, FRAME string_frame, FRAME position_frame)
{
    if (var == NULL)
        return;
    // Get the prefixes first
    char *dst_prefix = GetFrameString(dst_frame);
    char *string_prefix = string->token_type == IDENTIFIER_TOKEN ? GetFrameString(string_frame) : strdup("string@");
    char *position_prefix = position->token_type == IDENTIFIER_TOKEN ? GetFrameString(position_frame) : strdup("int@");

    // We assume that the type-checking has already been done
    /*
        - We will use the STRLEN instruction to get the length of the string and store it in R0
        - Then we will either insert 0 into dst, or we will use STRI2INT to get the ASCII value of the character at the given position
        - STRLEN <dst> <src>
        - STRI2INT <dst> <str> <position>
    */

    // Don't call STRLEN() since R0 is not represented by a token
    fprintf(stdout, "STRLEN GF@$R0 %s%s\n", string_prefix, string->attribute);

    /* Pseudocode for how that might look like
        if R0 == 0 jump RETURN0ORD
        B0 = position > (R0 - 1) || (position < 0)
        if B0 jump RETURN0ORD
        STRI2INT ...
        jump ENDORD
    */

    // Initial conditionals
    JUMPIFEQ("ORDRETURN0", "GF@$R0", "int@0", ord_count) // If the string is empty, return 0

    // Check if the position isn't < 0
    fprintf(stdout, "LT GF@$B2 %s%s int@0\n", position_prefix, position->attribute); // B2 = position < 0

    // Now check if position > (R0 - 1)
    fprintf(stdout, "SUB GF@$R0 GF@$R0 int@1\n");
    fprintf(stdout, "GT GF@$B1 %s%s GF@$R0\n", position_prefix, position->attribute);

    // OR those two
    fprintf(stdout, "OR GF@$B0 GF@$B1 GF@$B2\n"); // B0 = B1 || B2
    JUMPIFEQ("ORDRETURN0", "GF@$B0", "bool@true", ord_count)

    // Call STRI2INT and skip the 0 assignment
    STRI2INT(var, string, position, dst_frame, string_frame, position_frame);
    fprintf(stdout, "JUMP ENDORD%d\n", ord_count);

    fprintf(stdout, "LABEL ORDRETURN0%d\n", ord_count);
    fprintf(stdout, "MOVE %s%s int@0\n", dst_prefix, var->name);
    fprintf(stdout, "LABEL ENDORD%d\n", ord_count);

    // Deallocate the resources
    free(dst_prefix);
    free(string_prefix);
    free(position_prefix);

    // Increment the ord counter
    ord_count++;
}

void SUBSTRING(VariableSymbol *var, Token *str, Token *beginning_index, Token *end_index, FRAME dst_frame, FRAME src_frame, FRAME beginning_frame, FRAME end_frame)
{
    if (var == NULL)
        return;
    // Get all the prefixes first
    char *dst_prefix = GetFrameString(dst_frame);
    char *str_prefix = GetFrameString(src_frame);
    char *beginning_prefix = beginning_index->token_type == IDENTIFIER_TOKEN ? GetFrameString(beginning_frame) : strdup("int@");
    char *end_prefix = end_index->token_type == IDENTIFIER_TOKEN ? GetFrameString(end_frame) : strdup("int@");

    /* First, check the edge cases. The function ifj.substring(str, beginning_index, end_index) returns null when:
        1. beginning_index < 0
        2. end_index < 0
        3. beginning_index > end_index
        4. beginning_index >= ifj.length(str)
        5. end_index > ifj.length(str)
    */

    /* Edge cases handling pseudocode
        MOVE R0, beginning_index                    R0 = beginning
        MOVE R1, end_index                          R1 = end
        STRLEN R2, str                              R2 = length
        LT B1 R0 int@0                              B1 = beginning < 0
        LT B2 R1 int@0                              B2 = end < 0
        OR B0 B1 B2                                 B0 = B1 || B2, conditions 1 and 2 marked off
        GT B1 R0 R1                                 B1 = beginning > end
        OR B0 B0 B1                                 B0 = B0 || B1, condition 3 marked off
        GT B1 R0 R2                                 B1 = R0 > R2
        EQ B2 R0 R2                                 B2 = R0 == R2
        OR B1 B1 B2                                 B1 = B1 || B2
        OR B0 B0 B1                                 B0 = B0 || B1, condition 4 marked off
        GT B1 R1 R2                                 B1 = R1 > R2
        OR B0 B0 B1                                 B0 = B0 || B1, condition 5 marked off
        JUMPIFEQ SUBSTRINGRETURNNULL B0 bool@true   if(B0) return NULL
    */

    fprintf(stdout, "MOVE GF@$R0 %s%s\n", beginning_prefix, beginning_index->attribute); // R0 = beginning
    fprintf(stdout, "MOVE GF@$R1 %s%s\n", end_prefix, end_index->attribute);             // R1 = end
    fprintf(stdout, "STRLEN GF@$R2 %s%s\n", str_prefix, str->attribute);                 // R2 = length
    fprintf(stdout, "LT GF@$B1 GF@$R0 int@0\n");                                         // B1 = beginning < 0
    fprintf(stdout, "LT GF@$B2 GF@$R1 int@0\n");                                         // B2 = end < 0
    fprintf(stdout, "OR GF@$B0 GF@$B1 GF@$B2\n");                                        // B0 = B1 || B2, conditions 1 and 2 marked off
    fprintf(stdout, "GT GF@$B1 GF@$R0 GF@$R1\n");                                        // B1 = beginning > end
    fprintf(stdout, "OR GF@$B0 GF@$B0 GF@$B1\n");                                        // B0 = B0 || B1, condition 3 marked off
    fprintf(stdout, "GT GF@$B1 GF@$R0 GF@$R2\n");                                        // B1 = beginning > length
    fprintf(stdout, "EQ GF@$B2 GF@$R0 GF@$R2\n");                                        // B2 = beginning == length
    fprintf(stdout, "OR GF@$B1 GF@$B1 GF@$B2\n");                                        // B1 = B1 || B2
    fprintf(stdout, "OR GF@$B0 GF@$B0 GF@$B1\n");                                        // B0 = B0 || B1, condition 4 marked off
    fprintf(stdout, "GT GF@$B1 GF@$R1 GF@$R2\n");                                        // B1 = end > length
    fprintf(stdout, "OR GF@$B0 GF@$B0 GF@$B1\n");                                        // B0 = B0 || B1, condition 5 marked off
    JUMPIFEQ("SUBSTRINGRETURNNULL", "GF@$B0", "bool@true", substring_count);             // if(B0) return NULL

    /* Also check another edge case where i == j, in that case we return an empty string, pseudocodeL
        EQ B0 R0 R1
        JUMPIFEQ SUBSTRINGRETURNEMPTY B0 bool@true
    */
    fprintf(stdout, "EQ GF@$B0 GF@$R0 GF@$R1\n");
    JUMPIFEQ("SUBSTRINGRETURNEMPTY", "GF@$B0", "bool@true", substring_count);

    /* Substring getting pseudocode
        MOVE B2, true                           B2 = true (flag of the first character)
        LABEL SUBSTRINGWHILE
        LT B0 R0 R1                             B0 = beginning < end
        JUMPIFEQ SUBSTRINGWHILEEND B0 bool@false     while(beginning < end)
        GETCHAR str S1                          S1 = str[beginning]
        JUMPIFEQ SUBSTRINGFIRSTCHAR, B2, true   if(B2) goto FIRSTCHAR
        CONCAT S0 S1 S0                         else{ S0 = S0 + S1
        JUMP SUBSTRINGNOTFIRSTCHAR              goto NOTFIRSTCHAR }
        LABEL SUBSTRINGFIRSTCHAR
        MOVE S0 S1                              S0 = S1
        MOVE B2, false                          B2 = false
        LABEL SUBSTRINGNOTFIRSTCHAR
        ADD R0 R0 int@1                         beginning++
        JUMP SUBSTRINGWHILE
        LABEL SUBSTRINGWHILEEND
        MOVE var S0                             var = S0
        JUMP SUBSTRINGEND
    */

    fprintf(stdout, "MOVE GF@$B2 bool@true\n");                                  // B2 = true (flag of the first character)
    fprintf(stdout, "LABEL SUBSTRINGWHILE%d\n", substring_count);                // LABEL SUBSTRINGWHILE
    fprintf(stdout, "LT GF@$B0 GF@$R0 GF@$R1\n");                                // B0 = beginning < end
    JUMPIFEQ("SUBSTRINGWHILEEND", "GF@$B0", "bool@false", substring_count);      // while(beginning < end)
    fprintf(stdout, "GETCHAR GF@$S1 %s%s GF@$R0\n", str_prefix, str->attribute); // S1 = str[beginning]
    JUMPIFEQ("SUBSTRINGFIRSTCHAR", "GF@$B2", "bool@true", substring_count);      // if(B2) goto FIRSTCHAR
    fprintf(stdout, "CONCAT GF@$S0 GF@$S0 GF@$S1\n");                            // else{ S0 = S0 + S1
    fprintf(stdout, "JUMP SUBSTRINGNOTFIRSTCHAR%d\n", substring_count);          // goto NOTFIRSTCHAR
    fprintf(stdout, "LABEL SUBSTRINGFIRSTCHAR%d\n", substring_count);            // LABEL SUBSTRINGFIRSTCHAR
    fprintf(stdout, "MOVE GF@$S0 GF@$S1\n");                                     // S0 = S1
    fprintf(stdout, "MOVE GF@$B2 bool@false\n");                                 // B2 = false
    fprintf(stdout, "LABEL SUBSTRINGNOTFIRSTCHAR%d\n", substring_count);         // LABEL SUBSTRINGNOTFIRSTCHAR
    fprintf(stdout, "ADD GF@$R0 GF@$R0 int@1\n");                                // beginning++
    fprintf(stdout, "JUMP SUBSTRINGWHILE%d\n", substring_count);                 // goto SUBSTRINGWHILE
    fprintf(stdout, "LABEL SUBSTRINGWHILEEND%d\n", substring_count);             // LABEL SUBSTRINGWHILEEND
    fprintf(stdout, "MOVE %s%s GF@$S0\n", dst_prefix, var->name);                // var = S0
    JUMP_WITH_ORDER("SUBSTRINGEND", substring_count);                            // goto SUBSTRINGEND

    // Case where we return NULL
    fprintf(stdout, "LABEL SUBSTRINGRETURNNULL%d\n", substring_count); // LABEL SUBSTRINGRETURNNULL
    fprintf(stdout, "MOVE %s%s nil@nil\n", dst_prefix, var->name);     // var = nil
    JUMP_WITH_ORDER("SUBSTRINGEND", substring_count);                  // goto SUBSTRINGEND

    // Case where we return an empty string
    fprintf(stdout, "LABEL SUBSTRINGRETURNEMPTY%d\n", substring_count); // LABEL SUBSTRINGRETURNEMPTY
    fprintf(stdout, "MOVE %s%s string@\n", dst_prefix, var->name);      // var = ""

    // End of the function
    fprintf(stdout, "LABEL SUBSTRINGEND%d\n", substring_count); // LABEL SUBSTRINGEND

    // Increment the substring counter
    substring_count++;
}

void WriteStringLiteral(const char *str)
{
    for (int i = 0; i < (int)strlen(str); i++)
    {
        switch (str[i])
        {
        case '\n':
            fprintf(stdout, "\\010");
            break;

        case '\t':
            fprintf(stdout, "\\009");
            break;

        case '\v':
            fprintf(stdout, "\\011");
            break;

        case '\b':
            fprintf(stdout, "\\008");
            break;

        case '\r':
            fprintf(stdout, "\\013");
            break;

        case '\f':
            fprintf(stdout, "\\012");
            break;

        case '\\':
            fprintf(stdout, "\\092");
            break;

        case '\'':
            fprintf(stdout, "\\039");
            break;

        case '\"':
            fprintf(stdout, "\\034");
            break;

        case ' ':
            fprintf(stdout, "\\032");
            break;

        default:
            fprintf(stdout, "%c", str[i]);
            break;
        }
    }
} // hello rudko was here

void PopToRegister(DATA_TYPE type)
{
    switch (type)
    {
    case INT32_TYPE:
        fprintf(stdout, "POPS GF@$R0\n");
        break;

    case DOUBLE64_TYPE:
        fprintf(stdout, "POPS GF@$F0\n");
        break;

    case BOOLEAN:
        fprintf(stdout, "POPS GF@$B0\n");
        break;

    // This will never happen
    default:
        break;
    }
}

void PushRetValue(DATA_TYPE type)
{
    switch (type)
    {
    case INT32_TYPE:
    case INT32_NULLABLE_TYPE:
        fprintf(stdout, "PUSHS GF@$R0\n");
        break;

    case DOUBLE64_TYPE:
    case DOUBLE64_NULLABLE_TYPE:
        fprintf(stdout, "PUSHS GF@$F0\n");
        break;

    // Should never happen?
    case BOOLEAN:
        fprintf(stdout, "PUSHS GF@$B0\n");
        break;

    // This will never happen
    default:
        break;
    }
}