/* Kinetrix Parser Implementation
 * Lexer + Recursive Descent Parser → AST
 */

#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

// ============================================================================
// LEXER IMPLEMENTATION
// ============================================================================

Lexer* lexer_create(FILE *file, ErrorList *errors) {
    Lexer *lexer = malloc(sizeof(Lexer));
    lexer->file = file;
    lexer->current_char = fgetc(file);
    lexer->line = 1;
    lexer->column = 1;
    lexer->errors = errors;
    
    // Read first token
    lexer_next_token(lexer);
    
    return lexer;
}

void lexer_free(Lexer *lexer) {
    free(lexer);
}

static void lexer_advance(Lexer *lexer) {
    if (lexer->current_char == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    lexer->current_char = fgetc(lexer->file);
}

static void lexer_skip_whitespace(Lexer *lexer) {
    while (isspace(lexer->current_char)) {
        lexer_advance(lexer);
    }
}

static void lexer_skip_comment(Lexer *lexer) {
    if (lexer->current_char == '#') {
        while (lexer->current_char != '\n' && lexer->current_char != EOF) {
            lexer_advance(lexer);
        }
    }
}

void lexer_next_token(Lexer *lexer) {
    // Skip whitespace and comments FIRST
    do {
        lexer_skip_whitespace(lexer);
        lexer_skip_comment(lexer);
    } while (isspace(lexer->current_char) || lexer->current_char == '#');
    
    lexer->current_token.line = lexer->line;
    lexer->current_token.column = lexer->column;
    
    if (lexer->current_char == EOF) {
        lexer->current_token.type = TOK_EOF;
        strcpy(lexer->current_token.value, "");
        return;
    }
    
    // String literals
    if (lexer->current_char == '"') {
        int i = 0;
        lexer_advance(lexer);
        while (lexer->current_char != '"' && lexer->current_char != EOF && i < 255) {
            lexer->current_token.value[i++] = lexer->current_char;
            lexer_advance(lexer);
        }
        lexer->current_token.value[i] = '\0';
        if (lexer->current_char == '"') lexer_advance(lexer);
        lexer->current_token.type = TOK_STRING_LIT;
        return;
    }
    
    // Single-character tokens
    if (lexer->current_char == '{') {
        lexer->current_token.type = TOK_LBRACE;
        strcpy(lexer->current_token.value, "{");
        lexer_advance(lexer);
        return;
    }
    if (lexer->current_char == '}') {
        lexer->current_token.type = TOK_RBRACE;
        strcpy(lexer->current_token.value, "}");
        lexer_advance(lexer);
        return;
    }
    if (lexer->current_char == '(') {
        lexer->current_token.type = TOK_LPAREN;
        strcpy(lexer->current_token.value, "(");
        lexer_advance(lexer);
        return;
    }
    if (lexer->current_char == ')') {
        lexer->current_token.type = TOK_RPAREN;
        strcpy(lexer->current_token.value, ")");
        lexer_advance(lexer);
        return;
    }
    if (lexer->current_char == '[') {
        lexer->current_token.type = TOK_LBRACKET;
        strcpy(lexer->current_token.value, "[");
        lexer_advance(lexer);
        return;
    }
    if (lexer->current_char == ']') {
        lexer->current_token.type = TOK_RBRACKET;
        strcpy(lexer->current_token.value, "]");
        lexer_advance(lexer);
        return;
    }
    if (lexer->current_char == ',') {
        lexer->current_token.type = TOK_COMMA;
        strcpy(lexer->current_token.value, ",");
        lexer_advance(lexer);
        return;
    }
    if (lexer->current_char == '+') {
        lexer->current_token.type = TOK_PLUS;
        strcpy(lexer->current_token.value, "+");
        lexer_advance(lexer);
        return;
    }
    if (lexer->current_char == '-') {
        lexer->current_token.type = TOK_MINUS;
        strcpy(lexer->current_token.value, "-");
        lexer_advance(lexer);
        return;
    }
    if (lexer->current_char == '*') {
        lexer->current_token.type = TOK_STAR;
        strcpy(lexer->current_token.value, "*");
        lexer_advance(lexer);
        return;
    }
    if (lexer->current_char == '/') {
        lexer->current_token.type = TOK_SLASH;
        strcpy(lexer->current_token.value, "/");
        lexer_advance(lexer);
        return;
    }
    if (lexer->current_char == '%') {
        lexer->current_token.type = TOK_STAR;  // Use STAR for modulo for now
        strcpy(lexer->current_token.value, "%");
        lexer_advance(lexer);
        return;
    }
    if (lexer->current_char == '=') {
        lexer_advance(lexer);
        if (lexer->current_char == '=') {
            lexer->current_token.type = TOK_EQ;
            strcpy(lexer->current_token.value, "==");
            lexer_advance(lexer);
        } else {
            lexer->current_token.type = TOK_ASSIGN;
            strcpy(lexer->current_token.value, "=");
        }
        return;
    }
    // != operator
    if (lexer->current_char == '!') {
        lexer_advance(lexer);
        if (lexer->current_char == '=') {
            lexer->current_token.type = TOK_EQ;  // Reuse EQ slot, value distinguishes
            strcpy(lexer->current_token.value, "!=");
            lexer_advance(lexer);
        }
        return;
    }
    if (lexer->current_char == '<') {
        lexer_advance(lexer);
        if (lexer->current_char == '=') {
            lexer->current_token.type = TOK_LT;  // Reuse LT, value distinguishes
            strcpy(lexer->current_token.value, "<=");
            lexer_advance(lexer);
        } else {
            lexer->current_token.type = TOK_LT;
            strcpy(lexer->current_token.value, "<");
        }
        return;
    }
    if (lexer->current_char == '>') {
        lexer_advance(lexer);
        if (lexer->current_char == '=') {
            lexer->current_token.type = TOK_GT;  // Reuse GT, value distinguishes
            strcpy(lexer->current_token.value, ">=");
            lexer_advance(lexer);
        } else {
            lexer->current_token.type = TOK_GT;
            strcpy(lexer->current_token.value, ">");
        }
        return;
    }
    
    // Numbers
    if (isdigit(lexer->current_char)) {
        int i = 0;
        while ((isdigit(lexer->current_char) || lexer->current_char == '.') && i < 255) {
            lexer->current_token.value[i++] = lexer->current_char;
            lexer_advance(lexer);
        }
        lexer->current_token.value[i] = '\0';
        lexer->current_token.type = TOK_NUMBER;
        return;
    }
    
    // Identifiers and keywords
    if (isalpha(lexer->current_char) || lexer->current_char == '_') {
        int i = 0;
        while ((isalnum(lexer->current_char) || lexer->current_char == '_') && i < 255) {
            lexer->current_token.value[i++] = lexer->current_char;
            lexer_advance(lexer);
        }
        lexer->current_token.value[i] = '\0';
        
        // Check for keywords
        if (!strcmp(lexer->current_token.value, "program")) lexer->current_token.type = TOK_PROGRAM;
        else if (!strcmp(lexer->current_token.value, "extern")) lexer->current_token.type = TOK_EXTERN;
        else if (!strcmp(lexer->current_token.value, "def")) lexer->current_token.type = TOK_DEF;
        else if (!strcmp(lexer->current_token.value, "repeat")) lexer->current_token.type = TOK_REPEAT;
        else if (!strcmp(lexer->current_token.value, "loop")) lexer->current_token.type = TOK_LOOP;
        else if (!strcmp(lexer->current_token.value, "turn")) lexer->current_token.type = TOK_TURN;
        else if (!strcmp(lexer->current_token.value, "on")) lexer->current_token.type = TOK_ON;
        else if (!strcmp(lexer->current_token.value, "off")) lexer->current_token.type = TOK_OFF;
        else if (!strcmp(lexer->current_token.value, "wait")) lexer->current_token.type = TOK_WAIT;
        else if (!strcmp(lexer->current_token.value, "forever")) lexer->current_token.type = TOK_FOREVER;
        else if (!strcmp(lexer->current_token.value, "if")) lexer->current_token.type = TOK_IF;
        else if (!strcmp(lexer->current_token.value, "else")) lexer->current_token.type = TOK_ELSE;
        else if (!strcmp(lexer->current_token.value, "make")) lexer->current_token.type = TOK_MAKE;
        else if (!strcmp(lexer->current_token.value, "var")) lexer->current_token.type = TOK_VAR;
        else if (!strcmp(lexer->current_token.value, "set")) lexer->current_token.type = TOK_SET;
        else if (!strcmp(lexer->current_token.value, "change")) lexer->current_token.type = TOK_CHANGE;
        else if (!strcmp(lexer->current_token.value, "to")) lexer->current_token.type = TOK_TO;
        else if (!strcmp(lexer->current_token.value, "by")) lexer->current_token.type = TOK_BY;
        else if (!strcmp(lexer->current_token.value, "read")) lexer->current_token.type = TOK_READ;
        else if (!strcmp(lexer->current_token.value, "analog")) lexer->current_token.type = TOK_ANALOG;
        else if (!strcmp(lexer->current_token.value, "pulse")) lexer->current_token.type = TOK_PULSE;
        else if (!strcmp(lexer->current_token.value, "serial")) lexer->current_token.type = TOK_SERIAL;
        else if (!strcmp(lexer->current_token.value, "servo")) lexer->current_token.type = TOK_SERVO;
        else if (!strcmp(lexer->current_token.value, "print")) lexer->current_token.type = TOK_PRINT;
        else if (!strcmp(lexer->current_token.value, "tone")) lexer->current_token.type = TOK_TONE;
        else if (!strcmp(lexer->current_token.value, "notone")) lexer->current_token.type = TOK_NOTONE;
        else if (!strcmp(lexer->current_token.value, "return")) lexer->current_token.type = TOK_RETURN;
        else if (!strcmp(lexer->current_token.value, "while")) lexer->current_token.type = TOK_WHILE;
        else if (!strcmp(lexer->current_token.value, "break")) lexer->current_token.type = TOK_BREAK;
        else if (!strcmp(lexer->current_token.value, "array")) lexer->current_token.type = TOK_ARRAY;
        else if (!strcmp(lexer->current_token.value, "size")) lexer->current_token.type = TOK_SIZE;
        else if (!strcmp(lexer->current_token.value, "index")) lexer->current_token.type = TOK_INDEX;
        else if (!strcmp(lexer->current_token.value, "of")) lexer->current_token.type = TOK_OF;
        else if (!strcmp(lexer->current_token.value, "i2c")) lexer->current_token.type = TOK_I2C;
        else if (!strcmp(lexer->current_token.value, "begin")) lexer->current_token.type = TOK_BEGIN;
        else if (!strcmp(lexer->current_token.value, "start")) lexer->current_token.type = TOK_START;
        else if (!strcmp(lexer->current_token.value, "send")) lexer->current_token.type = TOK_SEND;
        else if (!strcmp(lexer->current_token.value, "stop")) lexer->current_token.type = TOK_STOP;
        else if (!strcmp(lexer->current_token.value, "pin")) lexer->current_token.type = TOK_PIN;
        else if (!strcmp(lexer->current_token.value, "is")) lexer->current_token.type = TOK_IS;
        else if (!strcmp(lexer->current_token.value, "high")) lexer->current_token.type = TOK_HIGH;
        else if (!strcmp(lexer->current_token.value, "low")) lexer->current_token.type = TOK_LOW;
        else if (!strcmp(lexer->current_token.value, "sin")) lexer->current_token.type = TOK_SIN;
        else if (!strcmp(lexer->current_token.value, "cos")) lexer->current_token.type = TOK_COS;
        else if (!strcmp(lexer->current_token.value, "tan")) lexer->current_token.type = TOK_TAN;
        else if (!strcmp(lexer->current_token.value, "and")) lexer->current_token.type = TOK_AND;
        else if (!strcmp(lexer->current_token.value, "or")) lexer->current_token.type = TOK_OR;

        else if (!strcmp(lexer->current_token.value, "sqrt")) lexer->current_token.type = TOK_SQRT;
        else if (!strcmp(lexer->current_token.value, "asin")) lexer->current_token.type = TOK_ASIN;
        else if (!strcmp(lexer->current_token.value, "acos")) lexer->current_token.type = TOK_ACOS;
        else if (!strcmp(lexer->current_token.value, "atan")) lexer->current_token.type = TOK_ATAN;
        else if (!strcmp(lexer->current_token.value, "atan2")) lexer->current_token.type = TOK_ATAN2;
        else if (!strcmp(lexer->current_token.value, "include")) lexer->current_token.type = TOK_INCLUDE;
        // New keywords for 100% completion
        else if (!strcmp(lexer->current_token.value, "not")) lexer->current_token.type = TOK_ID;  // handled in parser
        else if (!strcmp(lexer->current_token.value, "for")) lexer->current_token.type = TOK_ID;   // handled in parser
        else if (!strcmp(lexer->current_token.value, "from")) lexer->current_token.type = TOK_ID;  // handled in parser
        else if (!strcmp(lexer->current_token.value, "const")) lexer->current_token.type = TOK_ID; // handled in parser
        else if (!strcmp(lexer->current_token.value, "wait_us")) lexer->current_token.type = TOK_ID; // handled in parser
        else if (!strcmp(lexer->current_token.value, "map")) lexer->current_token.type = TOK_ID;   // handled in parser
        else if (!strcmp(lexer->current_token.value, "constrain")) lexer->current_token.type = TOK_ID; // handled in parser
        else if (!strcmp(lexer->current_token.value, "abs")) lexer->current_token.type = TOK_ID;   // handled in parser
        else if (!strcmp(lexer->current_token.value, "true")) { lexer->current_token.type = TOK_NUMBER; strcpy(lexer->current_token.value, "1"); }
        else if (!strcmp(lexer->current_token.value, "false")) { lexer->current_token.type = TOK_NUMBER; strcpy(lexer->current_token.value, "0"); }
        else lexer->current_token.type = TOK_ID;
        
        return;
    }
    
    // Unknown character
    char msg[256];
    snprintf(msg, sizeof(msg), "Unexpected character: '%c'", lexer->current_char);
    error_report(lexer->errors, ERROR_LEXICAL, lexer->line, lexer->column, "%s", msg);
    lexer_advance(lexer);
    lexer_next_token(lexer);  // Try next token
}

Token lexer_peek(Lexer *lexer) {
    return lexer->current_token;
}

// ============================================================================
// PARSER IMPLEMENTATION
// ============================================================================

Parser* parser_create(FILE *file, ErrorList *errors) {
    Parser *parser = malloc(sizeof(Parser));
    parser->lexer = lexer_create(file, errors);
    parser->symbols = symbol_table_create();
    parser->errors = errors;
    parser->in_loop = 0;
    parser->in_function = 0;
    return parser;
}

void parser_free(Parser *parser) {
    lexer_free(parser->lexer);
    symbol_table_free(parser->symbols);
    free(parser);
}

int parser_match(Parser *parser, TokenType type) {
    return parser->lexer->current_token.type == type;
}

int parser_expect(Parser *parser, TokenType type) {
    if (parser_match(parser, type)) {
        lexer_next_token(parser->lexer);
        return 1;
    }
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Expected token type %d, got %d", type, parser->lexer->current_token.type);
    error_report(parser->errors, ERROR_SYNTAX, 
              parser->lexer->current_token.line,
              parser->lexer->current_token.column, "%s", msg);
    return 0;
}

// Forward declarations
static ASTNode* parse_primary(Parser *parser);
static ASTNode* parse_arithmetic(Parser *parser);
static ASTNode* parse_comparison(Parser *parser);
static ASTNode* parse_expression(Parser *parser);
static ASTNode* parse_statement(Parser *parser);
static ASTNode* parse_block(Parser *parser);


// Parse primary expression
static ASTNode* parse_primary(Parser *parser) {
    Token tok = lexer_peek(parser->lexer);
    
    // Number
    if (parser_match(parser, TOK_NUMBER)) {
        double value = atof(tok.value);
        lexer_next_token(parser->lexer);
        return ast_number(value);
    }
    
    // String
    if (parser_match(parser, TOK_STRING_LIT)) {
        ASTNode *node = ast_string(tok.value);
        lexer_next_token(parser->lexer);
        return node;
    }
    
    // Identifier or function call
    if (parser_match(parser, TOK_ID)) {
        char *name = strdup(tok.value);
        lexer_next_token(parser->lexer);
        
        // Function call
        if (parser_match(parser, TOK_LPAREN)) {
            lexer_next_token(parser->lexer);
            
            ASTNode **args = NULL;
            int arg_count = 0;
            
            if (!parser_match(parser, TOK_RPAREN)) {
                args = malloc(sizeof(ASTNode*) * 10);  // Max 10 args
                args[arg_count++] = parse_expression(parser);
                
                while (parser_match(parser, TOK_COMMA)) {
                    lexer_next_token(parser->lexer);
                    args[arg_count++] = parse_expression(parser);
                }
            }
            
            parser_expect(parser, TOK_RPAREN);
            return ast_call(name, args, arg_count);
        }
        
        // Array access
        if (parser_match(parser, TOK_LBRACKET)) {
            lexer_next_token(parser->lexer);
            ASTNode *index = parse_expression(parser);
            parser_expect(parser, TOK_RBRACKET);
            return ast_array_access(ast_identifier(name), index);
        }
        
        // Simple identifier
        return ast_identifier(name);
    }
    
    // Parenthesized expression
    if (parser_match(parser, TOK_LPAREN)) {
        lexer_next_token(parser->lexer);
        ASTNode *expr = parse_expression(parser);
        parser_expect(parser, TOK_RPAREN);
        return expr;
    }
    
    // Math functions
    if (parser_match(parser, TOK_SIN) || parser_match(parser, TOK_COS) || 
        parser_match(parser, TOK_TAN) || parser_match(parser, TOK_SQRT) ||
        parser_match(parser, TOK_ASIN) || parser_match(parser, TOK_ACOS) ||
        parser_match(parser, TOK_ATAN) || parser_match(parser, TOK_ATAN2)) {
        
        MathFunc func = MATH_SIN;
        if (parser_match(parser, TOK_COS)) func = MATH_COS;
        else if (parser_match(parser, TOK_TAN)) func = MATH_TAN;
        else if (parser_match(parser, TOK_SQRT)) func = MATH_SQRT;
        else if (parser_match(parser, TOK_ASIN)) func = MATH_ASIN;
        else if (parser_match(parser, TOK_ACOS)) func = MATH_ACOS;
        else if (parser_match(parser, TOK_ATAN)) func = MATH_ATAN;
        else if (parser_match(parser, TOK_ATAN2)) func = MATH_ATAN2;
        
        lexer_next_token(parser->lexer);
        parser_expect(parser, TOK_LPAREN);
        
        ASTNode *arg1 = parse_expression(parser);
        ASTNode *arg2 = NULL;
        
        if (func == MATH_ATAN2) {
            parser_expect(parser, TOK_COMMA);
            arg2 = parse_expression(parser);
        }
        
        parser_expect(parser, TOK_RPAREN);
        return ast_math_func(func, arg1, arg2);
    }
    
    // Read operations
    if (parser_match(parser, TOK_READ)) {
        lexer_next_token(parser->lexer);
        
        if (parser_match(parser, TOK_ANALOG)) {
            lexer_next_token(parser->lexer);
            parser_expect(parser, TOK_PIN);
            ASTNode *pin = parse_expression(parser);
            return ast_analog_read(pin);
        }
        
        if (parser_match(parser, TOK_PULSE)) {
            lexer_next_token(parser->lexer);
            parser_expect(parser, TOK_PIN);
            ASTNode *pin = parse_expression(parser);
            return ast_pulse_read(pin);
        }
        
        if (parser_match(parser, TOK_I2C)) {
            lexer_next_token(parser->lexer);
            ASTNode *addr = parse_expression(parser);
            return ast_i2c_read(addr);
        }
        
        if (parser_match(parser, TOK_SERIAL)) {
            lexer_next_token(parser->lexer);
            return ast_number(0);  // Placeholder
        }
        
        // Default: digital read
        parser_expect(parser, TOK_PIN);
        ASTNode *pin = parse_expression(parser);
        return ast_gpio_read(pin);
    }
    
    // Special built-in functions: map, constrain, abs, not
    if (parser_match(parser, TOK_ID)) {
        const char *name = parser->lexer->current_token.value;
        
        // map(value, fromLow, fromHigh, toLow, toHigh)
        if (strcmp(name, "map") == 0) {
            lexer_next_token(parser->lexer);
            parser_expect(parser, TOK_LPAREN);
            ASTNode *v   = parse_expression(parser); parser_expect(parser, TOK_COMMA);
            ASTNode *fl  = parse_expression(parser); parser_expect(parser, TOK_COMMA);
            ASTNode *fh  = parse_expression(parser); parser_expect(parser, TOK_COMMA);
            ASTNode *tl  = parse_expression(parser); parser_expect(parser, TOK_COMMA);
            ASTNode *th  = parse_expression(parser);
            parser_expect(parser, TOK_RPAREN);
            // map(v, fl, fh, tl, th) → generate as call node
            ASTNode **args = malloc(sizeof(ASTNode*) * 5);
            args[0]=v; args[1]=fl; args[2]=fh; args[3]=tl; args[4]=th;
            return ast_call("map", args, 5);
        }
        
        // constrain(value, min, max)
        if (strcmp(name, "constrain") == 0) {
            lexer_next_token(parser->lexer);
            parser_expect(parser, TOK_LPAREN);
            ASTNode *v  = parse_expression(parser); parser_expect(parser, TOK_COMMA);
            ASTNode *mn = parse_expression(parser); parser_expect(parser, TOK_COMMA);
            ASTNode *mx = parse_expression(parser);
            parser_expect(parser, TOK_RPAREN);
            ASTNode **args = malloc(sizeof(ASTNode*) * 3);
            args[0]=v; args[1]=mn; args[2]=mx;
            return ast_call("constrain", args, 3);
        }
        
        // abs(value)
        if (strcmp(name, "abs") == 0) {
            lexer_next_token(parser->lexer);
            parser_expect(parser, TOK_LPAREN);
            ASTNode *v = parse_expression(parser);
            parser_expect(parser, TOK_RPAREN);
            ASTNode **args = malloc(sizeof(ASTNode*) * 1);
            args[0] = v;
            return ast_call("abs", args, 1);
        }
        
        // not(expr) or not expr
        if (strcmp(name, "not") == 0) {
            lexer_next_token(parser->lexer);
            ASTNode *operand = parse_primary(parser);
            return ast_unary_op(OP_NOT, operand);
        }
        
        // random(min, max)
        if (strcmp(name, "random") == 0) {
            lexer_next_token(parser->lexer);
            parser_expect(parser, TOK_LPAREN);
            ASTNode *mn = parse_expression(parser); parser_expect(parser, TOK_COMMA);
            ASTNode *mx = parse_expression(parser);
            parser_expect(parser, TOK_RPAREN);
            ASTNode **args = malloc(sizeof(ASTNode*) * 2);
            args[0]=mn; args[1]=mx;
            return ast_call("random", args, 2);
        }
        
        // min(a, b) and max(a, b)
        if (strcmp(name, "min") == 0 || strcmp(name, "max") == 0) {
            char fname[8]; strcpy(fname, name);
            lexer_next_token(parser->lexer);
            parser_expect(parser, TOK_LPAREN);
            ASTNode *a = parse_expression(parser); parser_expect(parser, TOK_COMMA);
            ASTNode *b = parse_expression(parser);
            parser_expect(parser, TOK_RPAREN);
            ASTNode **args = malloc(sizeof(ASTNode*) * 2);
            args[0]=a; args[1]=b;
            return ast_call(fname, args, 2);
        }
    }
    
    error_report(parser->errors, ERROR_SYNTAX, tok.line, tok.column, "Unexpected token in expression");
    return ast_number(0);  // Error recovery
}


// Level 1: Arithmetic (highest precedence): + - * / %
static ASTNode* parse_arithmetic(Parser *parser) {
    ASTNode *left = parse_primary(parser);
    
    while (parser_match(parser, TOK_PLUS) || parser_match(parser, TOK_MINUS) ||
           parser_match(parser, TOK_STAR) || parser_match(parser, TOK_SLASH)) {
        
        Operator op = OP_ADD;
        if (parser_match(parser, TOK_MINUS)) op = OP_SUB;
        else if (parser_match(parser, TOK_STAR)) {
            if (strcmp(parser->lexer->current_token.value, "%") == 0) {
                op = OP_MOD;
            } else {
                op = OP_MUL;
            }
        }
        else if (parser_match(parser, TOK_SLASH)) op = OP_DIV;
        
        lexer_next_token(parser->lexer);
        ASTNode *right = parse_primary(parser);
        left = ast_binary_op(op, left, right);
    }
    
    return left;
}

// Level 2: Comparison (medium precedence): > < >= <= == != is
static ASTNode* parse_comparison(Parser *parser) {
    ASTNode *left = parse_arithmetic(parser);
    
    while (parser_match(parser, TOK_EQ) || parser_match(parser, TOK_LT) ||
           parser_match(parser, TOK_GT) || parser_match(parser, TOK_IS)) {
        
        Operator op = OP_EQ;
        const char *val = parser->lexer->current_token.value;
        if      (strcmp(val, "!=") == 0) op = OP_NEQ;
        else if (strcmp(val, "<=") == 0) op = OP_LTE;
        else if (strcmp(val, ">=") == 0) op = OP_GTE;
        else if (parser_match(parser, TOK_LT)) op = OP_LT;
        else if (parser_match(parser, TOK_GT)) op = OP_GT;
        
        lexer_next_token(parser->lexer);
        ASTNode *right = parse_arithmetic(parser);
        left = ast_binary_op(op, left, right);
    }
    
    return left;
}

// Level 3: Logical (lowest precedence): and or
static ASTNode* parse_expression(Parser *parser) {
    ASTNode *left = parse_comparison(parser);
    
    while (parser_match(parser, TOK_AND) || parser_match(parser, TOK_OR)) {
        
        Operator op = OP_AND;
        if (parser_match(parser, TOK_OR)) op = OP_OR;
        
        lexer_next_token(parser->lexer);
        ASTNode *right = parse_comparison(parser);
        left = ast_binary_op(op, left, right);
    }
    
    return left;
}


// Parse statement
static ASTNode* parse_statement(Parser *parser) {
    // Variable declaration: make var x = 5
    if (parser_match(parser, TOK_MAKE)) {
        lexer_next_token(parser->lexer);
        parser_expect(parser, TOK_VAR);
        
        Token name_tok = lexer_peek(parser->lexer);
        parser_expect(parser, TOK_ID);
        
        ASTNode *init = NULL;
        if (parser_match(parser, TOK_ASSIGN)) {
            lexer_next_token(parser->lexer);
            init = parse_expression(parser);
        }
        
        return ast_var_decl(name_tok.value, NULL, init);
    }
    
    // Assignment: set x to 5
    if (parser_match(parser, TOK_SET)) {
        lexer_next_token(parser->lexer);
        
        ASTNode *target;
        if (parser_match(parser, TOK_PIN)) {
            lexer_next_token(parser->lexer);
            ASTNode *pin = parse_expression(parser);
            parser_expect(parser, TOK_TO);
            ASTNode *value = parse_expression(parser);
            return ast_analog_write(pin, value);
        } else if (parser_match(parser, TOK_INDEX)) {
            lexer_next_token(parser->lexer);
            Token arr_tok = lexer_peek(parser->lexer);
            parser_expect(parser, TOK_ID);
            parser_expect(parser, TOK_OF);
            ASTNode *index = parse_expression(parser);
            target = ast_array_access(ast_identifier(arr_tok.value), index);
        } else {
            Token var_tok = lexer_peek(parser->lexer);
            parser_expect(parser, TOK_ID);
            target = ast_identifier(var_tok.value);
        }
        
        parser_expect(parser, TOK_TO);
        ASTNode *value = parse_expression(parser);
        return ast_assignment(target, value);
    }
    
    // Change statement: change x by 5
    if (parser_match(parser, TOK_CHANGE)) {
        lexer_next_token(parser->lexer);
        
        Token var_tok = lexer_peek(parser->lexer);
        parser_expect(parser, TOK_ID);
        parser_expect(parser, TOK_BY);
        
        ASTNode *delta = parse_expression(parser);
        
        // Generate: x = x + delta
        ASTNode *var_node = ast_identifier(var_tok.value);
        ASTNode *add_expr = ast_binary_op(OP_ADD, ast_identifier(var_tok.value), delta);
        return ast_assignment(var_node, add_expr);
    }
    
    // GPIO: turn on/off pin X
    if (parser_match(parser, TOK_TURN)) {
        lexer_next_token(parser->lexer);
        int is_on = parser_match(parser, TOK_ON);
        lexer_next_token(parser->lexer);
        parser_expect(parser, TOK_PIN);
        ASTNode *pin = parse_expression(parser);
        return ast_gpio_write(pin, ast_number(is_on ? 1 : 0));
    }
    
    // Wait
    if (parser_match(parser, TOK_WAIT)) {
        lexer_next_token(parser->lexer);
        ASTNode *duration = parse_expression(parser);
        return ast_wait(duration);
    }
    
    // Print
    if (parser_match(parser, TOK_PRINT)) {
        lexer_next_token(parser->lexer);
        ASTNode *value = parse_expression(parser);
        ASTNode *print_node = malloc(sizeof(ASTNode));
        print_node->type = NODE_PRINT;
        print_node->data.unary.child = value;
        return print_node;
    }
    
    // I2C operations
    if (parser_match(parser, TOK_I2C)) {
        lexer_next_token(parser->lexer);
        
        if (parser_match(parser, TOK_BEGIN)) {
            lexer_next_token(parser->lexer);
            return ast_i2c_begin();
        }
        if (parser_match(parser, TOK_START)) {
            lexer_next_token(parser->lexer);
            ASTNode *addr = parse_expression(parser);
            return ast_i2c_start(addr);
        }
        if (parser_match(parser, TOK_SEND)) {
            lexer_next_token(parser->lexer);
            ASTNode *data = parse_expression(parser);
            return ast_i2c_send(data);
        }
        if (parser_match(parser, TOK_STOP)) {
            lexer_next_token(parser->lexer);
            return ast_i2c_stop();
        }
    }
    
    // If statement (with else-if support)
    if (parser_match(parser, TOK_IF)) {
        lexer_next_token(parser->lexer);
        ASTNode *condition = parse_expression(parser);
        parser_expect(parser, TOK_LBRACE);
        ASTNode *then_block = parse_block(parser);
        parser_expect(parser, TOK_RBRACE);
        
        ASTNode *else_block = NULL;
        if (parser_match(parser, TOK_ELSE)) {
            lexer_next_token(parser->lexer);
            // else if: parse the next if as the else block
            if (parser_match(parser, TOK_IF)) {
                // Recursively parse the else-if as a nested if statement
                ASTNode *elif_stmt = parse_statement(parser);
                // Wrap it in a block
                ASTNode **stmts = malloc(sizeof(ASTNode*));
                stmts[0] = elif_stmt;
                else_block = ast_block(stmts, 1);
            } else {
                parser_expect(parser, TOK_LBRACE);
                else_block = parse_block(parser);
                parser_expect(parser, TOK_RBRACE);
            }
        }
        
        return ast_if(condition, then_block, else_block);
    }
    
    // While loop
    if (parser_match(parser, TOK_WHILE)) {
        lexer_next_token(parser->lexer);
        ASTNode *condition = parse_expression(parser);
        parser_expect(parser, TOK_LBRACE);
        parser->in_loop++;
        ASTNode *body = parse_block(parser);
        parser->in_loop--;
        parser_expect(parser, TOK_RBRACE);
        return ast_while(condition, body);
    }
    
    // Extern Function definition: extern "C++" def name(param1)
    if (parser_match(parser, TOK_EXTERN)) {
        lexer_next_token(parser->lexer);
        
        Token lang_tok = lexer_peek(parser->lexer);
        parser_expect(parser, TOK_STRING_LIT);
        
        parser_expect(parser, TOK_DEF);
        Token name_tok = lexer_peek(parser->lexer);
        parser_expect(parser, TOK_ID);
        parser_expect(parser, TOK_LPAREN);
        
        char **param_names = malloc(sizeof(char*) * 16);
        int param_count = 0;
        
        while (!parser_match(parser, TOK_RPAREN) && !parser_match(parser, TOK_EOF)) {
            Token param_tok = lexer_peek(parser->lexer);
            parser_expect(parser, TOK_ID);
            param_names[param_count] = strdup(param_tok.value);
            param_count++;
            if (parser_match(parser, TOK_COMMA)) {
                lexer_next_token(parser->lexer);
            }
        }
        parser_expect(parser, TOK_RPAREN);
        
        // Build param types (all float for simplicity)
        Type **param_types = malloc(sizeof(Type*) * param_count);
        for (int i = 0; i < param_count; i++) param_types[i] = type_float();
        
        return ast_extern_function_def(name_tok.value, param_names, param_types, param_count, type_void(), lang_tok.value);
    }
    
    // Function definition: def name(param1, param2) { }
    if (parser_match(parser, TOK_DEF)) {
        lexer_next_token(parser->lexer);
        Token name_tok = lexer_peek(parser->lexer);
        parser_expect(parser, TOK_ID);
        parser_expect(parser, TOK_LPAREN);
        
        char **param_names = malloc(sizeof(char*) * 16);
        int param_count = 0;
        
        while (!parser_match(parser, TOK_RPAREN) && !parser_match(parser, TOK_EOF)) {
            Token param_tok = lexer_peek(parser->lexer);
            parser_expect(parser, TOK_ID);
            param_names[param_count] = strdup(param_tok.value);
            param_count++;
            if (parser_match(parser, TOK_COMMA)) {
                lexer_next_token(parser->lexer);
            }
        }
        parser_expect(parser, TOK_RPAREN);
        parser_expect(parser, TOK_LBRACE);
        parser->in_function++;
        ASTNode *body = parse_block(parser);
        parser->in_function--;
        parser_expect(parser, TOK_RBRACE);
        
        // Build param types (all float for simplicity)
        Type **param_types = malloc(sizeof(Type*) * param_count);
        for (int i = 0; i < param_count; i++) param_types[i] = type_float();
        
        return ast_function_def(name_tok.value, param_names, param_types, param_count, type_void(), body);
    }
    
    // Loop forever
    if (parser_match(parser, TOK_LOOP)) {
        lexer_next_token(parser->lexer);
        parser_expect(parser, TOK_FOREVER);
        parser_expect(parser, TOK_LBRACE);
        parser->in_loop++;
        ASTNode *body = parse_block(parser);
        parser->in_loop--;
        parser_expect(parser, TOK_RBRACE);
        return ast_forever(body);
    }
    
    // Repeat
    if (parser_match(parser, TOK_REPEAT)) {
        lexer_next_token(parser->lexer);
        ASTNode *count = parse_expression(parser);
        parser_expect(parser, TOK_LBRACE);
        parser->in_loop++;
        ASTNode *body = parse_block(parser);
        parser->in_loop--;
        parser_expect(parser, TOK_RBRACE);
        return ast_repeat(count, body);
    }
    
    // Return
    if (parser_match(parser, TOK_RETURN)) {
        lexer_next_token(parser->lexer);
        ASTNode *value = NULL;
        if (!parser_match(parser, TOK_RBRACE)) {
            value = parse_expression(parser);
        }
        return ast_return(value);
    }
    
    // Break
    if (parser_match(parser, TOK_BREAK)) {
        lexer_next_token(parser->lexer);
        return ast_break();
    }
    
    // Handle special identifier-based statements: for, const, wait_us
    if (parser_match(parser, TOK_ID)) {
        const char *kw = parser->lexer->current_token.value;
        
        // for i from 0 to 10 { }
        if (strcmp(kw, "for") == 0) {
            lexer_next_token(parser->lexer);  // consume 'for'
            Token var_tok = lexer_peek(parser->lexer);
            parser_expect(parser, TOK_ID);    // variable name
            // expect 'from'
            if (parser_match(parser, TOK_ID) && strcmp(parser->lexer->current_token.value, "from") == 0) {
                lexer_next_token(parser->lexer);
            }
            ASTNode *start = parse_expression(parser);
            // expect 'to'
            if (parser_match(parser, TOK_TO) || (parser_match(parser, TOK_ID) && strcmp(parser->lexer->current_token.value, "to") == 0)) {
                lexer_next_token(parser->lexer);
            }
            ASTNode *end = parse_expression(parser);
            parser_expect(parser, TOK_LBRACE);
            parser->in_loop++;
            ASTNode *body = parse_block(parser);
            parser->in_loop--;
            parser_expect(parser, TOK_RBRACE);
            
            // Build: var i = start; while i < end { body; i = i + 1 }
            // We represent as a repeat with a var decl injected
            // Use a block: { var i = start; while i <= end { body; i = i + 1 } }
            ASTNode *var_init = ast_var_decl(var_tok.value, NULL, start);
            ASTNode *cond = ast_binary_op(OP_LTE, ast_identifier(var_tok.value), end);
            // increment: change i by 1
            ASTNode *inc = ast_assignment(ast_identifier(var_tok.value),
                           ast_binary_op(OP_ADD, ast_identifier(var_tok.value), ast_number(1)));
            // append increment to body
            ASTNode **body_stmts = malloc(sizeof(ASTNode*) * (body->data.block.statement_count + 1));
            for (int i = 0; i < body->data.block.statement_count; i++)
                body_stmts[i] = body->data.block.statements[i];
            body_stmts[body->data.block.statement_count] = inc;
            ASTNode *new_body = ast_block(body_stmts, body->data.block.statement_count + 1);
            ASTNode *while_node = ast_while(cond, new_body);
            ASTNode **outer = malloc(sizeof(ASTNode*) * 2);
            outer[0] = var_init; outer[1] = while_node;
            return ast_block(outer, 2);
        }
        
        // const NAME = value
        if (strcmp(kw, "const") == 0) {
            lexer_next_token(parser->lexer);
            Token name_tok = lexer_peek(parser->lexer);
            parser_expect(parser, TOK_ID);
            parser_expect(parser, TOK_ASSIGN);
            ASTNode *val = parse_expression(parser);
            return ast_var_decl(name_tok.value, NULL, val);  // treat as var for now
        }
        
        // wait_us N  (microsecond delay)
        if (strcmp(kw, "wait_us") == 0) {
            lexer_next_token(parser->lexer);
            ASTNode *duration = parse_expression(parser);
            // Use a call node - codegen will emit delayMicroseconds()
            ASTNode **args = malloc(sizeof(ASTNode*));
            args[0] = duration;
            return ast_call("delayMicroseconds", args, 1);
        }
    }

    
    // Function call as statement: name(args...)
    if (parser_match(parser, TOK_ID)) {
        Token name_tok = lexer_peek(parser->lexer);
        // Peek ahead: if next token after ID is '(', it's a function call
        lexer_next_token(parser->lexer);  // consume the ID
        if (parser_match(parser, TOK_LPAREN)) {
            lexer_next_token(parser->lexer);  // consume '('
            ASTNode **args = malloc(sizeof(ASTNode*) * 16);
            int arg_count = 0;
            while (!parser_match(parser, TOK_RPAREN) && !parser_match(parser, TOK_EOF)) {
                args[arg_count++] = parse_expression(parser);
                if (parser_match(parser, TOK_COMMA)) lexer_next_token(parser->lexer);
            }
            parser_expect(parser, TOK_RPAREN);
            return ast_call(name_tok.value, args, arg_count);
        }
        // Not a function call - it's an assignment like: varname = expr
        // (already handled by set/change, but handle bare assignment too)
        if (parser_match(parser, TOK_ASSIGN)) {
            lexer_next_token(parser->lexer);
            ASTNode *value = parse_expression(parser);
            ASTNode *target = ast_identifier(name_tok.value);
            return ast_assignment(target, value);
        }
        // Unknown - just return identifier as expression
        return ast_identifier(name_tok.value);
    }
    
    return NULL;
}


// Parse block
static ASTNode* parse_block(Parser *parser) {
    ASTNode **statements = malloc(sizeof(ASTNode*) * 100);  // Max 100 statements
    int count = 0;
    
    while (!parser_match(parser, TOK_RBRACE) && !parser_match(parser, TOK_EOF)) {
        ASTNode *stmt = parse_statement(parser);
        if (stmt) {
            statements[count++] = stmt;
        } else {
            // CRITICAL FIX: If statement is NULL, we must advance to avoid infinite loop
            // This happens when we encounter an unexpected token
            error_report(parser->errors, ERROR_SYNTAX,
                        parser->lexer->current_token.line,
                        parser->lexer->current_token.column,
                        "Unexpected token, skipping");
            lexer_next_token(parser->lexer);  // Advance to prevent infinite loop
        }
    }
    
    return ast_block(statements, count);
}

// Main parse function
ASTNode* parser_parse(Parser *parser) {
    // Skip include statements (for now, just ignore them)
    while (parser_match(parser, TOK_INCLUDE)) {
        lexer_next_token(parser->lexer);
        if (parser_match(parser, TOK_STRING_LIT)) {
            lexer_next_token(parser->lexer);
        } else {
            error_report(parser->errors, ERROR_SYNTAX, parser->lexer->current_token.line, parser->lexer->current_token.column, "Expected string literal after 'include'");
        }
    }
    
    ASTNode **statements = malloc(sizeof(ASTNode*) * 1024);
    int count = 0;
    
    // Parse global top-level functions (like those from kinetrix_modules)
    while (parser_match(parser, TOK_DEF) || parser_match(parser, TOK_EXTERN)) {
        ASTNode *stmt = parse_statement(parser);
        if (stmt) statements[count++] = stmt;
    }

    // Skip optional "program" keyword
    if (parser_match(parser, TOK_PROGRAM)) {
        lexer_next_token(parser->lexer);
    }
    
    // Parse main program block statements
    if (parser_match(parser, TOK_LBRACE)) {
        lexer_next_token(parser->lexer);
        while (!parser_match(parser, TOK_RBRACE) && !parser_match(parser, TOK_EOF)) {
            ASTNode *stmt = parse_statement(parser);
            if (stmt) {
                statements[count++] = stmt;
            } else {
                error_report(parser->errors, ERROR_SYNTAX, parser->lexer->current_token.line, parser->lexer->current_token.column, "Unexpected token, skipping");
                lexer_next_token(parser->lexer);
            }
        }
        parser_expect(parser, TOK_RBRACE);
    }
    
    // Create combined main block and program node
    ASTNode *main_block = ast_block(statements, count);
    return ast_program(NULL, 0, main_block);
}








