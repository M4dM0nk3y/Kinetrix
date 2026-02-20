/* Kinetrix Parser - Builds AST from source code
 * Multi-pass architecture with proper error recovery
 */

#ifndef KINETRIX_PARSER_H
#define KINETRIX_PARSER_H

#include "ast.h"
#include "error.h"
#include "symbol_table.h"

// ============================================================================
// LEXER (Tokenizer)
// ============================================================================

typedef enum {
    TOK_EOF, TOK_ID, TOK_NUMBER, TOK_STRING_LIT,
    TOK_PROGRAM, TOK_LBRACE, TOK_RBRACE, TOK_SEMI, TOK_LPAREN, TOK_RPAREN, TOK_COMMA,
    TOK_TURN, TOK_ON, TOK_OFF, TOK_WAIT, TOK_SET, TOK_TO, TOK_MAKE, TOK_CHANGE, TOK_BY,
    TOK_FOREVER, TOK_IF, TOK_ELSE, TOK_READ, TOK_ANALOG, TOK_PULSE, TOK_SERVO, TOK_PRINT, TOK_DEF, TOK_REPEAT,
    TOK_TONE, TOK_NOTONE, TOK_FREQ, TOK_RETURN,
    TOK_WHILE, TOK_BREAK,
    TOK_ARRAY, TOK_SIZE, TOK_INDEX, TOK_OF, TOK_LBRACKET, TOK_RBRACKET,
    TOK_SERIAL,
    TOK_I2C, TOK_BEGIN, TOK_START, TOK_SEND, TOK_STOP,
    TOK_PIN, TOK_IS, TOK_HIGH, TOK_LOW, TOK_VAR,
    TOK_SIN, TOK_COS, TOK_TAN, TOK_SQRT,
    TOK_ACOS, TOK_ASIN, TOK_ATAN, TOK_ATAN2,
    TOK_ASSIGN, TOK_EQ, TOK_LT, TOK_GT, TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_AND, TOK_OR,
    TOK_LOOP, TOK_INCLUDE, TOK_EXTERN
} TokenType;

typedef struct {
    TokenType type;
    char value[256];
    int line;
    int column;
} Token;

typedef struct {
    FILE *file;
    int current_char;
    int line;
    int column;
    Token current_token;
    ErrorList *errors;
} Lexer;

Lexer* lexer_create(FILE *file, ErrorList *errors);
void lexer_free(Lexer *lexer);
void lexer_next_token(Lexer *lexer);
Token lexer_peek(Lexer *lexer);

// ============================================================================
// PARSER
// ============================================================================

typedef struct {
    Lexer *lexer;
    SymbolTable *symbols;
    ErrorList *errors;
    int in_loop;  // For break statement validation
    int in_function;  // For return statement validation
} Parser;

Parser* parser_create(FILE *file, ErrorList *errors);
void parser_free(Parser *parser);

// Main parsing function
ASTNode* parser_parse(Parser *parser);

// Helper functions
int parser_expect(Parser *parser, TokenType type);
int parser_match(Parser *parser, TokenType type);

#endif // KINETRIX_PARSER_H
