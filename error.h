/* Kinetrix Error Handling
 * Professional error reporting with source locations
 */

#ifndef KINETRIX_ERROR_H
#define KINETRIX_ERROR_H

#include <stdio.h>
#include <stdarg.h>

// ============================================================================
// ERROR TYPES
// ============================================================================

typedef enum {
    ERROR_LEXICAL,
    ERROR_SYNTAX,
    ERROR_SEMANTIC,
    ERROR_TYPE,
    ERROR_INTERNAL
} ErrorType;

typedef struct Error {
    ErrorType type;
    int line;
    int column;
    char *message;
    struct Error *next;
} Error;

typedef struct ErrorList {
    Error *head;
    Error *tail;
    int count;
    int max_errors;  // Stop after this many errors
} ErrorList;

// ============================================================================
// ERROR MANAGEMENT
// ============================================================================

ErrorList* error_list_create(int max_errors);
void error_list_free(ErrorList *list);

void error_report(ErrorList *list, ErrorType type, int line, int column, const char *format, ...);
void error_print_all(ErrorList *list, FILE *output);
int error_has_errors(ErrorList *list);

// Convenience macros
#define LEXICAL_ERROR(list, line, col, ...) \
    error_report(list, ERROR_LEXICAL, line, col, __VA_ARGS__)

#define SYNTAX_ERROR(list, line, col, ...) \
    error_report(list, ERROR_SYNTAX, line, col, __VA_ARGS__)

#define SEMANTIC_ERROR(list, line, col, ...) \
    error_report(list, ERROR_SEMANTIC, line, col, __VA_ARGS__)

#define TYPE_ERROR(list, line, col, ...) \
    error_report(list, ERROR_TYPE, line, col, __VA_ARGS__)

#define INTERNAL_ERROR(list, line, col, ...) \
    error_report(list, ERROR_INTERNAL, line, col, __VA_ARGS__)

#endif // KINETRIX_ERROR_H
