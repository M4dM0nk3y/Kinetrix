/* Kinetrix Parser - Builds AST from source code
 * Multi-pass architecture with proper error recovery
 * V3.0: Arrays, Interrupts, Protocols, Types, Error Handling, Structs, Tasks,
 * FFI
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
  TOK_EOF,
  TOK_ID,
  TOK_NUMBER,
  TOK_STRING_LIT,
  TOK_PROGRAM,
  TOK_LBRACE,
  TOK_RBRACE,
  TOK_SEMI,
  TOK_LPAREN,
  TOK_RPAREN,
  TOK_COMMA,
  TOK_TURN,
  TOK_ON,
  TOK_OFF,
  TOK_WAIT,
  TOK_SET,
  TOK_TO,
  TOK_MAKE,
  TOK_CHANGE,
  TOK_BY,
  TOK_FOREVER,
  TOK_IF,
  TOK_ELSE,
  TOK_READ,
  TOK_ANALOG,
  TOK_PULSE,
  TOK_SERVO,
  TOK_PRINT,
  TOK_PRINTLN,
  TOK_DEF,
  TOK_REPEAT,
  TOK_TONE,
  TOK_NOTONE,
  TOK_FREQ,
  TOK_RETURN,
  TOK_WHILE,
  TOK_BREAK,
  TOK_CONTINUE,
  TOK_ARRAY,
  TOK_SIZE,
  TOK_INDEX,
  TOK_OF,
  TOK_LBRACKET,
  TOK_RBRACKET,
  TOK_SERIAL,
  TOK_WAIT_US,
  TOK_I2C,
  TOK_BEGIN,
  TOK_START,
  TOK_SEND,
  TOK_STOP,
  TOK_FOR,
  TOK_FROM,
  TOK_PIN,
  TOK_IS,
  TOK_HIGH,
  TOK_LOW,
  TOK_VAR,
  TOK_SIN,
  TOK_COS,
  TOK_TAN,
  TOK_SQRT,
  TOK_ASIN,
  TOK_ACOS,
  TOK_ATAN,
  TOK_ATAN2,
  TOK_ASSIGN,
  TOK_EQ,
  TOK_NEQ,
  TOK_LT,
  TOK_GT,
  TOK_LTE,
  TOK_GTE,
  TOK_PLUS,
  TOK_MINUS,
  TOK_STAR,
  TOK_SLASH,
  TOK_MOD,
  TOK_AND,
  TOK_OR,
  TOK_NOT,
  TOK_LOOP,
  TOK_INCLUDE,
  TOK_EXTERN,

  /* ---- V3.0 new tokens ---- */

  /* Explicit types */
  TOK_INT_KW,   /* int  */
  TOK_FLOAT_KW, /* float */
  TOK_BOOL_KW,  /* bool */
  TOK_BYTE_KW,  /* byte */
  TOK_CAST,     /* cast */
  TOK_TRUE,     /* true */
  TOK_FALSE,    /* false */

  /* Interrupt keywords */
  TOK_RISING,   /* rising  */
  TOK_FALLING,  /* falling */
  TOK_CHANGING, /* changing */
  TOK_TIMER,    /* timer */
  TOK_EVERY,    /* every */
  TOK_US_TOK,   /* us  (microseconds suffix) */
  TOK_MS_TOK,   /* ms  (milliseconds suffix) */

  /* Protocol keywords */
  TOK_OPEN,     /* open  */
  TOK_BAUD,     /* baud  */
  TOK_HZ,       /* hz    */
  TOK_SPI,      /* spi   */
  TOK_TRANSFER, /* transfer */
  TOK_RECEIVE,  /* receive  */
  TOK_COUNT,    /* count     */
  TOK_INTO,     /* into      */
  TOK_CONST,    /* const     */
  TOK_WRITE_KW, /* write  */
  TOK_DEVICE,   /* device */
  TOK_REGISTER, /* register */
  TOK_VALUE_KW, /* value  */
  TOK_AT,       /* at    */

  /* Named device abstraction */
  TOK_DEFINE,  /* define */
  TOK_TYPE_KW, /* type  */
  TOK_AS,      /* as    */

  /* Error handling */
  TOK_TRY,        /* try      */
  TOK_ON_ERROR,   /* (on error — parsed as two tokens, handled in parser) */
  TOK_ENABLE,     /* enable   */
  TOK_WATCHDOG,   /* watchdog */
  TOK_FEED,       /* feed     */
  TOK_ASSERT,     /* assert   */
  TOK_TIMEOUT,    /* timeout  */
  TOK_DISABLE,    /* disable  */
  TOK_INTERRUPTS, /* interrupts */

  /* Concurrency */
  TOK_TASK,   /* task   */
  TOK_SHARED, /* shared */

  /* Buffers */
  TOK_BUFFER, /* buffer */
  TOK_PUSH,   /* push   */

  /* Radio APIs */
  TOK_RADIO_SEND,      /* radio_send_peer */
  TOK_RADIO_AVAILABLE, /* radio_available */
  TOK_RADIO_READ,      /* radio_read      */

  /* Wave 2 Motion & Motor Wrappers */
  TOK_STEPPER, /* stepper */
  TOK_MOTOR,   /* motor   */
  TOK_ENCODER, /* encoder */
  TOK_ESC,     /* esc     */
  TOK_PID,     /* pid     */

  /* Wave 3 Communication & Networking */
  TOK_CONNECT,   /* connect   */
  TOK_BLE,       /* ble       */
  TOK_WIFI,      /* wifi      */
  TOK_MQTT,      /* mqtt      */
  TOK_HTTP,      /* http      */
  TOK_WEBSOCKET, /* websocket */
  TOK_SUBSCRIBE, /* subscribe */
  TOK_PUBLISH,   /* publish   */

  /* FFI */
  TOK_COLON, /* : */
  TOK_ARROW, /* -> */

  /* Struct/member access */
  TOK_DOT, /* .  */

  /* Control */
  TOK_ELSE_KW /* kept for clarity */
} TokenType;

typedef struct {
  TokenType type;
  char *value;
  int capacity;
  int line;
  int column;
} Token;

typedef struct {
  FILE *file;
  const char *file_path;
  int current_char;
  int line;
  int column;
  Token current_token;
  ErrorList *errors;
} Lexer;

Lexer *lexer_create(FILE *file, const char *file_path, ErrorList *errors);
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
  int in_loop;     /* For break statement validation */
  int in_function; /* For return statement validation */
} Parser;

Parser *parser_create(FILE *file, const char *file_path, ErrorList *errors);
void parser_free(Parser *parser);

/* Main parsing function */
ASTNode *parser_parse(Parser *parser);

/* Helper functions */
int parser_expect(Parser *parser, TokenType type);
int parser_expect_id(Parser *parser, const char *id);
int parser_match(Parser *parser, TokenType type);

#endif /* KINETRIX_PARSER_H */
