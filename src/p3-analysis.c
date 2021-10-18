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

    /* BOILERPLATE: TODO: add any new desired state information (and clean it up in AnalysisData_free) */

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
    // set attribute for this node
    SET_INFERRED_TYPE(node->vardecl.type);

    if (node -> vardecl.type == VOID ) 
    {
        ErrorList_printf(ERROR_LIST, "Void variable '%s' on line %d", node->vardecl.name, node->source_line);
    }
}

/**
 * @brief set the inferred type (return type) for function declarations.  
 * (Alice added this)
 */
void AnalysisVisitor_infer_funcdecl (NodeVisitor* visitor, ASTNode* node) 
{
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
    else{
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
    SET_INFERRED_TYPE(BOOL);
}

/**
 * @brief set the inferred type for return statements
 * (Alice added this)
 */
void AnalysisVisitor_infer_return (NodeVisitor* visitor, ASTNode* node) 
{
    SET_INFERRED_TYPE(node->funcreturn.value);
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
        lookup_symbol_with_reporting(visitor, node, node->location.name);
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
 * @brief Check funcdecl for main method (we added this)
 */
void AnalysisVisitor_check_main (NodeVisitor* visitor, ASTNode* node) 
{
    Symbol *sym = lookup_symbol(node, "main");
    if (sym == NULL) {
        ErrorList_printf(ERROR_LIST, "Program does not contain a main function");
    }
}

/**
 * @brief check and make sure that assignment type matches declaration type (Alice added this)
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
 * @brief Check to make sure conditional type is a bool (Alice added this)
 * I think that this method can be condensed
 */
void AnalysisVisitor_check_conditional_type (NodeVisitor* visitor, ASTNode* node) {
    // initialize the location and value of the assignment for easier handling
    ASTNode* val = node->conditional.condition;

    // check for error and print error to errorlist
    if (GET_INFERRED_TYPE(node) != GET_INFERRED_TYPE(val)) 
    {
        ErrorList_printf(ERROR_LIST, "Invalid condition on line %d. Expected condition to be of type '%s', but was '%s'", node->source_line, DecafType_to_string(GET_INFERRED_TYPE(node)), DecafType_to_string(GET_INFERRED_TYPE(val)));
    }
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
    v->previsit_location = &AnalysisVisitor_infer_location;
    v->previsit_conditional = &AnalysisVisitor_infer_conditional;
    v->previsit_literal = &AnalysisVisitor_infer_literal;

    /* postvisit program calls */
    v->postvisit_location = &AnalysisVisitor_check_location;
    v->postvisit_assignment = &AnalysisVisitor_check_assignment_type;
    v->postvisit_conditional = &AnalysisVisitor_check_conditional_type;
    v->postvisit_location = &AnalysisVisitor_check_location;

    /* perform analysis, save error list, clean up, and return errors */
    NodeVisitor_traverse(v, tree);
    ErrorList* errors = ((AnalysisData*)v->data)->errors;
    NodeVisitor_free(v);
    return errors;
}

