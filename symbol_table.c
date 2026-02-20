/* Kinetrix Symbol Table Implementation */

#include "symbol_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HASH_TABLE_SIZE 128

// ============================================================================
// HASH FUNCTION
// ============================================================================

static unsigned int hash(const char *str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % HASH_TABLE_SIZE;
}

// ============================================================================
// SCOPE MANAGEMENT
// ============================================================================

static Scope* scope_create(Scope *parent) {
    Scope *scope = malloc(sizeof(Scope));
    scope->symbols = calloc(HASH_TABLE_SIZE, sizeof(Symbol*));
    scope->table_size = HASH_TABLE_SIZE;
    scope->parent = parent;
    scope->depth = parent ? parent->depth + 1 : 0;
    return scope;
}

static void scope_free(Scope *scope) {
    if (scope == NULL) return;
    
    for (int i = 0; i < scope->table_size; i++) {
        Symbol *sym = scope->symbols[i];
        while (sym != NULL) {
            Symbol *next = sym->next;
            free(sym->name);
            type_free(sym->type);
            free(sym);
            sym = next;
        }
    }
    
    free(scope->symbols);
    free(scope);
}

// ============================================================================
// SYMBOL TABLE OPERATIONS
// ============================================================================

SymbolTable* symbol_table_create() {
    SymbolTable *table = malloc(sizeof(SymbolTable));
    table->global_scope = scope_create(NULL);
    table->current_scope = table->global_scope;
    return table;
}

void symbol_table_free(SymbolTable *table) {
    if (table == NULL) return;
    
    // Free all scopes (walk up from current to global)
    Scope *scope = table->global_scope;
    while (scope != NULL) {
        Scope *parent = scope->parent;
        scope_free(scope);
        scope = parent;
    }
    
    free(table);
}

void symbol_table_enter_scope(SymbolTable *table) {
    table->current_scope = scope_create(table->current_scope);
}

void symbol_table_exit_scope(SymbolTable *table) {
    if (table->current_scope == table->global_scope) {
        fprintf(stderr, "Error: Cannot exit global scope\n");
        return;
    }
    
    Scope *old_scope = table->current_scope;
    table->current_scope = old_scope->parent;
    scope_free(old_scope);
}

int symbol_table_add(SymbolTable *table, const char *name, SymbolKind kind, Type *type, int line) {
    if (table == NULL || name == NULL) return 0;
    
    // Check if symbol already exists in current scope
    if (symbol_table_lookup_current_scope(table, name) != NULL) {
        return 0;  // Symbol already exists
    }
    
    unsigned int index = hash(name);
    
    Symbol *sym = malloc(sizeof(Symbol));
    sym->name = strdup(name);
    sym->kind = kind;
    sym->type = type_clone(type);
    sym->is_initialized = 0;
    sym->line_defined = line;
    sym->next = table->current_scope->symbols[index];
    
    table->current_scope->symbols[index] = sym;
    return 1;
}

Symbol* symbol_table_lookup(SymbolTable *table, const char *name) {
    if (table == NULL || name == NULL) return NULL;
    
    Scope *scope = table->current_scope;
    unsigned int index = hash(name);
    
    // Search from current scope up to global scope
    while (scope != NULL) {
        Symbol *sym = scope->symbols[index];
        while (sym != NULL) {
            if (strcmp(sym->name, name) == 0) {
                return sym;
            }
            sym = sym->next;
        }
        scope = scope->parent;
    }
    
    return NULL;
}

Symbol* symbol_table_lookup_current_scope(SymbolTable *table, const char *name) {
    if (table == NULL || name == NULL) return NULL;
    
    unsigned int index = hash(name);
    Symbol *sym = table->current_scope->symbols[index];
    
    while (sym != NULL) {
        if (strcmp(sym->name, name) == 0) {
            return sym;
        }
        sym = sym->next;
    }
    
    return NULL;
}

void symbol_table_print(SymbolTable *table) {
    if (table == NULL) return;
    
    printf("=== Symbol Table ===\n");
    Scope *scope = table->current_scope;
    
    while (scope != NULL) {
        printf("Scope depth %d:\n", scope->depth);
        for (int i = 0; i < scope->table_size; i++) {
            Symbol *sym = scope->symbols[i];
            while (sym != NULL) {
                printf("  %s: %s (line %d)\n", sym->name, type_to_string(sym->type), sym->line_defined);
                sym = sym->next;
            }
        }
        scope = scope->parent;
    }
}
