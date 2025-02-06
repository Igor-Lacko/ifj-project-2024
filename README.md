# IFJ24 project - compiler for the IFJ24 language

This project was made for the Formal Languages and Compilers course on
the Faculty of Information Technology, Brno University of Technology. It 
contains the implementation of a compiler of the IFJ24 language, which is functionally a subset of Zig.

## Modules overview:

### 1. Scanner
Contains the implementation of the DFA which functions as the lexical analyzer/tokenizer of the compiler

### 2. Core Parser
Contains the implementation of the recursive descent algorithm for syntax analysis. Also contains most
semantic checks and the actual code generation.

### 3. Codegen
Contains the implementation of code generating functions and macros, including code generation for the IFJ24
embedded functions

### 4. Expression parser
Contains the implementation of the precedential syntactic analysis algorithm for expression parsing and conversion to postfix. 
Also contains stack code generation and semantic checks of the converted expression.

### 5. Embedded functions
Contains identifying embedded functions, implementation of syntactic analysis along with semantic checks, and
if needed, calling codegen for the embedded functions.

### 6. Conditionals
Contains syntactic/semantic analysis for if-else statements, calling codegen for them and nesting in and out of these blocks.

### 7. Loop
Contains syntactic/semantic analysis for while loops, calling codegen for them and nesting in and out of these blocks.

### 8. Function parser
Contains code for the first "go-through" of the compiler, saving functions to the symbol table and making a global token vector,
basically preprocesses the input file for compilation.

### 9. Symtable
Contains the implementation of the symbol table. The symbol table is implemented as a hash table with open adressing
with a linear step of size 1.

### 10. Stack
Contains help/util stack structures. Contains a stack of symtables (for nesting the program), a stack for parsing
expressions and a stack for generating stack code for these expressions.

### 11. Vector
Contains help/util vector/dynamic array structures. Contains a structure to load a unknown-length input
in lexical analysis and a structure to save tokens into which is used in the second go-through of the compiler.

### 12. Error
Contains functions/macros for error messages and appropriate return codes.

## Evaluation
Implementation: 13/18
Documentation: 3/5

## Authors
Jakub Pogádl (xpogad00) <br>
Igor Lacko (xlackoi00) <br>
Boris Semanco (xseman06) <br>
Rudolf Baumgartner (xbaumg01)