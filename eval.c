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
static void add(value_t *value, node_t *left, node_t *right);
static void subtract(value_t *value, node_t *left, node_t *right);
static void multiply(value_t *value, node_t *left, node_t *right);
static void divide(value_t *value, node_t *left, node_t *right);
static void modulo(value_t *value, node_t *left, node_t *right);

// TODO TODO****: Paste new tarball files from canvas into the directory

// Lists valid types for each binop operator
// Can access the correct list by calling binopTypes[(Token) - TOK_PLUS]
static const struct {
    token_t binopTok;
    void (*func_ptr)(value_t *value, node_t *left, node_t *right);
    int numValid;
    node_type_t types[2];
} binopTypes[] = {
    {TOK_PLUS,   &add,  2,    {INT_TYPE, STRING_TYPE}},              // +
    {TOK_BMINUS, &subtract, 1,    {INT_TYPE}},                           // -
    {TOK_TIMES,  &multiply, 2,    {INT_TYPE, STRING_TYPE}},              // *
    {TOK_DIV,    &divide, 1,    {INT_TYPE}},                           // /
    {TOK_MOD,    &modulo, 1,    {INT_TYPE}},                           // %
    {TOK_AND,    NULL, 1,    {BOOL_TYPE}},                          // &
    {TOK_OR,     NULL, 1,    {BOOL_TYPE}},                          // |
    {TOK_LT,     NULL, 2,    {INT_TYPE, STRING_TYPE}},              // <
    {TOK_GT,     NULL, 2,    {INT_TYPE, STRING_TYPE}},              // >
    {TOK_EQ,     NULL, 2,    {INT_TYPE, STRING_TYPE}}               // ~
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

/* add() - Adds the values of the left and right nodes based on their type and 
 * stores the result in value
 */
static void add(value_t *value, node_t *left, node_t *right) {
    if(left->type != right->type) {
        handle_error(ERR_TYPE);
        return;
    }

    if(left->type == INT_TYPE) {
        value->ival = left->val.ival + right->val.ival;
        return;
    }
    if(left->type == STRING_TYPE) {
        value->sval = (char *) malloc(strlen(left->val.sval) + strlen(right->val.sval) + 1);
        if (! value->sval) {
            logging(LOG_FATAL, "failed to allocate string");
            return;
        }
                        
        strcpy(value->sval, left->val.sval);
        strcat(value->sval, right->val.sval);
        return;
    }

    handle_error(ERR_TYPE);
    return;
}

// subtract() - Subtracts the int value of right from left and stores it in value.
static void subtract(value_t *value, node_t *left, node_t *right) {
    if(left->type != INT_TYPE || right->type != INT_TYPE) {
        handle_error(ERR_TYPE);
        return;
    }

    value->ival = left->val.ival - right->val.ival;
    return;
}

// multiply() - TODO
static void multiply(value_t *value, node_t *left, node_t *right) {
    if(right->type != INT_TYPE) {
        handle_error(ERR_TYPE);
        return;
    }

    if(left->type == INT_TYPE) {
        value->ival = left->val.ival * right->val.ival;
        return;
    }

    if(left->type == STRING_TYPE) {
        value->sval = (char *) malloc(strlen(left->val.sval) + strlen(right->val.sval) + 1);
        if (! value->sval) {
            logging(LOG_FATAL, "failed to allocate string");
            return;
        }
                        
        strcpy(value->sval, left->val.sval);
        strcat(value->sval, right->val.sval);
        return;
    }

    handle_error(ERR_TYPE);
    return;
}

/* divide() - Divides the value of left by the value of right
 * and stores it in value. Causes an evaluation error if dividing
 * by 0.
 */
static void divide(value_t *value, node_t *left, node_t *right) {
    if(left->type != INT_TYPE || right->type != INT_TYPE) {
        handle_error(ERR_TYPE);
        return;
    }

    if(right->val.ival == 0) {
        handle_error(ERR_EVAL);
        return;
    }

    value->ival = left->val.ival / right->val.ival;
    return;
}

/* modulo() - Returns the modulo of the left value by the right value
 * and  stores it in vale. Causes an evaluation error if modulo by 0.
 */
static void modulo(value_t *value, node_t *left, node_t *right) {
    if(left->type != INT_TYPE || right->type != INT_TYPE) {
        handle_error(ERR_TYPE);
        return;
    }

    if(right->val.ival == 0) {
        handle_error(ERR_EVAL);
        return;
    }

    value->ival = left->val.ival % right->val.ival;
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

        // Handle binary operators
        if(is_binop(nptr->tok)) {
            for(int i = 0; i < 2; i++) {
                eval_node(nptr->children[i]);
            }

            if(terminate || ignore_input) return;
            
            // Call the appropriate function based on the token
            int index = nptr->tok - TOK_PLUS;
            (*binopTypes[index].func_ptr)(&nptr->val, nptr->children[0], nptr->children[1]);
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