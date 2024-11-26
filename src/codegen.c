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
    switch(frame)
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
    if(type == IDENTIFIER_TOKEN)
    {
        char *frame_string = GetFrameString(frame);
        fprintf(stdout, "PUSHS %s%s\n", frame_string, attribute);
        free(frame_string);
        return;
    }

    char *type_string = GetTypeStringToken(type);
    fprintf(stdout, "PUSHS %s", type_string);

    // White space handling for string literals
    if(type == LITERAL_TOKEN) WriteStringLiteral(attribute);
    else fprintf(stdout, "%s", attribute);

    fprintf(stdout, "\n");
    free(type_string);
}

void MOVE(const char *dst, const char *src, bool is_literal, FRAME dst_frame)
{
    char *frame_string = GetFrameString(dst_frame);
    fprintf(stdout, "MOVE %s%s ", frame_string, dst);
    if(is_literal) WriteStringLiteral(src);
    else fprintf(stdout, "%s\n", src);
}

void SETPARAM(int order, const char *value, TOKEN_TYPE type, FRAME frame)
{
    // Initial print of the target parameter variable
    fprintf(stdout, "MOVE TF@PARAM%d ", order);

    // Prefix is either GF@/LF@/TF@ or the type of the token (int@, float@0x, string@, bool@)
    char *prefix = type == IDENTIFIER_TOKEN ? GetFrameString(frame) : GetTypeStringToken(type);
    fprintf(stdout, "%s", prefix);

    // If the token is a string literal, call the WriteStringLiteral function to handle whitespaces accordingly
    if(type == LITERAL_TOKEN) WriteStringLiteral(value); // Why can't i use the ternary operator here :(((
    else fprintf(stdout, "%s", value);

    fprintf(stdout, "\n");

    free(prefix);
}

char *GetFrameString(FRAME frame)
{
    switch(frame)
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
    switch(type)
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
    switch(type)
    {
        case INT32_TYPE: case INT32_NULLABLE_TYPE:
            return strdup("int@");

        case DOUBLE64_TYPE: case DOUBLE64_NULLABLE_TYPE:
            return strdup("float@0x");

        case U8_ARRAY_TYPE: case U8_ARRAY_NULLABLE_TYPE:
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
    char *type;
    switch(var->type)
    {
        case U8_ARRAY_TYPE: case U8_ARRAY_NULLABLE_TYPE:
            type = strdup("string");
            break;

        case INT32_TYPE: case INT32_NULLABLE_TYPE:
            type = strdup("int");
            break;

        case DOUBLE64_TYPE: case DOUBLE64_NULLABLE_TYPE:
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


    switch(frame)
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
    if(token->token_type == LITERAL_TOKEN) WriteStringLiteral(token->attribute);
    else fprintf(stdout, "%s", token->attribute);

    // If the token is a float, print the exponent, other than that just a newline
    token->token_type == DOUBLE_64 ? fprintf(stdout, "p+0\n") : fprintf(stdout, "\n");

    free(prefix);
}

void INT2FLOAT(VariableSymbol *dst, Token *value, FRAME dst_frame, FRAME src_frame)
{
    char *src_prefix = value->token_type == IDENTIFIER_TOKEN ? GetFrameString(src_frame) : strdup("int@");
    char *dst_prefix = GetFrameString(dst_frame);

    fprintf(stdout, "INT2FLOAT %s%s %s%s\n", dst_prefix, dst->name, src_prefix, value->attribute);

    free(src_prefix);
    free(dst_prefix);
}

void FLOAT2INT(VariableSymbol *dst, Token *value, FRAME dst_frame, FRAME src_frame)
{
    char *src_prefix = value->token_type == IDENTIFIER_TOKEN ? GetFrameString(src_frame) : strdup("float@0x");
    char *dst_prefix = GetFrameString(dst_frame);

    fprintf(stdout, "FLOAT2INT %s%s %s%s\n", dst_prefix, dst->name, src_prefix, value->attribute);

    free(src_prefix);
    free(dst_prefix);
}

void STRLEN(VariableSymbol *var, Token *src, FRAME dst_frame, FRAME src_frame)
{
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
    // If this is being called, we assume that the needed type-checking has already been done so it won't be done here
    char *dst_frame_str = GetFrameString(dst_frame);
    (void) dst_frame_str;
    char *str1_prefix = str1->token_type == IDENTIFIER_TOKEN ? GetFrameString(str1_frame) : GetTypeStringToken(str1->token_type);
    char *str2_prefix = str2->token_type == IDENTIFIER_TOKEN ? GetFrameString(str2_frame) : GetTypeStringToken(str2->token_type);

    // Compare the strings with IFJcode24 instructions
    // B1 will store the strings s1 > s2, B2 will store s2 > s1, if neither of those is true, the strings are equal
    fprintf(stdout, "GT GF@$B1 %s", str1_prefix);
    if(str1->token_type == LITERAL_TOKEN) WriteStringLiteral(str1->attribute);
    else fprintf(stdout, "%s", str1->attribute);

    // second operand
    fprintf(stdout, " %s", str2_prefix);
    if(str2->token_type == LITERAL_TOKEN) WriteStringLiteral(str2->attribute);
    else fprintf(stdout, "%s", str2->attribute);

    fprintf(stdout, "\n");

    // Do the same for B2
    fprintf(stdout, "GT GF@$B2 %s", str2_prefix);
    if(str2->token_type == LITERAL_TOKEN) WriteStringLiteral(str2->attribute);
    else fprintf(stdout, "%s", str2->attribute);

    // second operand
    fprintf(stdout, " %s", str1_prefix);
    if(str1->token_type == LITERAL_TOKEN) WriteStringLiteral(str1->attribute);
    else fprintf(stdout, "%s", str1->attribute);

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
    MOVE(var->name, "int@-1", false, dst_frame);
    fprintf(stdout, "JUMP ENDSTRCMP%d\n", strcmp_count);

    // LABEL SECONDGREATER
    fprintf(stdout, "LABEL SECONDGREATER%d\n", strcmp_count);
    MOVE(var->name, "int@1", false, dst_frame);
    fprintf(stdout, "JUMP ENDSTRCMP%d\n", strcmp_count);

    // LABEL AREEQUAL
    fprintf(stdout, "LABEL AREEQUAL%d\n", strcmp_count);
    MOVE(var->name, "int@0", false, dst_frame);

    // LABEL ENDSTRCMP
    fprintf(stdout, "LABEL ENDSTRCMP%d\n", strcmp_count);

    // Increment the counter at the end of the function
    strcmp_count++;
}

void STRING(VariableSymbol *var, Token *src, FRAME dst_frame, FRAME src_frame)
{
    char *dst_prefix = GetFrameString(dst_frame);
    char *src_prefix = src->token_type == IDENTIFIER_TOKEN ? GetFrameString(src_frame) : strdup("string@");
    fprintf(stdout, "MOVE %s%s %s", dst_prefix, var->name, src_prefix);
    if(src->token_type == LITERAL_TOKEN)
    {
        WriteStringLiteral(src->attribute);
        fprintf(stdout, "\n");
    }
    else fprintf(stdout, "%s\n", src->attribute);
    free(dst_prefix);
    free(src_prefix);
}

void ORD(VariableSymbol *var, Token *string, Token *position, FRAME dst_frame, FRAME string_frame, FRAME position_frame)
{
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

    // Don't call STRLEN since it R0 is not represented by a token
    fprintf(stdout, "STRLEN GF@$R0 %s%s\n", string_prefix, string->attribute);

    /* Pseudocode for how that might look like
        if R0 == 0 jump RETURN0ORD
        B0 = position > R0
        if B0 jump RETURN0ORD
        STRI2INT ...
        jump ENDORD
    */

    // Initial conditionals
    JUMPIFEQ("ORDRETURN0", "GF@$R0", "int@0", ord_count)
    fprintf(stdout, "GT GF@$B0 %s%s GF@R0\n", position_prefix, string->attribute);
    JUMPIFEQ("ORDRETURN0", "GF@$B0", "bool@true", ord_count)

    // Call STRI2INT and skip the 0 assignment
    STRI2INT(var, string, position, dst_frame, string_frame, position_frame);
    fprintf(stdout, "JUMP ENDORD%d\n", ord_count);

    fprintf(stdout, "LABEL ORDRETURN0%d\n", ord_count);
    MOVE(var->name, "int@0", false, dst_frame);
    fprintf(stdout, "LABEL ENDORD%d\n", ord_count);

    // Deallocate the resources
    free(dst_prefix);
    free(string_prefix);
    free(position_prefix);

    // Increment the ord counter
    ord_count++;
}

void WriteStringLiteral(const char *str)
{
    for(int i = 0; i < (int)strlen(str); i++)
    {
        switch(str[i])
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
        case INT32_TYPE: case INT32_NULLABLE_TYPE:
            fprintf(stdout, "PUSHS GF@$R0\n");
            break;

        case DOUBLE64_TYPE: case DOUBLE64_NULLABLE_TYPE:
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