/**
 * @file p3-analysis.c
 * @brief Compiler phase 3: static analysis
 * Team Lima: Alice Robertson and Alexander Bain
 * 
 */
#include "p3-analysis.h"

/**
 * @brief State/data for static analysis visitor
 */
typedef struct AnalysisData
{
    /**
     * @brief List of errors detected
     */
    ErrorList *errors;

    /******************* State Information that we added *******************/

    /**
     * @brief current function that we are in
     */
    char *current_func;

    /**
     * @brief true if we are in a while loop, false otherwise
     */
    bool in_while;

    /**
     * @brief the current table to do look ups in
     */
    SymbolTable *curr_table;

    /**
     * @brief the program table
     */
    SymbolTable *program_table;

} AnalysisData;

/**
 * @brief Allocate memory for analysis data
 * 
 * @returns Pointer to allocated structure
 */
AnalysisData *AnalysisData_new()
{
    AnalysisData *data = (AnalysisData *)calloc(1, sizeof(AnalysisData));
    CHECK_MALLOC_PTR(data);
    data->errors = ErrorList_new();
    data->current_func = NULL;
    data->in_while = false;
    data->curr_table = NULL;
    data->program_table = NULL;
    return data;
}

/**
 * @brief Deallocate memory for analysis data
 * 
 * @param data Pointer to the structure to be deallocated
 */
void AnalysisData_free(AnalysisData *data)
{
    /* free everything in data that is allocated on the heap except the error
     * list; it needs to be returned after the analysis is complete */
    free(data->current_func);

    /* free "data" itself */
    free(data);
}

/**
 * @brief Macro for more convenient access to the data inside a @ref AnalysisVisitor
 * data structure
 */
#define DATA ((AnalysisData *)visitor->data)

/**
 * @brief Macro for more convenient access to the error list inside a
 * @ref AnalysisVisitor data structure
 */
#define ERROR_LIST (((AnalysisData *)visitor->data)->errors)

/**
 * @brief Macro for more convenient access to the current function inside a
 * @ref AnalysisVisitor data structure
 */
#define CURR_FUNC (((AnalysisData *)visitor->data)->current_func)

/**
 * @brief Macro for more convenient access to the bool for in_while inside a
 * @ref AnalysisVisitor data structure
 */
#define IN_WHILE (((AnalysisData *)visitor->data)->in_while)

/**
 * @brief Macro for more convenient access to the bool for in_while inside a
 * @ref AnalysisVisitor data structure
 */
#define CURR_TABLE (((AnalysisData *)visitor->data)->curr_table)

/**
 * @brief Macro for more convenient access to the bool for in_while inside a
 * @ref AnalysisVisitor data structure
 */
#define PROGRAM_TABLE (((AnalysisData *)visitor->data)->program_table)

/**
 * @brief Wrapper for @ref lookup_symbol that reports an error if the symbol isn't found
 * 
 * @param visitor Visitor with the error list for reporting
 * @param node AST node to begin the search at
 * @param name Name of symbol to find
 * @returns The @ref Symbol if found, otherwise @c NULL
 */
Symbol *lookup_symbol_with_reporting(NodeVisitor *visitor, ASTNode *node, const char *name)
{
    Symbol *symbol = lookup_symbol(node, name);
    if (symbol == NULL)
    {
        ErrorList_printf(ERROR_LIST, "Symbol '%s' undefined on line %d", name, node->source_line);
    }
    return symbol;
}

/**
 * @brief Macro for shorter storing of the inferred @c type attribute
 */
#define SET_INFERRED_TYPE(T) ASTNode_set_printable_attribute(node, "type", (void *)(T), \
                                                             type_attr_print, dummy_free)

/**
 * @brief Macro for shorter retrieval of the inferred @c type attribute
 */
#define GET_INFERRED_TYPE(N) (DecafType) ASTNode_get_attribute(N, "type")

/****************************** HELPER METHODS ******************************/
/**
 * @brief use the current symbol table defined in the analysis struct to check for duplicate variables
 */
void check_for_duplicates(NodeVisitor *visitor, ASTNode *node, char *name)
{
    // counts the number of times we see the symbol in the table
    int dup = 0;
    // for each symbol in the local symbols of the current tables, compare to the given symbol name
    FOR_EACH(Symbol *, sym, CURR_TABLE->local_symbols)
    {

        if (strncmp(name, sym->name, MAX_ID_LEN) == 0)
        {
            dup += 1;
        }
    }

    // if we saw the symbol more than once, its a duplicate symbol
    if (dup > 1)
    {
        ErrorList_printf(ERROR_LIST, "Duplicate symbol '%s' on line %d", name, node->source_line);
    }
}

/****************************** PRE VISITOR METHODS ******************************/

/**
 * @brief Set the current table
 */
void AnalysisVisitor_pre_program(NodeVisitor *visitor, ASTNode *node)
{
    CURR_TABLE = ASTNode_get_attribute(node, "symbolTable");
    PROGRAM_TABLE = ASTNode_get_attribute(node, "symbolTable");
}

/**
 * @brief Set the current table
 */
void AnalysisVisitor_pre_block(NodeVisitor *visitor, ASTNode *node)
{
    CURR_TABLE = ASTNode_get_attribute(node, "symbolTable");
}

/**
 * @brief Check to make sure that the nodes type is not void and set the inferred type (we added this)
 */
void AnalysisVisitor_pre_vardecl(NodeVisitor *visitor, ASTNode *node)
{
    // make sure that the type of the variable declaration is not void
    if (node->vardecl.type == VOID)
    {
        ErrorList_printf(ERROR_LIST, "Void variable '%s' on line %d", node->vardecl.name, node->source_line);
    }

    // make sure that the name of the variable is not main
    if (strcmp(node->vardecl.name, "main") == 0)
    {
        ErrorList_printf(ERROR_LIST, "Invalid variable name '%s' on line %d", node->vardecl.name, node->source_line);
    }

    // check if variable is an array and has valid length
    if (node->vardecl.is_array && node->vardecl.array_length < 0)
    {
        ErrorList_printf(ERROR_LIST, "Invalid array declaration. Array length must be greater than 0 but was %d", node->vardecl.array_length);
    }

    // set the inferred type
    SET_INFERRED_TYPE(node->vardecl.type);
}

/**
 * @brief set the inferred type (return type) for function declarations.  
 */
void AnalysisVisitor_pre_funcdecl(NodeVisitor *visitor, ASTNode *node)
{
    // set the current function to the name of this function
    CURR_FUNC = node->funcdecl.name;
    SET_INFERRED_TYPE(node->funcdecl.return_type);

    // Check for duplicate function declarations.
    check_for_duplicates(visitor, node, node->funcdecl.name);

    CURR_TABLE = ASTNode_get_attribute(node, "symbolTable");
}

/**
 * @brief set the inferred type for location (look up symbol)
 */
void AnalysisVisitor_pre_location(NodeVisitor *visitor, ASTNode *node)
{
    // look up the location and then set its type
    Symbol *loc = lookup_symbol(node, node->location.name);

    // error check, and then set inferred type
    if (loc == NULL)
    {
        SET_INFERRED_TYPE(VOID);
    }
    else
    {
        SET_INFERRED_TYPE(loc->type);
    }
}

/**
 * @brief set the inferred type for conditionals (which should always be bool)
 */
void AnalysisVisitor_pre_conditional(NodeVisitor *visitor, ASTNode *node)
{
    SET_INFERRED_TYPE(BOOL);
}

/**
 * @brief set the inferred type for while loop conditions (which should always be bool)
 */
void AnalysisVisitor_pre_while(NodeVisitor *visitor, ASTNode *node)
{
    // set boolean indicator to true for being in the while loop
    IN_WHILE = true;
    SET_INFERRED_TYPE(BOOL);
}

/**
 * @brief set the inferred type for return statements 
 */
void AnalysisVisitor_pre_return(NodeVisitor *visitor, ASTNode *node)
{
    // look up the symbol for the current function to get the expected return value
    Symbol *func = lookup_symbol(node, CURR_FUNC);
    // Set the inferred type
    if (func == NULL)
    {
        SET_INFERRED_TYPE(VOID);
    }
    else
    {
        SET_INFERRED_TYPE(func->type);
    }
}

/**
 * @brief set the inferred type for function calls
 */
void AnalysisVisitor_check_break(NodeVisitor *visitor, ASTNode *node)
{
    // error checking to make sure break statements appear in a while loop
    if (!IN_WHILE)
    {
        ErrorList_printf(ERROR_LIST, "Invalid break on line %d", node->source_line);
    }
}

/**
 * @brief set the inferred type for function calls
 */
void AnalysisVisitor_check_continue(NodeVisitor *visitor, ASTNode *node)
{
    // error checking to make sure continue statements appear in a while loop
    if (!IN_WHILE)
    {
        ErrorList_printf(ERROR_LIST, "Invalid continue on line %d", node->source_line);
    }
}

/**
 * @brief set the inferred type for binary operations
 */
void AnalysisVisitor_pre_binop(NodeVisitor *visitor, ASTNode *node)
{
    BinaryOpType op = node->binaryop.operator;

    // check the kind of op and set the inferred type
    if (op == ADDOP || op == SUBOP || op == MULOP || op == DIVOP || op == MODOP)
    {
        SET_INFERRED_TYPE(INT);
    }
    else
    {
        SET_INFERRED_TYPE(BOOL);
    }
}

/**
 * @brief set the inferred type for unary operations
 */
void AnalysisVisitor_pre_unop(NodeVisitor *visitor, ASTNode *node)
{
    UnaryOpType op = node->unaryop.operator;
    // Set the unary operation
    switch (op)
    {
    case NEGOP:
        SET_INFERRED_TYPE(INT);
    case NOTOP:
        SET_INFERRED_TYPE(BOOL);
    }
}

/**
 * @brief set the inferred type for function calls
 */
void AnalysisVisitor_pre_funcCall(NodeVisitor *visitor, ASTNode *node)
{
    // look up the symbol for the function to get the expected return type
    Symbol *func = lookup_symbol(node, node->funccall.name);

    if (func == NULL)
    {
        SET_INFERRED_TYPE(VOID);
    }
    else
    {
        SET_INFERRED_TYPE(func->type);
    }
}

/**
 * @brief set the inferred type for literals
 */
void AnalysisVisitor_pre_literal(NodeVisitor *visitor, ASTNode *node)
{
    SET_INFERRED_TYPE(node->literal.type);
}

/****************************** POST VISITOR METHODS ******************************/

/**
 * @brief Check for duplicate variables
 */
void AnalysisVisitor_post_vardecl(NodeVisitor *visitor, ASTNode *node)
{
    check_for_duplicates(visitor, node, node->vardecl.name);
}

/**
 * @brief Check to make sure that the location name is valid (we added this)
 */
void AnalysisVisitor_post_location(NodeVisitor *visitor, ASTNode *node)
{
    // makes sure that the location is valid (has been declared)
    if (node->location.index == NULL)
    { // if location is not an array
        Symbol *sym1 = lookup_symbol(node, node->location.name);

        if (sym1 == NULL)
        {
            lookup_symbol_with_reporting(visitor, node, node->location.name);
        }
        else if (sym1->length > 1)
        {
            ErrorList_printf(ERROR_LIST, "Invalid array access on line %d", node->source_line);
        }
    }
    else if (node->location.index != NULL)
    { // location is an array
        Symbol *sym = lookup_symbol(node, node->location.name);

        ASTNode *loc = node->location.index;
        int index = loc->literal.integer;

        // if the index is negative, print to errorlist
        if (index < 0) // this currently never runs because index will always be >= 0
        {
            ErrorList_printf(ERROR_LIST, "Array size '%s[%d]' on line %d is invalid", node->location.name, index, node->source_line);
        }
        else if (index >= sym->length) // if the index is greater than the array length
        {
            ErrorList_printf(ERROR_LIST, "Array access '%s[%d]' on line %d is invalid.", node->location.name, index, node->source_line);
        }
    }
}

/**
 * @brief post visit the function declaration to set current function to null to indicate that 
 * we are no longer in a function. And check for duplicate symbols
 */
void AnalysisVisitor_post_funcdecl(NodeVisitor *visitor, ASTNode *node)
{
    CURR_FUNC = NULL;
    CURR_TABLE = PROGRAM_TABLE;
}

/**
 * @brief Check program for main method
 */
void AnalysisVisitor_check_main(NodeVisitor *visitor, ASTNode *node)
{
    Symbol *sym = lookup_symbol(node, "main");
    // Checking if main function exists
    if (sym == NULL)
    {
        ErrorList_printf(ERROR_LIST, "Program does not contain a main function");
    }
    // Check that there's no parameters in main
    if (sym != NULL)
    {
        // look up the symbol for the function to get the expected return type
        Symbol *main = lookup_symbol(node, "main");

        if (main->parameters->head != NULL)
        {
            ErrorList_printf(ERROR_LIST, "Main method on line %d should not have any parameters", node->source_line);
        }
    }
}

/**
 * @brief check and make sure that assignment type matches declaration type
 */
void AnalysisVisitor_post_assignment(NodeVisitor *visitor, ASTNode *node)
{
    // initialize the location and value of the assignment for easier handling
    ASTNode *loc = node->assignment.location;
    ASTNode *val = node->assignment.value;

    // post visit check to make sure that the types match
    if (GET_INFERRED_TYPE(loc) != GET_INFERRED_TYPE(val))
    {
        ErrorList_printf(ERROR_LIST, "Type mismatch on line %d. Expected '%s' to be of type '%s', but was '%s'", node->source_line, loc->location.name, DecafType_to_string(GET_INFERRED_TYPE(loc)), DecafType_to_string(GET_INFERRED_TYPE(val)));
    }
}

/**
 * @brief check and make sure that return type matches func declaration
 */
void AnalysisVisitor_post_return(NodeVisitor *visitor, ASTNode *node)
{
    // if the return value is NULL but the method expects a return value
    if (GET_INFERRED_TYPE(node) != VOID && node->funcreturn.value == NULL)
    {
        ErrorList_printf(ERROR_LIST, "Type mismatch on line %d. Expected method to return type to be '%s', but was '%s'", node->source_line, DecafType_to_string(GET_INFERRED_TYPE(node)), DecafType_to_string(VOID));
    }
    else if (node->funcreturn.value != NULL && GET_INFERRED_TYPE(node->funcreturn.value) == VOID)
    {
        // Do nothing

    } // if the expected return type does not match the actual return type
    else if (node->funcreturn.value != NULL && GET_INFERRED_TYPE(node) != GET_INFERRED_TYPE(node->funcreturn.value))
    {
        ErrorList_printf(ERROR_LIST, "Type mismatch on line %d. Expected method to return type to be '%s', but was '%s'", node->source_line, DecafType_to_string(GET_INFERRED_TYPE(node)), DecafType_to_string(GET_INFERRED_TYPE(node->funcreturn.value)));
    }
}

/**
 * @brief Check to make sure conditional type is a bool 
 */
void AnalysisVisitor_post_conditional(NodeVisitor *visitor, ASTNode *node)
{
    // initialize the location and value of the assignment for easier handling
    ASTNode *val = node->conditional.condition;

    // check for error and print error to errorlist
    if (GET_INFERRED_TYPE(node) != GET_INFERRED_TYPE(val))
    {
        ErrorList_printf(ERROR_LIST, "Invalid condition on line %d. Expected condition to be of type '%s', but was '%s'", node->source_line, DecafType_to_string(GET_INFERRED_TYPE(node)), DecafType_to_string(GET_INFERRED_TYPE(val)));
    }
}

/**
 * @brief Postvisit binary op and check types
 */
void AnalysisVisitor_post_binop(NodeVisitor *visitor, ASTNode *node)
{
    // get the operator
    BinaryOpType op = node->binaryop.operator;

    // store the types of the left and right sides of the operator
    DecafType left = GET_INFERRED_TYPE(node->binaryop.left);
    DecafType right = GET_INFERRED_TYPE(node->binaryop.right);

    if (op == OROP || op == ANDOP) // AND / OR
    {
        if (left != BOOL || right != BOOL)
        {
            ErrorList_printf(ERROR_LIST, "Invalid binary operation on line %d. Expected 'bool %s bool' but was '%s %s %s'", node->source_line, BinaryOpToString(op), DecafType_to_string(left), BinaryOpToString(op), DecafType_to_string(right));
        }
    }
    else if (op == EQOP || op == NEQOP) // EQUAL / NOT EQUAL
    {
        if (left != right)
        {
            ErrorList_printf(ERROR_LIST, "Invalid binary operation on line %d. Expected values to be of the same type, but was '%s %s %s'", node->source_line, BinaryOpToString(op), DecafType_to_string(left), BinaryOpToString(op), DecafType_to_string(right));
        }
    }
    else if (op == LTOP || op == LEOP || op == GEOP || op == GTOP) // LESS THAN (OR EQUAL) / GREATER THAN (OR EQUAL)
    {
        if (left != INT || right != INT)
        {
            ErrorList_printf(ERROR_LIST, "Invalid binary operation on line %d. Expected 'int %s int' but was '%s %s %s'", node->source_line, BinaryOpToString(op), DecafType_to_string(left), BinaryOpToString(op), DecafType_to_string(right));
        }
    }
    else if (op == ADDOP || op == SUBOP || op == MULOP || op == DIVOP || op == MODOP) // ADD / SUBTRACT / MULTIPLY / DIVIDE / MOD
    {
        if (left != INT || right != INT)
        {
            ErrorList_printf(ERROR_LIST, "Invalid binary operation on line %d. Expected 'int %s int' but was '%s %s %s'", node->source_line, BinaryOpToString(op), DecafType_to_string(left), BinaryOpToString(op), DecafType_to_string(right));
        }
    }
}

/**
 * @brief Postvisit binary op and check types
 */
void AnalysisVisitor_post_unop(NodeVisitor *visitor, ASTNode *node)
{
    UnaryOpType op = node->unaryop.operator;

    // if the inferred type of the node does not match the inferred type of the unary child
    if (GET_INFERRED_TYPE(node) != GET_INFERRED_TYPE(node->unaryop.child))
    {
        ErrorList_printf(ERROR_LIST, "Invalid unary operation on line %d. Expected '%s%s' but was '%s%s'", node->source_line, UnaryOpToString(op), DecafType_to_string(GET_INFERRED_TYPE(node)), UnaryOpToString(op), DecafType_to_string(GET_INFERRED_TYPE(node->unaryop.child)));
    }
}

/**
 * @brief set in_while to false to indicate that we are no longer in a while loop
 */
void AnalysisVisitor_post_while(NodeVisitor *visitor, ASTNode *node)
{
    IN_WHILE = false;
}

/**
 * @brief check parameter types for function calls
 */
void AnalysisVisitor_post_funcCall(NodeVisitor *visitor, ASTNode *node)
{
    // look up the symbol for the function to get the expected return type
    Symbol *func = lookup_symbol(node, node->funccall.name);

    if (func->parameters->head->type != GET_INFERRED_TYPE(node->funccall.arguments->head))
    {
        ErrorList_printf(ERROR_LIST, "Invalid argument type on line %d", node->source_line);
    }
}

ErrorList *analyze(ASTNode *tree)
{
    /* allocate analysis structures */
    NodeVisitor *v = NodeVisitor_new();
    v->data = (void *)AnalysisData_new();
    v->dtor = (Destructor)AnalysisData_free;

    /* previsit program calls */
    v->previsit_program = &AnalysisVisitor_pre_program;
    v->previsit_block = &AnalysisVisitor_pre_block;
    v->previsit_vardecl = &AnalysisVisitor_pre_vardecl;
    v->previsit_funcdecl = &AnalysisVisitor_pre_funcdecl;
    v->previsit_location = &AnalysisVisitor_pre_location;
    v->previsit_conditional = &AnalysisVisitor_pre_conditional;
    v->previsit_whileloop = &AnalysisVisitor_pre_while;
    v->previsit_break = &AnalysisVisitor_check_break;
    v->previsit_continue = &AnalysisVisitor_check_continue;
    v->previsit_binaryop = &AnalysisVisitor_pre_binop;
    v->previsit_unaryop = &AnalysisVisitor_pre_unop;
    v->previsit_return = &AnalysisVisitor_pre_return;
    v->previsit_funccall = &AnalysisVisitor_pre_funcCall;
    v->previsit_literal = &AnalysisVisitor_pre_literal;

    /* postvisit program calls */
    v->postvisit_program = &AnalysisVisitor_check_main;
    v->postvisit_vardecl = &AnalysisVisitor_post_vardecl;
    v->postvisit_funcdecl = &AnalysisVisitor_post_funcdecl;
    v->postvisit_funccall = &AnalysisVisitor_post_funcCall;
    v->postvisit_assignment = &AnalysisVisitor_post_assignment;
    v->postvisit_conditional = &AnalysisVisitor_post_conditional;
    v->postvisit_location = &AnalysisVisitor_post_location;
    v->postvisit_return = &AnalysisVisitor_post_return;
    v->postvisit_binaryop = &AnalysisVisitor_post_binop;
    v->postvisit_unaryop = &AnalysisVisitor_post_unop;

    /* perform analysis, save error list, clean up, and return errors */

    /* adding for null tree test */
    if (tree == NULL)
    {
        ErrorList_printf((((AnalysisData *)v->data)->errors), "Null tree");
    }
    else
    {
        NodeVisitor_traverse(v, tree);
    }

    ErrorList *errors = ((AnalysisData *)v->data)->errors;
    NodeVisitor_free(v);
    return errors;
}
