/**************************************************************************
 * C S 429 EEL interpreter
 * 
 * eval.c - This file contains the skeleton of functions to be implemented by
 * you. When completed, it will contain the code used to evaluate an expression
 * based on its AST.
 * 
 * Copyright (c) 2021. S. Chatterjee, X. Shen, T. Byrd. All rights reserved.
 * May not be used, modified, or copied without permission.
 **************************************************************************/ 

#include "ci.h"

extern bool is_binop(token_t);
extern bool is_unop(token_t);
char *strrev(char *str);

// Lists valid types for each binop operator
// Can access the correct list by calling binopTypes[(Token) - TOK_PLUS]
static const struct {
    token_t binopTok;
    int numValid;
    node_type_t types[2];
} binopTypes[] = {
    {TOK_PLUS,   2,    {INT_TYPE, STRING_TYPE}},              // +
    {TOK_BMINUS, 1,    {INT_TYPE}},                           // -
    {TOK_TIMES,  2,    {INT_TYPE, STRING_TYPE}},              // *
    {TOK_DIV,    1,    {INT_TYPE}},                           // /
    {TOK_MOD,    1,    {INT_TYPE}},                           // %
    {TOK_AND,    1,    {BOOL_TYPE}},                          // &
    {TOK_OR,     1,    {BOOL_TYPE}},                          // |
    {TOK_LT,     2,    {INT_TYPE, STRING_TYPE}},              // <
    {TOK_GT,     2,    {INT_TYPE, STRING_TYPE}},              // >
    {TOK_EQ,     2,    {INT_TYPE, STRING_TYPE}}               // ~
};

/* infer_type() - set the type of a non-root node based on the types of children
 * Parameter: A node pointer, possibly NULL.
 * Return value: None.
 * Side effect: The type field of the node is updated.
 * (STUDENT TODO)
 */

static void infer_type(node_t *nptr) {
    if(nptr == NULL) return;
    if (terminate || ignore_input) return;

    if(nptr->node_type == NT_INTERNAL) {
        for (int i = 0; i < 3; ++i) {
            infer_type(nptr->children[i]);
        }

        // Handle binary operator
        if(is_binop(nptr->tok)) {
            if(nptr->children[0]->type != nptr->children[1]->type) {
                handle_error(ERR_TYPE);
            }

            node_type_t childrenType = nptr->children[0]->type;
            int index = nptr->tok - TOK_PLUS;
            
            for(int i = 0; i < binopTypes[index].numValid; i++) {
                if(childrenType == binopTypes[index].types[i]) {
                    nptr->type = childrenType;
                    return;
                }
            }

            handle_error(ERR_TYPE);
        }
    }

    return;
}

/* infer_root() - set the type of the root node based on the types of children
 * Parameter: A pointer to a root node, possibly NULL.
 * Return value: None.
 * Side effect: The type field of the node is updated. 
 */

static void infer_root(node_t *nptr) {
    if (nptr == NULL) return;
    // check running status
    if (terminate || ignore_input) return;

    // check for assignment
    if (nptr->type == ID_TYPE) {
        infer_type(nptr->children[1]);
    } else {
        for (int i = 0; i < 3; ++i) {
            infer_type(nptr->children[i]);
        }
        if (nptr->children[0] == NULL) {
            logging(LOG_ERROR, "failed to find child node");
            return;
        }
        nptr->type = nptr->children[0]->type;
    }
    return;
}

/* eval_node() - set the value of a non-root node based on the values of children
 * Parameter: A node pointer, possibly NULL.
 * Return value: None.
 * Side effect: The val field of the node is updated.
 * (STUDENT TODO) 
 */

static void eval_node(node_t *nptr) {
    if(nptr == NULL) return;
    if(terminate || ignore_input) return;

    if(nptr->node_type == NT_INTERNAL) {
        if(is_binop(nptr->tok)) {
            for(int i = 0; i < 2; i++) {
                eval_node(nptr->children[i]);
            }

            if(terminate || ignore_input) return;

            switch(nptr->tok) {
                case TOK_PLUS:
                    if(nptr->children[0]->type == INT_TYPE) {
                        nptr->val.ival = nptr->children[0]->val.ival + nptr->children[1]->val.ival;
                    }
                    if(nptr->children[0]->type == STRING_TYPE) {
                        nptr->val.sval = (char *) malloc(strlen(nptr->children[0]->val.sval) + strlen(nptr->children[1]->val.sval) + 1);
                        if (! nptr->val.sval) {
                            logging(LOG_FATAL, "failed to allocate string");
                            return;
                        }
                        
                        strcpy(nptr->val.sval, nptr->children[0]->val.sval);
                        strcat(nptr->val.sval, nptr->children[1]->val.sval);
                    }
                    break;
            }

        }
    }

    return;
}

/* eval_root() - set the value of the root node based on the values of children 
 * Parameter: A pointer to a root node, possibly NULL.
 * Return value: None.
 * Side effect: The val dield of the node is updated. 
 */

void eval_root(node_t *nptr) {
    if (nptr == NULL) return;
    // check running status
    if (terminate || ignore_input) return;

    // check for assignment
    if (nptr->type == ID_TYPE) {
        eval_node(nptr->children[1]);
        if (terminate || ignore_input) return;
        
        if (nptr->children[0] == NULL) {
            logging(LOG_ERROR, "failed to find child node");
            return;
        }
        put(nptr->children[0]->val.sval, nptr->children[1]);
        return;
    }

    for (int i = 0; i < 2; ++i) {
        eval_node(nptr->children[i]);
    }
    if (terminate || ignore_input) return;
    
    if (nptr->type == STRING_TYPE) {
        (nptr->val).sval = (char *) malloc(strlen(nptr->children[0]->val.sval) + 1);
        if (! nptr->val.sval) {
            logging(LOG_FATAL, "failed to allocate string");
            return;
        }
        strcpy(nptr->val.sval, nptr->children[0]->val.sval);
    } else {
        nptr->val.ival = nptr->children[0]->val.ival;
    }
    return;
}

/* infer_and_eval() - wrapper for calling infer() and eval() 
 * Parameter: A pointer to a root node.
 * Return value: none.
 * Side effect: The type and val fields of the node are updated. 
 */

void infer_and_eval(node_t *nptr) {
    infer_root(nptr);
    eval_root(nptr);
    return;
}

/* strrev() - helper function to reverse a given string 
 * Parameter: The string to reverse.
 * Return value: The reversed string. The input string is not modified.
 * (STUDENT TODO)
 */

char *strrev(char *str) {
    return NULL;
}