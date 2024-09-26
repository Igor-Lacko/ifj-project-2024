#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "symtable.h"
#include "error.h"



SymtableListNode *InitNode(SYMBOL_TYPE symbol_type, void *symbol){
    SymtableListNode *node; if((node = malloc(sizeof(SymtableListNode))) == NULL){
        ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
    }

    node -> symbol_type = symbol_type; node -> symbol = symbol;
}


void DestroyNode(SymtableListNode *node){
    //just in case, check if it wasn't called on an empty node
    if(node == NULL) return;

    if(node -> symbol != NULL){
        //I LOVE THE TERNARY OPERATOR
        node -> symbol_type == FUNCTION_SYMBOL ? DestroyFunctionSymbol(node -> symbol) : DestroyVariableSymbol(node -> symbol); 
    }

    node -> next = NULL;

    free(node);
}


void AppendNode(SymtableListNode *list, SymtableListNode *node){
    SymtableListNode *current_node = list;

    //jump through the nodes until the next one is empty
    while(current_node -> next != NULL){
        current_node = current_node -> next;
    }

    current_node -> next = node;
}


void PopNode(SymtableListNode *list){
    SymtableListNode *current_node = list;

    //we need to look 2 nodes forward, since we need to free current_node -> next and set it to NULL
    while(current_node -> next -> next != NULL){
        current_node = current_node -> next;
    }

    DestroyNode(current_node -> next);
    current_node -> next = NULL;
}


//this is probably a very inefficient function, might redo to a doubly linked list later
void RemoveNode(SymtableListNode *list, SymtableListNode *node){
    SymtableListNode *current_node = list;

    //we want to find the node before the one we want to remove and link it with node -> next
    while(current_node -> next != node){
        current_node = current_node -> next;
    }

    current_node -> next = node -> next;
    DestroyNode(node);
}


//recursion woohoo
void DestroyList(SymtableListNode *list){
    if(list -> next != NULL) DestroyList(list -> next);
    DestroyNode(list);
}


Symtable *InitSymtable(size_t size){
    Symtable *symtable; if((symtable = malloc(sizeof(Symtable))) == NULL){
        ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
    }

    //allocate size linked lists and set each one to NULL
    if((symtable -> table = calloc(size, sizeof(SymtableListNode*))) == NULL){
        ErrorExit(ERROR_INTERNAL, "Memory allocation failed");
    }
}