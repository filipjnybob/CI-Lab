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
static void and(value_t *value, node_t *left, node_t *right);
static void or(value_t *value, node_t *left, node_t *right);
static void lessThan(value_t *value, node_t *left, node_t *right);
static void greaterThan(value_t *value, node_t *left, node_t *right);
static void equals(value_t *value, node_t *left, node_t *right);

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
    {TOK_AND,    &and, 1,    {BOOL_TYPE}},                          // &
    {TOK_OR,     &or, 1,    {BOOL_TYPE}},                          // |
    {TOK_LT,     &lessThan, 2,    {INT_TYPE, STRING_TYPE}},              // <
    {TOK_GT,     &greaterThan, 2,    {INT_TYPE, STRING_TYPE}},              // >
    {TOK_EQ,     &equals, 2,    {INT_TYPE, STRING_TYPE}}               // ~
};

void resolve_variable(node_t *nptr) {
    entry_t* var = get(nptr->val.sval);

    if(var == NULL) {
        handle_error(ERR_UNDEFINED);
        return;
    }

    nptr->type = var->type;

    if(nptr->type == STRING_TYPE) {
        nptr->val.sval = (char *) malloc(strlen(var->val.sval) + 1);
        if (! nptr->val.sval) {
            logging(LOG_FATAL, "failed to allocate string");
            return;
        }
        strcpy(nptr->val.sval, var->val.sval);
    } else {
        nptr->val.ival = var->val.ival;
    }

    return;
}

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

        // Handle unary operator
        if(is_unop(nptr->tok)) {
            if(nptr->children[0] == NULL) {
                handle_error(ERR_SYNTAX);
                return;
            }
            if(nptr->tok == TOK_UMINUS) {
                if(nptr->children[0]->type == INT_TYPE) {
                    nptr->type = INT_TYPE;
                } else if(nptr->children[0]->type == STRING_TYPE) {
                    nptr->type = STRING_TYPE;
                } else {
                    handle_error(ERR_TYPE);
                    return;
                }
                return;
            }

            if(nptr->tok == TOK_NOT) {
                if(nptr->children[0]->type != BOOL_TYPE) {
                    handle_error(ERR_TYPE);
                    return;
                }

                nptr->type = BOOL_TYPE;
                return;
            }
        }

        // Handle binary operator
        if(is_binop(nptr->tok)) {
            if(nptr->children[0] == NULL || nptr->children[1] == NULL) {
                handle_error(ERR_SYNTAX);
                return;
            }
            
            // Special case
            // String times operator allows different types
            if(nptr->tok == TOK_TIMES) {
                // 2nd argument must be an int
                if(nptr->children[1]->type != INT_TYPE) {
                    handle_error(ERR_TYPE);
                    return;
                }

                if(nptr->children[0]->type == INT_TYPE) {
                    nptr->type = INT_TYPE;
                    return;
                } else if(nptr->children[0]->type == STRING_TYPE) {
                    nptr->type = STRING_TYPE;
                    return;
                }

                handle_error(ERR_TYPE);
                return;
            } else {
                // All other tokens
                if(nptr->children[0]->type != nptr->children[1]->type) {
                    handle_error(ERR_TYPE);
                }

                node_type_t childrenType = nptr->children[0]->type;
                int index = nptr->tok - TOK_PLUS;
            
                for(int i = 0; i < binopTypes[index].numValid; i++) {
                    if(childrenType == binopTypes[index].types[i]) {
                        if(nptr->tok == TOK_GT || nptr->tok == TOK_LT || nptr->tok == TOK_EQ) {
                            nptr->type = BOOL_TYPE;
                        } else {
                            nptr->type = childrenType;
                        }
                        
                        return;
                    }
                }
            }

            

            

            handle_error(ERR_TYPE);
        }

        // Handle ternary operator
        if(nptr->tok == TOK_QUESTION) {
            if(nptr->children[0]->type != BOOL_TYPE) {
                handle_error(ERR_TYPE);
                return;
            }

            if(nptr->children[1]->type == nptr->children[2]->type) {
                nptr->type = nptr->children[1]->type;
                return;
            }

            handle_error(ERR_TYPE);
            return;

        }
        
    }

    if(nptr->type == ID_TYPE) {
        resolve_variable(nptr);
        return;
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
            handle_error(ERR_SYNTAX);
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

    // Handle ints
    if(left->type == INT_TYPE) {
        value->ival = left->val.ival * right->val.ival;
        return;
    }

    // Handle Strings
    if(left->type == STRING_TYPE) {
        value->sval = (char *) malloc((strlen(left->val.sval) * right->val.ival) + 1);
        if (! value->sval) {
            logging(LOG_FATAL, "failed to allocate string");
            return;
        }

        strcpy(value->sval, "");

        for(int i = 0; i < right->val.ival; i++) {
            strcat(value->sval, left->val.sval);
        }
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

/* modulo() - Calculates the modulo of the left value by the right value
 * and  stores it in value. Causes an evaluation error if modulo by 0.
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

/* and() - Calculates the AND of the left and right bool values and
 * stores it in value
 */
static void and(value_t *value, node_t *left, node_t *right) {
    if(left->type != BOOL_TYPE || right->type != BOOL_TYPE) {
        handle_error(ERR_TYPE);
        return;
    }

    value->bval = left->val.bval && right->val.bval;
    return;
}

/* or() - Calculates the OR of the left and right bool values and
 * stores it in value
 */
static void or(value_t *value, node_t *left, node_t *right) {
    if(left->type != BOOL_TYPE || right->type != BOOL_TYPE) {
        handle_error(ERR_TYPE);
        return;
    }

    value->bval = left->val.bval || right->val.bval;
    return;
}

/* lessThan() - Compares either two ints or two strings lexigraphically
 * and stores true in value if left is less than right.
 */
static void lessThan(value_t *value, node_t *left, node_t *right) {

    strcmp("abd", "abc");

    if(left->type != right->type) {
        handle_error(ERR_TYPE);
        return;
    }

    if(left->type == INT_TYPE) {
        value->bval = left->val.ival < right->val.ival;
        return;
    }
    if(left->type == STRING_TYPE) {
        value->bval = strcmp(left->val.sval, right->val.sval) < 0;;
        return;
    }

    handle_error(ERR_TYPE);
    return;
}

/* greaterThan() - Compares either two ints or two strings lexigraphically
 * and stores true in value if left is greater than right.
 */
static void greaterThan(value_t *value, node_t *left, node_t *right) {
    if(left->type != right->type) {
        handle_error(ERR_TYPE);
        return;
    }

    if(left->type == INT_TYPE) {
        value->bval = left->val.ival > right->val.ival;
        return;
    }
    if(left->type == STRING_TYPE) {
        value->bval = strcmp(left->val.sval, right->val.sval) > 0;
        return;
    }

    handle_error(ERR_TYPE);
    return;
}

/* equals() - Compares either two ints or two strings lexigraphically
 * and stores true in value if left is equal to right.
 */
static void equals(value_t *value, node_t *left, node_t *right) {
    if(left->type != right->type) {
        handle_error(ERR_TYPE);
        return;
    }

    if(left->type == INT_TYPE) {
        value->bval = left->val.ival == right->val.ival;
        return;
    }
    if(left->type == STRING_TYPE) {
        value->bval = strcmp(left->val.sval, right->val.sval) == 0;
        return;
    }

    handle_error(ERR_TYPE);
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
        // Handle ternary operators
        if(nptr->tok == TOK_QUESTION) {
            if(nptr->children[0]->type != BOOL_TYPE){
                handle_error(ERR_TYPE);
                return;
            }

            if(nptr->type == INT_TYPE) {
                nptr->val.ival = nptr->children[0]->val.bval ? nptr->children[1]->val.ival : nptr->children[2]->val.ival;
            } else if(nptr->type == BOOL_TYPE) {
                nptr->val.bval = nptr->children[0]->val.bval ? nptr->children[1]->val.bval : nptr->children[2]->val.bval;
            } else if(nptr->type == STRING_TYPE) {
                char* string = nptr->children[0]->val.bval ? nptr->children[1]->val.sval : nptr->children[2]->val.sval;

                nptr->val.sval = (char *) malloc(strlen(string) + 1);
                if (! nptr->val.sval) {
                    logging(LOG_FATAL, "failed to allocate string");
                    return;
                }
                strcpy(nptr->val.sval, string);
            }

            return;
        }

        for(int i = 0; i < 2; i++) {
            eval_node(nptr->children[i]);
        }
        
        // Handle unary operators
        if(is_unop(nptr->tok)) {
            // _
            if(nptr->tok == TOK_UMINUS) {
                if(nptr->children[0]->type == INT_TYPE) {
                    nptr->val.ival = nptr->children[0]->val.ival * -1;
                } else if(nptr->children[0]->type == STRING_TYPE) {
                    nptr->val.sval = strrev(nptr->children[0]->val.sval);
                } else {
                    handle_error(ERR_TYPE);
                    return;
                }
                return;
            }

            // !
            else if(nptr->tok == TOK_NOT) {
                if(nptr->children[0]->type != BOOL_TYPE) {
                    handle_error(ERR_TYPE);
                    return;
                }

                nptr->val.bval = !nptr->children[0]->val.bval;
                return;
            }
        }

        // Handle binary operators
        if(is_binop(nptr->tok)) {
            if(terminate || ignore_input) return;
            
            // Call the appropriate function based on the token
            int index = nptr->tok - TOK_PLUS;
            (*binopTypes[index].func_ptr)(&nptr->val, nptr->children[0], nptr->children[1]);
            return;
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
            handle_error(ERR_SYNTAX);
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
    int length = strlen(str);

    char* result = malloc(length + 1);

    for(int i = length - 1; i >= 0; i--) {
        result[length - 1 - i] = str[i];
    }

    result[length] = '\0';

    return result;
}