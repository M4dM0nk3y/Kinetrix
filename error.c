/* Kinetrix Error Handling Implementation */

#include "error.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// ERROR MANAGEMENT
// ============================================================================

ErrorList* error_list_create(int max_errors) {
    ErrorList *list = malloc(sizeof(ErrorList));
    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
    list->max_errors = max_errors;
    return list;
}

void error_list_free(ErrorList *list) {
    if (list == NULL) return;
    
    Error *err = list->head;
    while (err != NULL) {
        Error *next = err->next;
        free(err->message);
        free(err);
        err = next;
    }
    
    free(list);
}

void error_report(ErrorList *list, ErrorType type, int line, int column, const char *format, ...) {
    if (list == NULL) return;
    
    // Stop adding errors if we've hit the limit
    if (list->count >= list->max_errors) return;
    
    Error *err = malloc(sizeof(Error));
    err->type = type;
    err->line = line;
    err->column = column;
    err->next = NULL;
    
    // Format the message
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    err->message = strdup(buffer);
    
    // Add to list
    if (list->tail == NULL) {
        list->head = list->tail = err;
    } else {
        list->tail->next = err;
        list->tail = err;
    }
    
    list->count++;
}

void error_print_all(ErrorList *list, FILE *output) {
    if (list == NULL || output == NULL) return;
    
    const char *type_names[] = {
        "Lexical Error",
        "Syntax Error",
        "Semantic Error",
        "Type Error",
        "Internal Error"
    };
    
    Error *err = list->head;
    while (err != NULL) {
        fprintf(output, "%s at line %d, column %d: %s\n",
                type_names[err->type], err->line, err->column, err->message);
        err = err->next;
    }
    
    if (list->count > 0) {
        fprintf(output, "\n%d error(s) found.\n", list->count);
    }
}

int error_has_errors(ErrorList *list) {
    return list != NULL && list->count > 0;
}
