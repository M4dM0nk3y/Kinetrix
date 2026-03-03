/* Kinetrix Symbol Table
 * Manages variable and function scopes for semantic analysis
 */

#ifndef KINETRIX_SYMBOL_TABLE_H
#define KINETRIX_SYMBOL_TABLE_H

#include "ast.h"

// ============================================================================
// SYMBOL TABLE
// ============================================================================

typedef enum {
    SYMBOL_VARIABLE,
    SYMBOL_FUNCTION,
    SYMBOL_PARAMETER,
    SYMBOL_DEVICE
} SymbolKind;

typedef struct Symbol {
    char *name;
    SymbolKind kind;
    Type *type;
    ProtocolType protocol;
    int is_initialized;
    int is_used;
    int line_defined;  // For error messages
    struct Symbol *next;  // For hash table chaining
} Symbol;

typedef struct Scope {
    Symbol **symbols;  // Hash table
    int table_size;
    struct Scope *parent;  // For nested scopes
    int depth;
} Scope;

typedef struct SymbolTable {
    Scope *current_scope;
    Scope *global_scope;
} SymbolTable;

// ============================================================================
// SYMBOL TABLE OPERATIONS
// ============================================================================

// Create/destroy
SymbolTable* symbol_table_create();
void symbol_table_free(SymbolTable *table);

// Scope management
void symbol_table_enter_scope(SymbolTable *table);
void symbol_table_exit_scope(SymbolTable *table);

// Symbol operations
int symbol_table_add(SymbolTable *table, const char *name, SymbolKind kind, Type *type, int line);
Symbol* symbol_table_lookup(SymbolTable *table, const char *name);
Symbol* symbol_table_lookup_current_scope(SymbolTable *table, const char *name);
Symbol* symbol_table_add_device(SymbolTable *table, const char *name, ProtocolType protocol);

// Utilities
void symbol_table_print(SymbolTable *table);  // For debugging

#endif // KINETRIX_SYMBOL_TABLE_H
