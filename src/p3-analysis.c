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

/**
 * @brief Check to make sure that the nodes type is not void (we added this)
 */
void AnalysisVisitor_check_vardecl (NodeVisitor* visitor, ASTNode* node) {
    if (node -> type != VARDECL) {
        // error
    }
    if (node -> vardecl.type == VOID ) {
        ErrorList_printf(ERROR_LIST, "Void variable '%s' on line %d", node->vardecl.name, node->source_line);
    }
}

/**
 * @brief Check to make sure that the location name is valid (we added this)
 */
void AnalysisVisitor_check_location (NodeVisitor* visitor, ASTNode* node) {

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
void AnalysisVisitor_check_main (NodeVisitor* visitor, ASTNode* node) {
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

    // find the symbol declaration for the location name
    Symbol *sym = lookup_symbol(node, loc->location.name);

    if (sym == NULL) // error
    {
        ErrorList_printf(ERROR_LIST, "Invalid assignment operation on line %d", node->source_line);
    } 

    // check what the value type is, then check it with the symbol expected type
    if (val->type == BINARYOP) 
    {
        // NEEDS TO BE COMPLETED
        // mainly am confused as to if it needs a loop or not
        // also unsure if this would technically be handled somewhere else.
        // like parsing expressions
    } 
    else if (val->type == LOCATION) 
    {
        // look up the type of the location
        Symbol *valSym = lookup_symbol(val, val->location.name);

        // check for error and print error to errorlist
        if (sym->type != valSym->type) 
        {
            ErrorList_printf(ERROR_LIST, "Type mismatch on line %d. Expected '%s' to be of type '%s', but was '%s'", node->source_line, loc->location.name, DecafType_to_string(sym->type), DecafType_to_string(valSym->type));
        }
    } 
    else if (val->type == FUNCCALL) 
    {
        // look up the type of the funccall
        Symbol *funcSym = lookup_symbol(val, val->funccall.name);
        
        // check for error and print error to errorlist
        if (sym->type != funcSym->type) 
        {
            ErrorList_printf(ERROR_LIST, "Type mismatch on line %d. Expected '%s' to be of type '%s', but was '%s'", node->source_line, loc->location.name, DecafType_to_string(sym->type), DecafType_to_string(funcSym->type));
        }
    } 
    else if (val->type == LITERAL) 
    {
        // check for error and print error to errorlist
        if (sym->type != val->literal.type) 
        {
            ErrorList_printf(ERROR_LIST, "Type mismatch on line %d. Expected '%s' to be of type '%s', but was '%s'", node->source_line, loc->location.name, DecafType_to_string(sym->type), DecafType_to_string(val->literal.type));
        }
    } 
    else 
    {
        // something went wrong
        // unsure if we need this or not
        ErrorList_printf(ERROR_LIST, "Invalid assignment operation on line %d", node->source_line);
    }

}

/**
 * @brief Check to make sure conditional type is a bool (Alice added this)
 * I think that this method can be condensed
 */
void AnalysisVisitor_check_conditional_type (NodeVisitor* visitor, ASTNode* node) {
    // initialize the location and value of the assignment for easier handling
    ASTNode* val = node->conditional.condition;

    // check what the value type is, then check it with the symbol expected type
    if (val->type == BINARYOP) 
    {
        // NEEDS TO BE COMPLETED
        // mainly am confused as to if it needs a loop or not
        // also unsure if this would technically be handled somewhere else.
        // like parsing expressions
    } 
    else if (val->type == LOCATION) 
    {
        // look up the type of the location
        Symbol *valSym = lookup_symbol(val, val->location.name);

        // check for error and print error to errorlist
        if (valSym->type != BOOL) 
        {
            ErrorList_printf(ERROR_LIST, "Invalid condition on line %d. Expected '%s' to be of type 'BOOL', but was '%s'", node->source_line, node->conditional.condition, DecafType_to_string(valSym->type));
        }
    } 
    else if (val->type == FUNCCALL) 
    {
        // look up the type of the funccall
        Symbol *funcSym = lookup_symbol(val, val->funccall.name);
        
        // check for error and print error to errorlist
        if (funcSym->type != BOOL) 
        {
             ErrorList_printf(ERROR_LIST, "Invalid condition on line %d. Expected '%s' to be of type 'BOOL', but was '%s'", node->source_line, node->conditional.condition, DecafType_to_string(funcSym->type));
        }
    } 
    else if (val->type == LITERAL) 
    {
        // check for error and print error to errorlist
        if (val->literal.type != BOOL) 
        {
             ErrorList_printf(ERROR_LIST, "Invalid condition on line %d. Expected '%s' to be of type 'BOOL', but was '%s'", node->source_line, node->conditional.condition, DecafType_to_string(val->literal.type));
        }
    } 
    else 
    {
        // something went wrong
        // unsure if we need this or not
        ErrorList_printf(ERROR_LIST, "Invalid assignment operation on line %d", node->source_line);
    }
}


ErrorList* analyze (ASTNode* tree)
{
    /* allocate analysis structures */
    NodeVisitor* v = NodeVisitor_new();
    v->data = (void*)AnalysisData_new();
    v->dtor = (Destructor)AnalysisData_free;

    /* BOILERPLATE: TODO: register analysis callbacks */
    v->previsit_program = &AnalysisVisitor_check_main;
    v->previsit_vardecl = &AnalysisVisitor_check_vardecl;
    v->previsit_location = &AnalysisVisitor_check_location;
    v->previsit_assignment = &AnalysisVisitor_check_assignment_type;
    v->previsit_conditional = &AnalysisVisitor_check_conditional_type;

    /* perform analysis, save error list, clean up, and return errors */
    NodeVisitor_traverse(v, tree);
    ErrorList* errors = ((AnalysisData*)v->data)->errors;
    NodeVisitor_free(v);
    return errors;
}

