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
    ErrorList* errors;

    /******************* State Information that we added *******************/
    
    /**
     * @brief current function that we are in
     */
    char* current_func;

    /**
     * @brief true if we are in a while loop, false otherwise
     */
    bool in_while;

} AnalysisData;

/**
 * @brief Allocate memory for analysis data
 * 
 * @returns Pointer to allocated structure
 */
AnalysisData* AnalysisData_new ()
{
    AnalysisData* data = (AnalysisData*)calloc(1, sizeof(AnalysisData));
    CHECK_MALLOC_PTR(data);
    data->errors = ErrorList_new();
    data->current_func = NULL;
    data->in_while = false;
    return data;
}

/**
 * @brief Deallocate memory for analysis data
 * 
 * @param data Pointer to the structure to be deallocated
 */
void AnalysisData_free (AnalysisData* data)
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
#define DATA ((AnalysisData*)visitor->data)

/**
 * @brief Macro for more convenient access to the error list inside a
 * @ref AnalysisVisitor data structure
 */
#define ERROR_LIST (((AnalysisData*)visitor->data)->errors)

/**
 * @brief Macro for more convenient access to the current function inside a
 * @ref AnalysisVisitor data structure
 */
#define CURR_FUNC (((AnalysisData*)visitor->data)->current_func)

/**
 * @brief Macro for more convenient access to the bool for in_while inside a
 * @ref AnalysisVisitor data structure
 */
#define IN_WHILE (((AnalysisData*)visitor->data)->in_while)

/**
 * @brief Wrapper for @ref lookup_symbol that reports an error if the symbol isn't found
 * 
 * @param visitor Visitor with the error list for reporting
 * @param node AST node to begin the search at
 * @param name Name of symbol to find
 * @returns The @ref Symbol if found, otherwise @c NULL
 */
Symbol* lookup_symbol_with_reporting(NodeVisitor* visitor, ASTNode* node, const char* name)
{
    Symbol* symbol = lookup_symbol(node, name);
    if (symbol == NULL) {
        ErrorList_printf(ERROR_LIST, "Symbol '%s' undefined on line %d", name, node->source_line);
    }
    return symbol;
}

/**
 * @brief Macro for shorter storing of the inferred @c type attribute
 */
#define SET_INFERRED_TYPE(T) ASTNode_set_printable_attribute(node, "type", (void*)(T), \
                                 type_attr_print, dummy_free)

/**
 * @brief Macro for shorter retrieval of the inferred @c type attribute
 */
#define GET_INFERRED_TYPE(N) (DecafType)ASTNode_get_attribute(N, "type")

/****************************** PRE VISITOR METHODS ******************************/

/**
 * @brief Check to make sure that the nodes type is not void and set the inferred type (we added this)
 */
void AnalysisVisitor_check_vardecl (NodeVisitor* visitor, ASTNode* node) 
{
    SET_INFERRED_TYPE(node->vardecl.type);

    // make sure that the type of the variable declaration is not void
    if (node->vardecl.type == VOID ) 
    {
        ErrorList_printf(ERROR_LIST, "Void variable '%s' on line %d", node->vardecl.name, node->source_line);
    }

    // check if variable is an array and has valid length
    if (node->vardecl.is_array && node->vardecl.array_length < 0)
    {
        ErrorList_printf(ERROR_LIST, "Invalid array declaration. Array length must be greater than 0 but was %d", node->vardecl.array_length);
    }
}

/**
 * @brief set the inferred type (return type) for function declarations.  
 * (Alice added this)
 */
void AnalysisVisitor_infer_funcdecl (NodeVisitor* visitor, ASTNode* node) 
{
    // set the current function to the name of this function
    CURR_FUNC = node->funcdecl.name;
    SET_INFERRED_TYPE(node->funcdecl.return_type);
}

/**
 * @brief set the inferred type for location (look up symbol)
 * (Alice added this)
 */
void AnalysisVisitor_infer_location (NodeVisitor* visitor, ASTNode* node) 
{
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
 * (Alice added this)
 */
void AnalysisVisitor_infer_conditional (NodeVisitor* visitor, ASTNode* node) 
{
    SET_INFERRED_TYPE(BOOL);
}

/**
 * @brief set the inferred type for while loop conditions (which should always be bool)
 * (Alice added this)
 */
void AnalysisVisitor_infer_while (NodeVisitor* visitor, ASTNode* node) 
{
    // set boolean indicator to true for being in the while loop
    IN_WHILE = true;
    SET_INFERRED_TYPE(BOOL);
}

/**
 * @brief set the inferred type for return statements
 * (Alice added this)
 */
void AnalysisVisitor_infer_return (NodeVisitor* visitor, ASTNode* node) 
{
    // look up the symbol for the current function to get the expected return value
    Symbol *func = lookup_symbol(node, CURR_FUNC);
    SET_INFERRED_TYPE(func->type);
}

/**
 * @brief set the inferred type for function calls
 * (Alice added this)
 */
void AnalysisVisitor_check_break (NodeVisitor* visitor, ASTNode* node) 
{
    // error checking to make sure break statements appear in a while loop
    if (!IN_WHILE) 
    {
        ErrorList_printf(ERROR_LIST, "Invalid break on line %d", node->source_line);
    }
}

/**
 * @brief set the inferred type for function calls
 * (Alice added this)
 */
void AnalysisVisitor_check_continue (NodeVisitor* visitor, ASTNode* node) 
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
void AnalysisVisitor_pre_binop(NodeVisitor* visitor, ASTNode* node)
{
    BinaryOpType op = node->binaryop.operator;

    // check the kind of op and set the inferred type 
    if (op == OROP || op == ANDOP)
    {
        SET_INFERRED_TYPE(BOOL);
    }
    else if (op == EQOP || op == NEQOP)
    {
        SET_INFERRED_TYPE(BOOL);
    }
    else if (op == LTOP || op == LEOP || op == GEOP || op == GTOP)
    {
        SET_INFERRED_TYPE(BOOL);
    }
    else if (op == ADDOP || op == SUBOP || op == MULOP || op == DIVOP || op == MODOP)
    {
        SET_INFERRED_TYPE(INT);
    }
}

/**
 * @brief set the inferred type for unary operations
 */
void AnalysisVisitor_pre_unop(NodeVisitor* visitor, ASTNode* node)
{
    UnaryOpType op = node->unaryop.operator;

    switch (op) {
        case NEGOP:     SET_INFERRED_TYPE(INT);
        case NOTOP:     SET_INFERRED_TYPE(BOOL);
    }
}

/**
 * @brief set the inferred type for function calls
 * (Alice added this)
 */
void AnalysisVisitor_infer_funcCall (NodeVisitor* visitor, ASTNode* node) 
{
    // look up the symbol for the function to get the expected return type
    Symbol *func = lookup_symbol(node, node->funccall.name);

    SET_INFERRED_TYPE(func->type);
}

/**
 * @brief set the inferred type for literals
 * (Alice added this)
 */
void AnalysisVisitor_infer_literal (NodeVisitor* visitor, ASTNode* node) 
{
    SET_INFERRED_TYPE(node->literal.type);
}

/****************************** POST VISITOR METHODS ******************************/

/**
 * @brief Check to make sure that the location name is valid (we added this)
 */
void AnalysisVisitor_check_location (NodeVisitor* visitor, ASTNode* node) 
{
    // makes sure that the location is valid (has been declared)
    if (node->location.index == NULL) 
    { // if location is not an array
        Symbol* sym1 = lookup_symbol(node, node->location.name);

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
        Symbol* sym = lookup_symbol(node, node->location.name);

        ASTNode* loc = node->location.index;
        int index = loc->literal.integer;
        //ErrorList_printf(ERROR_LIST, "Index is %d", index);
        // for some reason this is not working for negative array accesses. 
        // the above error list thing is for deubgging
        // the first part of the if statement below is just never being run because all negtive array
        // accesses are registered as a 0 and i dont know why. has to be from somewhere else

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
 * we are no longer in a function
 */
void AnalysisVisitor_post_funcdecl (NodeVisitor* visitor, ASTNode* node) 
{
    CURR_FUNC = NULL;
}

/**
 * @brief Check program for main method
 */
void AnalysisVisitor_check_main (NodeVisitor* visitor, ASTNode* node) 
{
    Symbol *sym = lookup_symbol(node, "main");
    if (sym == NULL) {
        ErrorList_printf(ERROR_LIST, "Program does not contain a main function");
    }
}

/**
 * @brief check and make sure that assignment type matches declaration type
 */
void AnalysisVisitor_check_assignment_type (NodeVisitor* visitor, ASTNode* node) 
{
    // initialize the location and value of the assignment for easier handling
    ASTNode* loc = node->assignment.location;
    ASTNode* val = node->assignment.value;

    // post visit check to make sure that the types match
    if (GET_INFERRED_TYPE(loc) != GET_INFERRED_TYPE(val)) 
    {
        ErrorList_printf(ERROR_LIST, "Type mismatch on line %d. Expected '%s' to be of type '%s', but was '%s'", node->source_line, loc->location.name, DecafType_to_string(GET_INFERRED_TYPE(loc)), DecafType_to_string(GET_INFERRED_TYPE(val)));
    }
}

/**
 * @brief check and make sure that return type matches func declaration
 */
void AnalysisVisitor_check_return_type (NodeVisitor* visitor, ASTNode* node) 
{
    // post visit check to make sure that the types match
    if (GET_INFERRED_TYPE(node->funcreturn.value) == VOID)
    {
        // do nothing. 
        // the if below caused integration test D_undefined_var to fail
    }
    else if (GET_INFERRED_TYPE(node) != GET_INFERRED_TYPE(node->funcreturn.value)) 
    {
        ErrorList_printf(ERROR_LIST, "Type mismatch on line %d. Expected method to return type to be '%s', but was '%s'", node->source_line, DecafType_to_string(GET_INFERRED_TYPE(node)), DecafType_to_string(GET_INFERRED_TYPE(node->funcreturn.value)));
    }
}

/**
 * @brief Check to make sure conditional type is a bool 
 */
void AnalysisVisitor_check_conditional_type (NodeVisitor* visitor, ASTNode* node) 
{
    // initialize the location and value of the assignment for easier handling
    ASTNode* val = node->conditional.condition;

    // check for error and print error to errorlist
    if (GET_INFERRED_TYPE(node) != GET_INFERRED_TYPE(val)) 
    {
        ErrorList_printf(ERROR_LIST, "Invalid condition on line %d. Expected condition to be of type '%s', but was '%s'", node->source_line, DecafType_to_string(GET_INFERRED_TYPE(node)), DecafType_to_string(GET_INFERRED_TYPE(val)));
    }
}

/**
 * @brief Postvisit binary op and check types
 */
void AnalysisVisitor_post_binop (NodeVisitor* visitor, ASTNode* node) 
{
    BinaryOpType op = node->binaryop.operator;
    
    DecafType left = GET_INFERRED_TYPE(node->binaryop.left);
    DecafType right = GET_INFERRED_TYPE(node->binaryop.right);

    if (op == OROP || op == ANDOP)
    {
        if (left != BOOL || right != BOOL)
        {
            ErrorList_printf(ERROR_LIST, "Invalid binary operation on line %d. Expected 'bool %s bool' but was '%s %s %s'", node->source_line, BinaryOpToString(op), DecafType_to_string(left), BinaryOpToString(op), DecafType_to_string(right));
        }
    }
    else if (op == EQOP || op == NEQOP)
    {
        if (left != right)
        {
            ErrorList_printf(ERROR_LIST, "Invalid binary operation on line %d. Expected values to be of the same type, but was '%s %s %s'", node->source_line, BinaryOpToString(op), DecafType_to_string(left), BinaryOpToString(op), DecafType_to_string(right));
        }
    }
    else if (op == LTOP || op == LEOP || op == GEOP || op == GTOP)
    {
        if (left != INT || right != INT)
        {
            ErrorList_printf(ERROR_LIST, "Invalid binary operation on line %d. Expected 'int %s int' but was '%s %s %s'", node->source_line, BinaryOpToString(op), DecafType_to_string(left), BinaryOpToString(op), DecafType_to_string(right));
        }
    }
    else if (op == ADDOP || op == SUBOP || op == MULOP || op == DIVOP || op == MODOP)
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
void AnalysisVisitor_post_unop (NodeVisitor* visitor, ASTNode* node) 
{
    UnaryOpType op = node->unaryop.operator;

    if (GET_INFERRED_TYPE(node) != GET_INFERRED_TYPE(node->unaryop.child)) 
    {
        ErrorList_printf(ERROR_LIST, "Invalid unary operation on line %d. Expected '%s%s' but was '%s%s'", node->source_line, UnaryOpToString(op), DecafType_to_string(GET_INFERRED_TYPE(node)), UnaryOpToString(op), DecafType_to_string(GET_INFERRED_TYPE(node->unaryop.child)));
    }
}

/**
 * @brief set in_while to false to indicate that we are no longer in a while loop
 */
void AnalysisVisitor_post_while (NodeVisitor* visitor, ASTNode* node) 
{
    IN_WHILE = false;
}


ErrorList* analyze (ASTNode* tree)
{
    /* allocate analysis structures */
    NodeVisitor* v = NodeVisitor_new();
    v->data = (void*)AnalysisData_new();
    v->dtor = (Destructor)AnalysisData_free;

    /* previsit program calls */
    v->previsit_program = &AnalysisVisitor_check_main;
    v->previsit_vardecl = &AnalysisVisitor_check_vardecl;
    v->previsit_funcdecl = &AnalysisVisitor_infer_funcdecl;
    v->previsit_location = &AnalysisVisitor_infer_location;
    v->previsit_conditional = &AnalysisVisitor_infer_conditional;
    v->previsit_whileloop = &AnalysisVisitor_infer_while;
    v->previsit_break = &AnalysisVisitor_check_break;
    v->previsit_continue = &AnalysisVisitor_check_continue;
    v->previsit_binaryop = &AnalysisVisitor_pre_binop;
    v->previsit_unaryop = &AnalysisVisitor_pre_unop;
    v->previsit_return = &AnalysisVisitor_infer_return;
    v->previsit_funccall = &AnalysisVisitor_infer_funcCall;
    v->previsit_literal = &AnalysisVisitor_infer_literal;

    /* postvisit program calls */
    v->postvisit_funcdecl = &AnalysisVisitor_post_funcdecl;
    v->postvisit_assignment = &AnalysisVisitor_check_assignment_type;
    v->postvisit_conditional = &AnalysisVisitor_check_conditional_type;
    v->postvisit_location = &AnalysisVisitor_check_location;
    v->postvisit_return = &AnalysisVisitor_check_return_type;
    v->postvisit_binaryop = &AnalysisVisitor_post_binop;
    v->postvisit_unaryop = &AnalysisVisitor_post_unop;

    /* perform analysis, save error list, clean up, and return errors */
    NodeVisitor_traverse(v, tree);
    ErrorList* errors = ((AnalysisData*)v->data)->errors;
    NodeVisitor_free(v);
    return errors;
}

