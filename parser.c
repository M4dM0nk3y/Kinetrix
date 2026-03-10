/* Kinetrix Parser Implementation
 * Lexer + Recursive Descent Parser → AST
 */

#define _POSIX_C_SOURCE 200809L
#include "parser.h"
#include <ctype.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

// ============================================================================
// LEXER IMPLEMENTATION
// ============================================================================

Lexer *lexer_create(FILE *file, const char *file_path, ErrorList *errors) {
  Lexer *lexer = malloc(sizeof(Lexer));
  lexer->file = file;
  lexer->file_path = file_path;
  lexer->current_char = fgetc(file);
  lexer->line = 1;
  lexer->column = 1;
  lexer->errors = errors;
  lexer->current_token.value = malloc(256);
  lexer->current_token.capacity = 256;
  lexer->current_token.value[0] = '\0';

  // Read first token
  lexer_next_token(lexer);

  return lexer;
}

void lexer_free(Lexer *lexer) {
  if (lexer->current_token.value)
    free(lexer->current_token.value);
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
  /* # single-line comment */
  if (lexer->current_char == '#') {
    while (lexer->current_char != '\n' && lexer->current_char != EOF) {
      lexer_advance(lexer);
    }
    return;
  }
  /* // single-line comment — peek at next char to distinguish from divide */
  if (lexer->current_char == '/') {
    int next = fgetc(lexer->file);
    if (next == '/') {
      /* Confirmed comment: consume to end of line */
      while (lexer->current_char != '\n' && lexer->current_char != EOF) {
        lexer_advance(lexer);
      }
    } else {
      /* Not a comment — put the char back */
      if (next != EOF)
        ungetc(next, lexer->file);
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
    while (lexer->current_char != '"' && lexer->current_char != EOF) {
      if (i >= lexer->current_token.capacity - 2) {
        lexer->current_token.capacity *= 2;
        lexer->current_token.value =
            realloc(lexer->current_token.value, lexer->current_token.capacity);
      }
      lexer->current_token.value[i++] = lexer->current_char;
      lexer_advance(lexer);
    }
    lexer->current_token.value[i] = '\0';
    if (lexer->current_char == '"')
      lexer_advance(lexer);
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
  if (lexer->current_char == ':') {
    lexer->current_token.type = TOK_COLON;
    strcpy(lexer->current_token.value, ":");
    lexer_advance(lexer);
    return;
  }
  if (lexer->current_char == '-') {
    lexer_advance(lexer);
    if (lexer->current_char == '>') {
      lexer->current_token.type = TOK_ARROW;
      strcpy(lexer->current_token.value, "->");
      lexer_advance(lexer);
    } else {
      lexer->current_token.type = TOK_MINUS;
      strcpy(lexer->current_token.value, "-");
    }
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
    lexer->current_token.type = TOK_MOD;
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
      lexer->current_token.type = TOK_NEQ;
      strcpy(lexer->current_token.value, "!=");
      lexer_advance(lexer);
    }
    return;
  }
  if (lexer->current_char == '<') {
    lexer_advance(lexer);
    if (lexer->current_char == '=') {
      lexer->current_token.type = TOK_LTE;
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
      lexer->current_token.type = TOK_GTE;
      strcpy(lexer->current_token.value, ">=");
      lexer_advance(lexer);
    } else if (lexer->current_char == '-') {
      /* Check for -> */
      lexer_advance(lexer);
      if (lexer->current_char == '>') {
        lexer->current_token.type = TOK_ARROW;
        strcpy(lexer->current_token.value, "->");
        lexer_advance(lexer);
      } else {
        /* It was a lone > followed by - : emit > now, handle - next time */
        lexer->current_token.type = TOK_GT;
        strcpy(lexer->current_token.value, ">");
      }
    } else {
      lexer->current_token.type = TOK_GT;
      strcpy(lexer->current_token.value, ">");
    }
    return;
  }
  /* Dot operator */
  if (lexer->current_char == '.') {
    lexer->current_token.type = TOK_DOT;
    strcpy(lexer->current_token.value, ".");
    lexer_advance(lexer);
    return;
  }
  /* Colon operator */
  if (lexer->current_char == ':') {
    lexer->current_token.type = TOK_COLON;
    strcpy(lexer->current_token.value, ":");
    lexer_advance(lexer);
    return;
  }

  // Hex literals: 0x...
  if (lexer->current_char == '0') {
    int i = 0;
    lexer->current_token.value[i++] = lexer->current_char;
    lexer_advance(lexer);
    if (lexer->current_char == 'x' || lexer->current_char == 'X') {
      lexer->current_token.value[i++] = lexer->current_char;
      lexer_advance(lexer);
      while (isxdigit(lexer->current_char)) {
        if (i >= lexer->current_token.capacity - 1) {
          lexer->current_token.capacity *= 2;
          lexer->current_token.value = realloc(lexer->current_token.value,
                                               lexer->current_token.capacity);
        }
        lexer->current_token.value[i++] = lexer->current_char;
        lexer_advance(lexer);
      }
      lexer->current_token.value[i] = '\0';
      lexer->current_token.type = TOK_NUMBER;
      // Convert hex string to decimal value in-place
      long val = strtol(lexer->current_token.value, NULL, 16);

      // Dynamic allocation for string conversion instead of fixed size 256
      int needed_size = snprintf(NULL, 0, "%ld", val) + 1;
      if (lexer->current_token.capacity < needed_size) {
        lexer->current_token.capacity = needed_size;
        lexer->current_token.value =
            realloc(lexer->current_token.value, lexer->current_token.capacity);
      }
      snprintf(lexer->current_token.value, lexer->current_token.capacity, "%ld",
               val);
      return;
    } else if (lexer->current_char == 'b' || lexer->current_char == 'B') {
      lexer->current_token.value[i++] = lexer->current_char;
      lexer_advance(lexer);
      while (lexer->current_char == '0' || lexer->current_char == '1') {
        if (i >= lexer->current_token.capacity - 1) {
          lexer->current_token.capacity *= 2;
          lexer->current_token.value = realloc(lexer->current_token.value,
                                               lexer->current_token.capacity);
        }
        lexer->current_token.value[i++] = lexer->current_char;
        lexer_advance(lexer);
      }
      lexer->current_token.value[i] = '\0';
      lexer->current_token.type = TOK_NUMBER;
      long val = strtol(lexer->current_token.value + 2, NULL, 2);

      int needed_size = snprintf(NULL, 0, "%ld", val) + 1;
      if (lexer->current_token.capacity < needed_size) {
        lexer->current_token.capacity = needed_size;
        lexer->current_token.value =
            realloc(lexer->current_token.value, lexer->current_token.capacity);
      }
      snprintf(lexer->current_token.value, lexer->current_token.capacity, "%ld",
               val);
      return;
    } else {
      // Regular number starting with 0
      while (isdigit(lexer->current_char) || lexer->current_char == '.') {
        if (i >= lexer->current_token.capacity - 1) {
          lexer->current_token.capacity *= 2;
          lexer->current_token.value = realloc(lexer->current_token.value,
                                               lexer->current_token.capacity);
        }
        lexer->current_token.value[i++] = lexer->current_char;
        lexer_advance(lexer);
      }
      lexer->current_token.value[i] = '\0';
      lexer->current_token.type = TOK_NUMBER;
      return;
    }
  }

  /* Decimal numbers starting with 1-9 */
  if (isdigit(lexer->current_char)) {
    int i = 0;
    while (isdigit(lexer->current_char) || lexer->current_char == '.') {
      if (i >= lexer->current_token.capacity - 1) {
        lexer->current_token.capacity *= 2;
        lexer->current_token.value =
            realloc(lexer->current_token.value, lexer->current_token.capacity);
      }
      lexer->current_token.value[i++] = lexer->current_char;
      lexer_advance(lexer);
    }
    lexer->current_token.value[i] = '\0';
    lexer->current_token.type = TOK_NUMBER;
    return;
  }

  if (isalpha(lexer->current_char) || lexer->current_char == '_') {
    int i = 0;
    while (isalnum(lexer->current_char) || lexer->current_char == '_') {
      if (i >= lexer->current_token.capacity - 2) {
        lexer->current_token.capacity *= 2;
        lexer->current_token.value =
            realloc(lexer->current_token.value, lexer->current_token.capacity);
      }
      lexer->current_token.value[i++] = lexer->current_char;
      lexer_advance(lexer);
    }
    lexer->current_token.value[i] = '\0';

    // ---- keyword table ----
    const char *v = lexer->current_token.value;
    if (!strcmp(v, "program"))
      lexer->current_token.type = TOK_PROGRAM;
    else if (!strcmp(v, "extern"))
      lexer->current_token.type = TOK_EXTERN;
    else if (!strcmp(v, "def"))
      lexer->current_token.type = TOK_DEF;
    else if (!strcmp(v, "repeat"))
      lexer->current_token.type = TOK_REPEAT;
    else if (!strcmp(v, "loop"))
      lexer->current_token.type = TOK_LOOP;
    else if (!strcmp(v, "turn"))
      lexer->current_token.type = TOK_TURN;
    else if (!strcmp(v, "on"))
      lexer->current_token.type = TOK_ON;
    else if (!strcmp(v, "off"))
      lexer->current_token.type = TOK_OFF;
    else if (!strcmp(v, "wait"))
      lexer->current_token.type = TOK_WAIT;
    else if (!strcmp(v, "wait_us"))
      lexer->current_token.type = TOK_WAIT_US;
    else if (!strcmp(v, "forever"))
      lexer->current_token.type = TOK_FOREVER;
    else if (!strcmp(v, "if"))
      lexer->current_token.type = TOK_IF;
    else if (!strcmp(v, "else"))
      lexer->current_token.type = TOK_ELSE;
    else if (!strcmp(v, "make"))
      lexer->current_token.type = TOK_MAKE;
    else if (!strcmp(v, "var"))
      lexer->current_token.type = TOK_VAR;
    else if (!strcmp(v, "set"))
      lexer->current_token.type = TOK_SET;
    else if (!strcmp(v, "change"))
      lexer->current_token.type = TOK_CHANGE;
    else if (!strcmp(v, "to"))
      lexer->current_token.type = TOK_TO;
    else if (!strcmp(v, "by"))
      lexer->current_token.type = TOK_BY;
    else if (!strcmp(v, "read"))
      lexer->current_token.type = TOK_READ;
    else if (!strcmp(v, "write"))
      lexer->current_token.type = TOK_WRITE_KW;
    else if (!strcmp(v, "analog"))
      lexer->current_token.type = TOK_ANALOG;
    else if (!strcmp(v, "pulse"))
      lexer->current_token.type = TOK_PULSE;
    else if (!strcmp(v, "serial"))
      lexer->current_token.type = TOK_SERIAL;
    else if (!strcmp(v, "servo"))
      lexer->current_token.type = TOK_SERVO;
    else if (!strcmp(v, "print"))
      lexer->current_token.type = TOK_PRINT;
    else if (!strcmp(v, "println"))
      lexer->current_token.type = TOK_PRINTLN;
    else if (!strcmp(v, "tone"))
      lexer->current_token.type = TOK_TONE;
    else if (!strcmp(v, "notone"))
      lexer->current_token.type = TOK_NOTONE;
    else if (!strcmp(v, "return"))
      lexer->current_token.type = TOK_RETURN;
    else if (!strcmp(v, "while"))
      lexer->current_token.type = TOK_WHILE;
    else if (!strcmp(v, "for"))
      lexer->current_token.type = TOK_FOR;
    else if (!strcmp(v, "from"))
      lexer->current_token.type = TOK_FROM;
    else if (!strcmp(v, "break"))
      lexer->current_token.type = TOK_BREAK;
    else if (!strcmp(v, "array"))
      lexer->current_token.type = TOK_ARRAY;
    else if (!strcmp(v, "buffer"))
      lexer->current_token.type = TOK_BUFFER;
    else if (!strcmp(v, "push"))
      lexer->current_token.type = TOK_PUSH;
    /* size, index, count, into, value, start, stop, at, as — context-sensitive,
     * kept as TOK_ID */
    else if (!strcmp(v, "of"))
      lexer->current_token.type = TOK_OF;
    else if (!strcmp(v, "i2c"))
      lexer->current_token.type = TOK_I2C;
    else if (!strcmp(v, "spi"))
      lexer->current_token.type = TOK_SPI;
    else if (!strcmp(v, "begin"))
      lexer->current_token.type = TOK_BEGIN;
    else if (!strcmp(v, "start"))
      lexer->current_token.type = TOK_START;
    else if (!strcmp(v, "send"))
      lexer->current_token.type = TOK_SEND;
    else if (!strcmp(v, "stop"))
      lexer->current_token.type = TOK_STOP;
    else if (!strcmp(v, "open"))
      lexer->current_token.type = TOK_OPEN;
    else if (!strcmp(v, "baud"))
      lexer->current_token.type = TOK_BAUD;
    else if (!strcmp(v, "hz"))
      lexer->current_token.type = TOK_HZ;
    else if (!strcmp(v, "transfer"))
      lexer->current_token.type = TOK_TRANSFER;
    else if (!strcmp(v, "receive"))
      lexer->current_token.type = TOK_RECEIVE;
    else if (!strcmp(v, "device"))
      lexer->current_token.type = TOK_DEVICE;
    else if (!strcmp(v, "register"))
      lexer->current_token.type = TOK_REGISTER;
    else if (!strcmp(v, "pin"))
      lexer->current_token.type = TOK_PIN;
    else if (!strcmp(v, "is"))
      lexer->current_token.type = TOK_IS;
    else if (!strcmp(v, "high"))
      lexer->current_token.type = TOK_HIGH;
    else if (!strcmp(v, "low"))
      lexer->current_token.type = TOK_LOW;
    else if (!strcmp(v, "sin"))
      lexer->current_token.type = TOK_SIN;
    else if (!strcmp(v, "cos"))
      lexer->current_token.type = TOK_COS;
    else if (!strcmp(v, "tan"))
      lexer->current_token.type = TOK_TAN;
    else if (!strcmp(v, "and"))
      lexer->current_token.type = TOK_AND;
    else if (!strcmp(v, "or"))
      lexer->current_token.type = TOK_OR;
    else if (!strcmp(v, "sqrt"))
      lexer->current_token.type = TOK_SQRT;
    else if (!strcmp(v, "asin"))
      lexer->current_token.type = TOK_ASIN;
    else if (!strcmp(v, "acos"))
      lexer->current_token.type = TOK_ACOS;
    else if (!strcmp(v, "atan"))
      lexer->current_token.type = TOK_ATAN;
    else if (!strcmp(v, "atan2"))
      lexer->current_token.type = TOK_ATAN2;
    else if (!strcmp(v, "include"))
      lexer->current_token.type = TOK_INCLUDE;
    else if (!strcmp(v, "import"))
      lexer->current_token.type = TOK_INCLUDE;
    /* V3.0 explicit types */
    else if (!strcmp(v, "int"))
      lexer->current_token.type = TOK_INT_KW;
    else if (!strcmp(v, "float"))
      lexer->current_token.type = TOK_FLOAT_KW;
    else if (!strcmp(v, "bool"))
      lexer->current_token.type = TOK_BOOL_KW;
    else if (!strcmp(v, "byte"))
      lexer->current_token.type = TOK_BYTE_KW;
    else if (!strcmp(v, "cast"))
      lexer->current_token.type = TOK_CAST;
    else if (!strcmp(v, "true"))
      lexer->current_token.type = TOK_TRUE;
    else if (!strcmp(v, "false"))
      lexer->current_token.type = TOK_FALSE;
    /* V3.0 interrupt */
    else if (!strcmp(v, "rising"))
      lexer->current_token.type = TOK_RISING;
    else if (!strcmp(v, "falling"))
      lexer->current_token.type = TOK_FALLING;
    else if (!strcmp(v, "changing"))
      lexer->current_token.type = TOK_CHANGING;
    else if (!strcmp(v, "timer"))
      lexer->current_token.type = TOK_TIMER;
    else if (!strcmp(v, "every"))
      lexer->current_token.type = TOK_EVERY;
    else if (!strcmp(v, "us"))
      lexer->current_token.type = TOK_US_TOK;
    else if (!strcmp(v, "ms"))
      lexer->current_token.type = TOK_MS_TOK;
    /* V3.0 error handling */
    else if (!strcmp(v, "try"))
      lexer->current_token.type = TOK_TRY;
    else if (!strcmp(v, "enable"))
      lexer->current_token.type = TOK_ENABLE;
    else if (!strcmp(v, "watchdog"))
      lexer->current_token.type = TOK_WATCHDOG;
    else if (!strcmp(v, "feed"))
      lexer->current_token.type = TOK_FEED;
    else if (!strcmp(v, "assert"))
      lexer->current_token.type = TOK_ASSERT;
    else if (!strcmp(v, "timeout"))
      lexer->current_token.type = TOK_TIMEOUT;
    else if (!strcmp(v, "disable"))
      lexer->current_token.type = TOK_DISABLE;
    else if (!strcmp(v, "interrupts"))
      lexer->current_token.type = TOK_INTERRUPTS;
    /* V3.0 structs/define */
    else if (!strcmp(v, "define"))
      lexer->current_token.type = TOK_DEFINE;
    else if (!strcmp(v, "type"))
      lexer->current_token.type = TOK_TYPE_KW;
    /* V3.0 concurrency */
    else if (!strcmp(v, "task"))
      lexer->current_token.type = TOK_TASK;
    else if (!strcmp(v, "shared"))
      lexer->current_token.type = TOK_SHARED;
    /* V3.0 explicit types */
    else if (!strcmp(v, "continue"))
      lexer->current_token.type = TOK_CONTINUE;
    else if (!strcmp(v, "radio_send_peer"))
      lexer->current_token.type = TOK_RADIO_SEND;
    else if (!strcmp(v, "radio_available"))
      lexer->current_token.type = TOK_RADIO_AVAILABLE;
    else if (!strcmp(v, "radio_read"))
      lexer->current_token.type = TOK_RADIO_READ;
    /* Wave 2 Wrappers */
    else if (!strcmp(v, "stepper"))
      lexer->current_token.type = TOK_STEPPER;
    else if (!strcmp(v, "motor"))
      lexer->current_token.type = TOK_MOTOR;
    else if (!strcmp(v, "encoder"))
      lexer->current_token.type = TOK_ENCODER;
    else if (!strcmp(v, "esc"))
      lexer->current_token.type = TOK_ESC;
    else if (!strcmp(v, "pid"))
      lexer->current_token.type = TOK_PID;
    /* Wave 3 Wrappers */
    else if (!strcmp(v, "connect"))
      lexer->current_token.type = TOK_CONNECT;
    else if (!strcmp(v, "ble"))
      lexer->current_token.type = TOK_BLE;
    else if (!strcmp(v, "wifi"))
      lexer->current_token.type = TOK_WIFI;
    else if (!strcmp(v, "mqtt"))
      lexer->current_token.type = TOK_MQTT;
    else if (!strcmp(v, "http"))
      lexer->current_token.type = TOK_HTTP;
    else if (!strcmp(v, "websocket") || !strcmp(v, "ws"))
      lexer->current_token.type = TOK_WEBSOCKET;
    else if (!strcmp(v, "subscribe"))
      lexer->current_token.type = TOK_SUBSCRIBE;
    else if (!strcmp(v, "publish"))
      lexer->current_token.type = TOK_PUBLISH;
    /* Wave 4 Navigation/Storage */
    else if (!strcmp(v, "imu"))
      lexer->current_token.type = TOK_IMU;
    else if (!strcmp(v, "accel"))
      lexer->current_token.type = TOK_ACCEL;
    else if (!strcmp(v, "gyro"))
      lexer->current_token.type = TOK_GYRO;
    else if (!strcmp(v, "orientation"))
      lexer->current_token.type = TOK_ORIENTATION;
    else if (!strcmp(v, "gps"))
      lexer->current_token.type = TOK_GPS;
    else if (!strcmp(v, "latitude"))
      lexer->current_token.type = TOK_LATITUDE;
    else if (!strcmp(v, "longitude"))
      lexer->current_token.type = TOK_LONGITUDE;
    else if (!strcmp(v, "altitude"))
      lexer->current_token.type = TOK_ALTITUDE;
    else if (!strcmp(v, "lidar"))
      lexer->current_token.type = TOK_LIDAR;
    else if (!strcmp(v, "precise"))
      lexer->current_token.type = TOK_PRECISE;
    else if (!strcmp(v, "sd"))
      lexer->current_token.type = TOK_SD;
    else if (!strcmp(v, "file"))
      lexer->current_token.type = TOK_FILE;
    /* V3.0 handled as TOK_ID in parser */
    else if (!strcmp(v, "not"))
      lexer->current_token.type = TOK_NOT;
    else if (!strcmp(v, "const"))
      lexer->current_token.type = TOK_ID;
    else if (!strcmp(v, "map"))
      lexer->current_token.type = TOK_ID;
    else if (!strcmp(v, "constrain"))
      lexer->current_token.type = TOK_ID;
    else if (!strcmp(v, "abs"))
      lexer->current_token.type = TOK_ID;
    else
      lexer->current_token.type = TOK_ID;

    return;
  }

  // Unknown character
  char msg[256];
  snprintf(msg, sizeof(msg), "Unexpected character: '%c'", lexer->current_char);
  error_report(lexer->errors, ERROR_LEXICAL, lexer->line, lexer->column, "%s",
               msg);
  lexer_advance(lexer);
  lexer_next_token(lexer);
}

Token lexer_peek(Lexer *lexer) { return lexer->current_token; }

// ============================================================================
// PARSER IMPLEMENTATION
// ============================================================================

Parser *parser_create(FILE *file, const char *file_path, ErrorList *errors) {
  Parser *parser = malloc(sizeof(Parser));
  parser->lexer = lexer_create(file, file_path, errors);
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

/* Match an identifier by string value (for context-sensitive keywords) */
static int parser_match_id(Parser *parser, const char *name) {
  return parser->lexer->current_token.type == TOK_ID &&
         strcmp(parser->lexer->current_token.value, name) == 0;
}

static const char *token_type_to_string(TokenType type) {
  switch (type) {
  case TOK_EOF:
    return "EOF";
  case TOK_ID:
    return "Identifier";
  case TOK_NUMBER:
    return "Number";
  case TOK_STRING_LIT:
    return "String Literal";
  case TOK_PROGRAM:
    return "program";
  case TOK_LBRACE:
    return "{";
  case TOK_RBRACE:
    return "}";
  case TOK_SEMI:
    return ";";
  case TOK_LPAREN:
    return "(";
  case TOK_RPAREN:
    return ")";
  case TOK_COMMA:
    return ",";
  case TOK_TURN:
    return "turn";
  case TOK_ON:
    return "on";
  case TOK_OFF:
    return "off";
  case TOK_WAIT:
    return "wait";
  case TOK_SET:
    return "set";
  case TOK_TO:
    return "to";
  case TOK_MAKE:
    return "make";
  case TOK_CHANGE:
    return "change";
  case TOK_BY:
    return "by";
  case TOK_FOREVER:
    return "forever";
  case TOK_IF:
    return "if";
  case TOK_ELSE:
    return "else";
  case TOK_ELSE_KW:
    return "else";
  case TOK_READ:
    return "read";
  case TOK_ANALOG:
    return "analog";
  case TOK_PULSE:
    return "pulse";
  case TOK_SERVO:
    return "servo";
  case TOK_PRINT:
    return "print";
  case TOK_PRINTLN:
    return "println";
  case TOK_DEF:
    return "def";
  case TOK_REPEAT:
    return "repeat";
  case TOK_TONE:
    return "tone";
  case TOK_NOTONE:
    return "notone";
  case TOK_FREQ:
    return "freq";
  case TOK_RETURN:
    return "return";
  case TOK_WHILE:
    return "while";
  case TOK_BREAK:
    return "break";
  case TOK_CONTINUE:
    return "continue";
  case TOK_ARRAY:
    return "array";
  case TOK_SIZE:
    return "size";
  case TOK_INDEX:
    return "index";
  case TOK_OF:
    return "of";
  case TOK_LBRACKET:
    return "[";
  case TOK_RBRACKET:
    return "]";
  case TOK_SERIAL:
    return "serial";
  case TOK_WAIT_US:
    return "wait_us";
  case TOK_I2C:
    return "i2c";
  case TOK_BEGIN:
    return "begin";
  case TOK_START:
    return "start";
  case TOK_SEND:
    return "send";
  case TOK_STOP:
    return "stop";
  case TOK_FOR:
    return "for";
  case TOK_FROM:
    return "from";
  case TOK_PIN:
    return "pin";
  case TOK_IS:
    return "is";
  case TOK_HIGH:
    return "HIGH";
  case TOK_LOW:
    return "LOW";
  case TOK_VAR:
    return "var";
  case TOK_SIN:
    return "sin";
  case TOK_COS:
    return "cos";
  case TOK_TAN:
    return "tan";
  case TOK_SQRT:
    return "sqrt";
  case TOK_ASIN:
    return "asin";
  case TOK_ACOS:
    return "acos";
  case TOK_ATAN:
    return "atan";
  case TOK_ATAN2:
    return "atan2";
  case TOK_ASSIGN:
    return "=";
  case TOK_EQ:
    return "==";
  case TOK_NEQ:
    return "!=";
  case TOK_LT:
    return "<";
  case TOK_GT:
    return ">";
  case TOK_LTE:
    return "<=";
  case TOK_GTE:
    return ">=";
  case TOK_PLUS:
    return "+";
  case TOK_MINUS:
    return "-";
  case TOK_STAR:
    return "*";
  case TOK_SLASH:
    return "/";
  case TOK_MOD:
    return "%";
  case TOK_AND:
    return "and";
  case TOK_OR:
    return "or";
  case TOK_NOT:
    return "not";
  case TOK_LOOP:
    return "loop";
  case TOK_TRUE:
    return "true";
  case TOK_FALSE:
    return "false";
  case TOK_INCLUDE:
    return "include";
  case TOK_EXTERN:
    return "extern";
  case TOK_INT_KW:
    return "int";
  case TOK_FLOAT_KW:
    return "float";
  case TOK_BOOL_KW:
    return "bool";
  case TOK_BYTE_KW:
    return "byte";
  case TOK_CAST:
    return "cast";
  case TOK_RISING:
    return "rising";
  case TOK_FALLING:
    return "falling";
  case TOK_CHANGING:
    return "changing";
  case TOK_TIMER:
    return "timer";
  case TOK_EVERY:
    return "every";
  case TOK_US_TOK:
    return "us";
  case TOK_MS_TOK:
    return "ms";
  case TOK_OPEN:
    return "open";
  case TOK_BAUD:
    return "baud";
  case TOK_HZ:
    return "hz";
  case TOK_SPI:
    return "spi";
  case TOK_TRANSFER:
    return "transfer";
  case TOK_RECEIVE:
    return "receive";
  case TOK_COUNT:
    return "count";
  case TOK_INTO:
    return "into";
  case TOK_WRITE_KW:
    return "write";
  case TOK_DEVICE:
    return "device";
  case TOK_REGISTER:
    return "register";
  case TOK_VALUE_KW:
    return "value";
  case TOK_AT:
    return "at";
  case TOK_DEFINE:
    return "define";
  case TOK_TYPE_KW:
    return "type";
  case TOK_AS:
    return "as";
  case TOK_TRY:
    return "try";
  case TOK_ON_ERROR:
    return "on_error";
  case TOK_ENABLE:
    return "enable";
  case TOK_WATCHDOG:
    return "watchdog";
  case TOK_FEED:
    return "feed";
  case TOK_ASSERT:
    return "assert";
  case TOK_TIMEOUT:
    return "timeout";
  case TOK_DISABLE:
    return "disable";
  case TOK_INTERRUPTS:
    return "interrupts";
  case TOK_TASK:
    return "task";
  case TOK_SHARED:
    return "shared";
  case TOK_BUFFER:
    return "buffer";
  case TOK_PUSH:
    return "push";
  case TOK_RADIO_SEND:
    return "radio_send_peer";
  case TOK_RADIO_AVAILABLE:
    return "radio_available";
  case TOK_RADIO_READ:
    return "radio_read";
  case TOK_COLON:
    return ":";
  case TOK_ARROW:
    return "->";
  case TOK_DOT:
    return ".";
  case TOK_CONST:
    return "const";
  default:
    return "Unknown Token";
  }
}

int parser_expect(Parser *parser, TokenType type) {
  if (parser_match(parser, type)) {
    lexer_next_token(parser->lexer);
    return 1;
  }

  char msg[256];
  snprintf(msg, sizeof(msg), "Expected '%s', got '%s'%s%s",
           token_type_to_string(type),
           token_type_to_string(parser->lexer->current_token.type),
           parser->lexer->current_token.type == TOK_ID ? " (" : "",
           parser->lexer->current_token.type == TOK_ID
               ? parser->lexer->current_token.value
               : "");

  if (parser->lexer->current_token.type == TOK_ID) {
    snprintf(msg + strlen(msg), sizeof(msg) - strlen(msg), ")");
  }

  error_report(parser->errors, ERROR_SYNTAX, parser->lexer->current_token.line,
               parser->lexer->current_token.column, "%s", msg);
  return 0;
}

int parser_expect_id(Parser *parser, const char *id) {
  if (parser_match_id(parser, id)) {
    lexer_next_token(parser->lexer);
    return 1;
  }

  error_report(parser->errors, ERROR_SYNTAX, parser->lexer->current_token.line,
               parser->lexer->current_token.column,
               "Expected identifier '%s', got '%s'", id,
               parser->lexer->current_token.value
                   ? parser->lexer->current_token.value
                   : token_type_to_string(parser->lexer->current_token.type));
  return 0;
}

// Forward declarations
static ASTNode *parse_primary(Parser *parser);
static ASTNode *parse_arithmetic(Parser *parser);
static ASTNode *parse_comparison(Parser *parser);
static ASTNode *parse_expression(Parser *parser);
static ASTNode *parse_statement(Parser *parser);
static ASTNode *parse_block(Parser *parser);
static ASTNode *clone_target(ASTNode *node);

// Parse primary expression
static ASTNode *parse_primary(Parser *parser) {
  Token tok = parser->lexer->current_token;
  tok.value = strdup(tok.value);

  // Unary Minus
  if (parser_match(parser, TOK_MINUS)) {
    lexer_next_token(parser->lexer);
    ASTNode *operand = parse_primary(parser);
    return ast_binary_op(OP_SUB, ast_number(0), operand); // 0 - value
  }

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

  // Booleans literals explicitly bound natively over generic tokens
  if (parser_match(parser, TOK_TRUE)) {
    lexer_next_token(parser->lexer);
    return ast_bool(1);
  }
  if (parser_match(parser, TOK_FALSE)) {
    lexer_next_token(parser->lexer);
    return ast_bool(0);
  }

  // Identifier or built-in pseudo-functions
  if (parser_match(parser, TOK_ID)) {
    char *name = strdup(tok.value);
    lexer_next_token(parser->lexer);

    // compute pid <expr>
    if (strcmp(name, "compute") == 0) {
      if (!parser_match(parser, TOK_PID)) {
        error_report(parser->errors, ERROR_SYNTAX,
                     parser->lexer->current_token.line,
                     parser->lexer->current_token.column,
                     "Expected 'pid' after 'compute'");
      } else {
        lexer_next_token(parser->lexer);
      }
      ASTNode *expr = parse_expression(parser);
      free(name);
      return ast_pid_compute(expr);
    }

    // map(value, fromLow, fromHigh, toLow, toHigh)
    if (strcmp(name, "map") == 0) {
      parser_expect(parser, TOK_LPAREN);
      ASTNode *v = parse_expression(parser);
      parser_expect(parser, TOK_COMMA);
      ASTNode *fl = parse_expression(parser);
      parser_expect(parser, TOK_COMMA);
      ASTNode *fh = parse_expression(parser);
      parser_expect(parser, TOK_COMMA);
      ASTNode *tl = parse_expression(parser);
      parser_expect(parser, TOK_COMMA);
      ASTNode *th = parse_expression(parser);
      parser_expect(parser, TOK_RPAREN);
      ASTNode **args = malloc(sizeof(ASTNode *) * 5);
      args[0] = v;
      args[1] = fl;
      args[2] = fh;
      args[3] = tl;
      args[4] = th;
      free(name);
      return ast_call("map", args, 5);
    }

    // constrain(value, min, max)
    if (strcmp(name, "constrain") == 0) {
      parser_expect(parser, TOK_LPAREN);
      ASTNode *v = parse_expression(parser);
      parser_expect(parser, TOK_COMMA);
      ASTNode *mn = parse_expression(parser);
      parser_expect(parser, TOK_COMMA);
      ASTNode *mx = parse_expression(parser);
      parser_expect(parser, TOK_RPAREN);
      ASTNode **args = malloc(sizeof(ASTNode *) * 3);
      args[0] = v;
      args[1] = mn;
      args[2] = mx;
      free(name);
      return ast_call("constrain", args, 3);
    }

    // abs(value)
    if (strcmp(name, "abs") == 0) {
      parser_expect(parser, TOK_LPAREN);
      ASTNode *v = parse_expression(parser);
      parser_expect(parser, TOK_RPAREN);
      ASTNode **args = malloc(sizeof(ASTNode *) * 1);
      args[0] = v;
      free(name);
      return ast_call("abs", args, 1);
    }

    // random(min, max)
    if (strcmp(name, "random") == 0) {
      parser_expect(parser, TOK_LPAREN);
      ASTNode *mn = parse_expression(parser);
      parser_expect(parser, TOK_COMMA);
      ASTNode *mx = parse_expression(parser);
      parser_expect(parser, TOK_RPAREN);
      ASTNode **args = malloc(sizeof(ASTNode *) * 2);
      args[0] = mn;
      args[1] = mx;
      free(name);
      return ast_call("random", args, 2);
    }

    // min(a, b) and max(a, b)
    if (strcmp(name, "min") == 0 || strcmp(name, "max") == 0) {
      char fname[8];
      strcpy(fname, name);
      parser_expect(parser, TOK_LPAREN);
      ASTNode *a = parse_expression(parser);
      parser_expect(parser, TOK_COMMA);
      ASTNode *b = parse_expression(parser);
      parser_expect(parser, TOK_RPAREN);
      ASTNode **args = malloc(sizeof(ASTNode *) * 2);
      args[0] = a;
      args[1] = b;
      free(name);
      return ast_call(fname, args, 2);
    }

    // Function call
    if (parser_match(parser, TOK_LPAREN)) {
      lexer_next_token(parser->lexer);
      ASTNode **args = NULL;
      int arg_count = 0;
      if (!parser_match(parser, TOK_RPAREN)) {
        int args_capacity = 4;
        args = malloc(sizeof(ASTNode *) * args_capacity);
        args[arg_count++] = parse_expression(parser);
        while (parser_match(parser, TOK_COMMA)) {
          lexer_next_token(parser->lexer);
          if (arg_count >= args_capacity) {
            args_capacity *= 2;
            args = realloc(args, sizeof(ASTNode *) * args_capacity);
          }
          args[arg_count++] = parse_expression(parser);
        }
      }
      parser_expect(parser, TOK_RPAREN);
      /* dot-method call: obj.method() — treat as call */
      ASTNode *result = ast_call(name, args, arg_count);
      return result;
    }

    // Array access
    if (parser_match(parser, TOK_LBRACKET)) {
      lexer_next_token(parser->lexer);
      ASTNode *index = parse_expression(parser);
      parser_expect(parser, TOK_RBRACKET);
      return ast_array_access(ast_identifier(name), index);
    }

    /* Dot member access: obj.field  (postfix) */
    ASTNode *base = ast_identifier(name);
    while (parser_match(parser, TOK_DOT)) {
      lexer_next_token(parser->lexer);
      Token member_tok = parser->lexer->current_token;
      member_tok.value = strdup(member_tok.value);
      /* consume member name (may be any token with a string value) */
      lexer_next_token(parser->lexer);
      base = ast_struct_access(base, member_tok.value);
    }
    return base;
  }

  // Parenthesized expression
  if (parser_match(parser, TOK_LPAREN)) {
    lexer_next_token(parser->lexer);
    ASTNode *expr = parse_expression(parser);
    parser_expect(parser, TOK_RPAREN);
    return expr;
  }

  // Radio expressions
  if (parser_match(parser, TOK_RADIO_AVAILABLE)) {
    lexer_next_token(parser->lexer);
    parser_expect(parser, TOK_LPAREN);
    parser_expect(parser, TOK_RPAREN);
    return ast_radio_available();
  }
  if (parser_match(parser, TOK_RADIO_READ)) {
    lexer_next_token(parser->lexer);
    parser_expect(parser, TOK_LPAREN);
    parser_expect(parser, TOK_RPAREN);
    return ast_radio_read();
  }

  // Math functions
  if (parser_match(parser, TOK_SIN) || parser_match(parser, TOK_COS) ||
      parser_match(parser, TOK_TAN) || parser_match(parser, TOK_SQRT) ||
      parser_match(parser, TOK_ASIN) || parser_match(parser, TOK_ACOS) ||
      parser_match(parser, TOK_ATAN) || parser_match(parser, TOK_ATAN2)) {

    MathFunc func = MATH_SIN;
    if (parser_match(parser, TOK_COS))
      func = MATH_COS;
    else if (parser_match(parser, TOK_TAN))
      func = MATH_TAN;
    else if (parser_match(parser, TOK_SQRT))
      func = MATH_SQRT;
    else if (parser_match(parser, TOK_ASIN))
      func = MATH_ASIN;
    else if (parser_match(parser, TOK_ACOS))
      func = MATH_ACOS;
    else if (parser_match(parser, TOK_ATAN))
      func = MATH_ATAN;
    else if (parser_match(parser, TOK_ATAN2))
      func = MATH_ATAN2;

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
      ASTNode *timeout = NULL;
      if (parser_match(parser, TOK_TIMEOUT)) {
        lexer_next_token(parser->lexer);
        timeout = parse_expression(parser);
      }
      return ast_pulse_read(pin, timeout);
    }

    /* read i2c device <addr> register <reg> [count <n> into <array>] (V3.0
     * high-level) */
    if (parser_match(parser, TOK_I2C)) {
      lexer_next_token(parser->lexer);
      if (parser_match(parser, TOK_DEVICE)) {
        lexer_next_token(parser->lexer);
      }
      ASTNode *addr = parse_expression(parser);
      ASTNode *reg = NULL;
      if (parser_match(parser, TOK_REGISTER)) {
        lexer_next_token(parser->lexer);
        reg = parse_expression(parser);
      }

      /* Array read logic: read i2c device addr register reg count N into arr */
      if (parser_match_id(parser, "count")) {
        lexer_next_token(parser->lexer);
        ASTNode *count = parse_expression(parser);
        if (!parser_match_id(parser, "into")) {
          error_report(parser->errors, ERROR_SYNTAX,
                       parser->lexer->current_token.line,
                       parser->lexer->current_token.column,
                       "Expected 'into' after count expression");
        } else {
          lexer_next_token(parser->lexer);
        }
        Token arr_tok = parser->lexer->current_token;
        arr_tok.value = strdup(arr_tok.value);
        parser_expect(parser, TOK_ID);
        return ast_i2c_device_read_array(addr, reg, count, arr_tok.value);
      }

      if (reg)
        return ast_i2c_device_read(addr, reg);
      return ast_i2c_read(addr);
    }

    /* receive serial (V3.0) */
    if (parser_match(parser, TOK_SERIAL)) {
      lexer_next_token(parser->lexer);
      return ast_serial_recv();
    }

    /* distance logic moved below */

    /* read temperature */
    if (parser_match_id(parser, "temperature")) {
      lexer_next_token(parser->lexer);
      return ast_dht_read_temp();
    }

    /* read humidity */
    if (parser_match_id(parser, "humidity")) {
      lexer_next_token(parser->lexer);
      return ast_dht_read_humid();
    }

    /* --- Wave 4: Navigation & Storage Reads --- */
    /* read accel x | y | z */
    if (parser_match(parser, TOK_ACCEL)) {
      lexer_next_token(parser->lexer);
      if (parser_match_id(parser, "x")) {
        lexer_next_token(parser->lexer);
        return ast_imu_read_x();
      }
      if (parser_match_id(parser, "y")) {
        lexer_next_token(parser->lexer);
        return ast_imu_read_y();
      }
      if (parser_match_id(parser, "z")) {
        lexer_next_token(parser->lexer);
        return ast_imu_read_z();
      }
    }

    /* read gyro x | y | z */
    if (parser_match(parser, TOK_GYRO)) {
      lexer_next_token(parser->lexer);
      if (parser_match_id(parser, "x")) {
        lexer_next_token(parser->lexer);
        return ast_imu_read_x();
      }
      if (parser_match_id(parser, "y")) {
        lexer_next_token(parser->lexer);
        return ast_imu_read_y();
      }
      if (parser_match_id(parser, "z")) {
        lexer_next_token(parser->lexer);
        return ast_imu_read_z();
      }
    }

    /* read orientation */
    if (parser_match(parser, TOK_ORIENTATION)) {
      lexer_next_token(parser->lexer);
      return ast_imu_orient();
    }

    /* read latitude | longitude | altitude | speed */
    if (parser_match(parser, TOK_LATITUDE)) {
      lexer_next_token(parser->lexer);
      return ast_gps_read_lat();
    }
    if (parser_match(parser, TOK_LONGITUDE)) {
      lexer_next_token(parser->lexer);
      return ast_gps_read_lon();
    }
    if (parser_match(parser, TOK_ALTITUDE)) {
      lexer_next_token(parser->lexer);
      return ast_gps_read_alt();
    }
    if (parser_match_id(parser, "speed")) {
      lexer_next_token(parser->lexer);
      return ast_gps_read_spd();
    }

    /* read distance precise */
    if (parser_match_id(parser, "distance")) {
      // Must check if it's 'read distance precise' or 'read distance trigger N
      // echo M'
      lexer_next_token(parser->lexer);
      if (parser_match(parser, TOK_PRECISE)) {
        lexer_next_token(parser->lexer);
        return ast_lidar_read();
      } else {
        /* expect 'trigger' */
        if (parser_match_id(parser, "trigger"))
          lexer_next_token(parser->lexer);
        ASTNode *trigger = parse_expression(parser);
        /* expect 'echo' */
        if (parser_match_id(parser, "echo"))
          lexer_next_token(parser->lexer);
        ASTNode *echo = parse_expression(parser);
        return ast_distance_read(trigger, echo);
      }
    }

    /* read file */
    if (parser_match(parser, TOK_FILE)) {
      lexer_next_token(parser->lexer);
      return ast_file_read();
    }

    /* read encoder (Wave 2) */
    if (parser_match(parser, TOK_ENCODER)) {
      lexer_next_token(parser->lexer);
      return ast_encoder_read();
    }
    /* receive spi becomes an error; transfer spi handled below */
    if (parser_match(parser, TOK_ID)) {
      /* read <devicename>  OR  read <devicename> register <reg> */
      Token dev_tok = parser->lexer->current_token;
      dev_tok.value = strdup(dev_tok.value);
      lexer_next_token(parser->lexer);
      ASTNode *reg = NULL;
      if (parser_match(parser, TOK_REGISTER)) {
        lexer_next_token(parser->lexer);
        reg = parse_expression(parser);
      }
      ProtocolType proto = PROTOCOL_UART;
      Symbol *sym = symbol_table_lookup(parser->symbols, dev_tok.value);
      if (sym && sym->kind == SYMBOL_DEVICE)
        proto = sym->protocol;
      return ast_device_read(dev_tok.value, proto, reg);
    }

    // Default: digital read
    parser_expect(parser, TOK_PIN);
    ASTNode *pin = parse_expression(parser);
    return ast_gpio_read(pin);
  }

  /* transfer spi <expr>  (expression) */
  if (parser_match(parser, TOK_TRANSFER)) {
    lexer_next_token(parser->lexer);
    if (parser_match(parser, TOK_SPI))
      lexer_next_token(parser->lexer);
    ASTNode *data = parse_expression(parser);
    return ast_spi_transfer(data);
  }

  /* cast <type> <expr> */
  if (parser_match(parser, TOK_CAST)) {
    lexer_next_token(parser->lexer);
    Type *t = type_float();
    if (parser_match(parser, TOK_INT_KW)) {
      t = type_int();
      lexer_next_token(parser->lexer);
    } else if (parser_match(parser, TOK_FLOAT_KW)) {
      t = type_float();
      lexer_next_token(parser->lexer);
    } else if (parser_match(parser, TOK_BOOL_KW)) {
      t = type_bool();
      lexer_next_token(parser->lexer);
    } else if (parser_match(parser, TOK_BYTE_KW)) {
      t = type_byte();
      lexer_next_token(parser->lexer);
    }
    ASTNode *operand = parse_primary(parser);
    return ast_cast(t, operand);
  }

  /* ============================================================
   * Wave 3: Communication Expression Handlers
   * ============================================================ */

  /* ble receive */
  if (parser_match(parser, TOK_BLE)) {
    lexer_next_token(parser->lexer);
    if (parser_match(parser, TOK_RECEIVE)) {
      lexer_next_token(parser->lexer);
      return ast_ble_receive();
    }
  }

  /* wifi ip */
  if (parser_match(parser, TOK_WIFI)) {
    lexer_next_token(parser->lexer);
    if (parser_match_id(parser, "ip")) {
      lexer_next_token(parser->lexer);
      return ast_wifi_ip();
    }
  }

  /* mqtt read */
  if (parser_match(parser, TOK_MQTT)) {
    lexer_next_token(parser->lexer);
    if (parser_match(parser, TOK_READ)) {
      lexer_next_token(parser->lexer);
      return ast_mqtt_read();
    }
  }

  /* http get "url" */
  if (parser_match(parser, TOK_HTTP)) {
    lexer_next_token(parser->lexer);
    if (parser_match_id(parser, "get")) {
      lexer_next_token(parser->lexer);
      ASTNode *url = parse_expression(parser);
      return ast_http_get(url);
    }
  }

  /* ws receive */
  if (parser_match(parser, TOK_WEBSOCKET)) {
    lexer_next_token(parser->lexer);
    if (parser_match(parser, TOK_RECEIVE)) {
      lexer_next_token(parser->lexer);
      return ast_ws_receive();
    }
  }

  // not expr (Standalone, TOK_NOT is its own token type)
  if (parser_match(parser, TOK_NOT)) {
    lexer_next_token(parser->lexer);
    ASTNode *operand = parse_primary(parser);
    return ast_unary_op(OP_NOT, operand);
  }

  error_report(parser->errors, ERROR_SYNTAX, tok.line, tok.column,
               "Unexpected token in expression");
  return ast_number(0); // Error recovery
}

// Level 1: Arithmetic (highest precedence): + - * / %
static ASTNode *parse_arithmetic(Parser *parser) {
  ASTNode *left = parse_primary(parser);

  while (parser_match(parser, TOK_PLUS) || parser_match(parser, TOK_MINUS) ||
         parser_match(parser, TOK_STAR) || parser_match(parser, TOK_SLASH) ||
         parser_match(parser, TOK_MOD)) {

    Operator op = OP_ADD;
    if (parser_match(parser, TOK_MINUS))
      op = OP_SUB;
    else if (parser_match(parser, TOK_STAR))
      op = OP_MUL;
    else if (parser_match(parser, TOK_SLASH))
      op = OP_DIV;
    else if (parser_match(parser, TOK_MOD))
      op = OP_MOD;

    lexer_next_token(parser->lexer);
    ASTNode *right = parse_primary(parser);
    left = ast_binary_op(op, left, right);
  }

  return left;
}

// Level 2: Comparison (medium precedence): > < >= <= == != is
static ASTNode *parse_comparison(Parser *parser) {
  ASTNode *left = parse_arithmetic(parser);

  while (parser_match(parser, TOK_EQ) || parser_match(parser, TOK_NEQ) ||
         parser_match(parser, TOK_LT) || parser_match(parser, TOK_GT) ||
         parser_match(parser, TOK_LTE) || parser_match(parser, TOK_GTE) ||
         parser_match(parser, TOK_IS)) {

    Operator op = OP_EQ;
    if (parser_match(parser, TOK_NEQ))
      op = OP_NEQ;
    else if (parser_match(parser, TOK_LTE))
      op = OP_LTE;
    else if (parser_match(parser, TOK_GTE))
      op = OP_GTE;
    else if (parser_match(parser, TOK_LT))
      op = OP_LT;
    else if (parser_match(parser, TOK_GT))
      op = OP_GT;

    lexer_next_token(parser->lexer);
    ASTNode *right = parse_arithmetic(parser);
    left = ast_binary_op(op, left, right);
  }

  return left;
}

// Level 3: Logical (lowest precedence): and or
static ASTNode *parse_expression(Parser *parser) {
  ASTNode *left = parse_comparison(parser);

  while (parser_match(parser, TOK_AND) || parser_match(parser, TOK_OR)) {

    Operator op = OP_AND;
    if (parser_match(parser, TOK_OR))
      op = OP_OR;

    lexer_next_token(parser->lexer);
    ASTNode *right = parse_comparison(parser);
    left = ast_binary_op(op, left, right);
  }

  return left;
}

// Parse statement
static ASTNode *parse_statement(Parser *parser) {
  /* ---- make … (all forms) ---- */
  if (parser_match(parser, TOK_MAKE)) {
    lexer_next_token(parser->lexer);

    /* make array <type> <name>[<size>]    OR
       make array <name>[<size>] of <type> */
    if (parser_match(parser, TOK_ARRAY)) {
      lexer_next_token(parser->lexer);
      /* optional type keyword */
      Type *elem_type = type_float();
      if (parser_match(parser, TOK_INT_KW)) {
        elem_type = type_int();
        lexer_next_token(parser->lexer);
      } else if (parser_match(parser, TOK_FLOAT_KW)) {
        elem_type = type_float();
        lexer_next_token(parser->lexer);
      } else if (parser_match(parser, TOK_BOOL_KW)) {
        elem_type = type_bool();
        lexer_next_token(parser->lexer);
      } else if (parser_match(parser, TOK_BYTE_KW)) {
        elem_type = type_byte();
        lexer_next_token(parser->lexer);
      }
      Token name_tok = parser->lexer->current_token;
      name_tok.value = strdup(name_tok.value);
      parser_expect(parser, TOK_ID);
      int sz = 0;
      if (parser_match(parser, TOK_LBRACKET)) {
        /* make array name[N] syntax */
        lexer_next_token(parser->lexer);
        Token sz_tok = parser->lexer->current_token;
        sz_tok.value = strdup(sz_tok.value);
        sz = (int)atof(sz_tok.value);
        lexer_next_token(parser->lexer);
        parser_expect(parser, TOK_RBRACKET);
      } else if (parser_match_id(parser, "size")) {
        /* make array name size N syntax */
        lexer_next_token(parser->lexer);
        Token sz_tok = parser->lexer->current_token;
        sz_tok.value = strdup(sz_tok.value);
        sz = (int)atof(sz_tok.value);
        lexer_next_token(parser->lexer);
      }
      /* optional "of <type>" */
      if (parser_match(parser, TOK_OF)) {
        lexer_next_token(parser->lexer);
        if (parser_match(parser, TOK_INT_KW)) {
          elem_type = type_int();
          lexer_next_token(parser->lexer);
        } else if (parser_match(parser, TOK_FLOAT_KW)) {
          elem_type = type_float();
          lexer_next_token(parser->lexer);
        } else if (parser_match(parser, TOK_BOOL_KW)) {
          elem_type = type_bool();
          lexer_next_token(parser->lexer);
        } else if (parser_match(parser, TOK_BYTE_KW)) {
          elem_type = type_byte();
          lexer_next_token(parser->lexer);
        }
      }
      return ast_array_decl(name_tok.value, elem_type, sz);
    }

    /* make buffer <type> <name>[<size>]   OR
       make buffer <name>[<size>] of <type> */
    if (parser_match(parser, TOK_BUFFER)) {
      lexer_next_token(parser->lexer);
      Type *elem_type = type_float();
      if (parser_match(parser, TOK_INT_KW)) {
        elem_type = type_int();
        lexer_next_token(parser->lexer);
      } else if (parser_match(parser, TOK_FLOAT_KW)) {
        elem_type = type_float();
        lexer_next_token(parser->lexer);
      } else if (parser_match(parser, TOK_BOOL_KW)) {
        elem_type = type_bool();
        lexer_next_token(parser->lexer);
      } else if (parser_match(parser, TOK_BYTE_KW)) {
        elem_type = type_byte();
        lexer_next_token(parser->lexer);
      }
      Token name_tok = parser->lexer->current_token;
      name_tok.value = strdup(name_tok.value);
      parser_expect(parser, TOK_ID);
      parser_expect(parser, TOK_LBRACKET);
      Token sz_tok = parser->lexer->current_token;
      sz_tok.value = strdup(sz_tok.value);
      int sz = (int)atof(sz_tok.value);
      lexer_next_token(parser->lexer);
      parser_expect(parser, TOK_RBRACKET);
      if (parser_match(parser, TOK_OF)) {
        lexer_next_token(parser->lexer);
        if (parser_match(parser, TOK_INT_KW)) {
          elem_type = type_int();
          lexer_next_token(parser->lexer);
        } else if (parser_match(parser, TOK_FLOAT_KW)) {
          elem_type = type_float();
          lexer_next_token(parser->lexer);
        } else if (parser_match(parser, TOK_BOOL_KW)) {
          elem_type = type_bool();
          lexer_next_token(parser->lexer);
        } else if (parser_match(parser, TOK_BYTE_KW)) {
          elem_type = type_byte();
          lexer_next_token(parser->lexer);
        }
      }
      return ast_buffer_decl(name_tok.value, elem_type, sz);
    }

    /* make var <name> = <expr>   (legacy) */
    if (parser_match(parser, TOK_VAR)) {
      lexer_next_token(parser->lexer);
      Token name_tok = parser->lexer->current_token;
      name_tok.value = strdup(name_tok.value);
      parser_expect(parser, TOK_ID);
      ASTNode *init = NULL;
      if (parser_match(parser, TOK_ASSIGN)) {
        lexer_next_token(parser->lexer);
        init = parse_expression(parser);
      }
      symbol_table_add(parser->symbols, name_tok.value, SYMBOL_VARIABLE,
                       type_inferred(), name_tok.line);
      return ast_var_decl(name_tok.value, NULL, init);
    }

    /* make int|float|bool|byte <name> = <expr>   (V3.0 explicit type) */
    Type *decl_type = NULL;
    if (parser_match(parser, TOK_INT_KW)) {
      decl_type = type_int();
      lexer_next_token(parser->lexer);
    } else if (parser_match(parser, TOK_FLOAT_KW)) {
      decl_type = type_float();
      lexer_next_token(parser->lexer);
    } else if (parser_match(parser, TOK_BOOL_KW)) {
      decl_type = type_bool();
      lexer_next_token(parser->lexer);
    } else if (parser_match(parser, TOK_BYTE_KW)) {
      decl_type = type_byte();
      lexer_next_token(parser->lexer);
    } else if (parser_match_id(parser, "string")) {
      decl_type = type_string();
      lexer_next_token(parser->lexer);
    }

    if (decl_type != NULL) {
      /* might be: make float name = expr   OR   make float ratio = cast float
       * speed/255 */
      Token name_tok = parser->lexer->current_token;
      name_tok.value = strdup(name_tok.value);
      parser_expect(parser, TOK_ID);
      ASTNode *init = NULL;
      if (parser_match(parser, TOK_ASSIGN)) {
        lexer_next_token(parser->lexer);
        init = parse_expression(parser);
      }
      symbol_table_add(parser->symbols, name_tok.value, SYMBOL_VARIABLE,
                       decl_type, name_tok.line);
      return ast_var_decl(name_tok.value, decl_type, init);
    }

    /* make <StructTypeName> <varname>  (struct instantiation) */
    if (parser_match(parser, TOK_ID)) {
      Token type_tok = parser->lexer->current_token;
      type_tok.value = strdup(type_tok.value);
      lexer_next_token(parser->lexer);
      /* If next token is also an ID, it's a struct instance */
      if (parser_match(parser, TOK_ID)) {
        Token var_tok = parser->lexer->current_token;
        var_tok.value = strdup(var_tok.value);
        lexer_next_token(parser->lexer);
        return ast_struct_instance(type_tok.value, var_tok.value);
      }
      /* Otherwise it was make <name> = expr (no type) — legacy compat */
      ASTNode *init = NULL;
      if (parser_match(parser, TOK_ASSIGN)) {
        lexer_next_token(parser->lexer);
        init = parse_expression(parser);
      }
      return ast_var_decl(type_tok.value, NULL, init);
    }

    error_report(parser->errors, ERROR_SYNTAX,
                 parser->lexer->current_token.line,
                 parser->lexer->current_token.column,
                 "Expected var/int/float/bool/byte/array/buffer or identifier "
                 "after 'make'");
    return NULL;
  }

  // Assignment: set x to 5  / set x[i] to 5  / set x.field to 5
  if (parser_match(parser, TOK_SET)) {
    lexer_next_token(parser->lexer);

    ASTNode *target;
    /* set pixel N to R G B */
    if (parser_match_id(parser, "pixel")) {
      lexer_next_token(parser->lexer);
      ASTNode *index = parse_expression(parser);
      parser_expect(parser, TOK_TO);
      ASTNode *r = parse_expression(parser);
      ASTNode *g = parse_expression(parser);
      ASTNode *b = parse_expression(parser);
      return ast_neopixel_set(index, r, g, b);
    }

    /* --- Wave 2 Wrappers --- */
    if (parser_match(parser, TOK_STEPPER)) {
      lexer_next_token(parser->lexer);
      if (!parser_match_id(parser, "speed")) {
        error_report(parser->errors, ERROR_SYNTAX,
                     parser->lexer->current_token.line,
                     parser->lexer->current_token.column, "Expected 'speed'");
      } else
        lexer_next_token(parser->lexer);
      ASTNode *speed = parse_expression(parser);
      return ast_stepper_speed(speed);
    }
    if (parser_match(parser, TOK_ESC)) {
      lexer_next_token(parser->lexer);
      if (!parser_match_id(parser, "throttle")) {
        error_report(
            parser->errors, ERROR_SYNTAX, parser->lexer->current_token.line,
            parser->lexer->current_token.column, "Expected 'throttle'");
      } else
        lexer_next_token(parser->lexer);
      ASTNode *throttle = parse_expression(parser);
      return ast_esc_throttle(throttle);
    }
    if (parser_match(parser, TOK_PID)) {
      lexer_next_token(parser->lexer);
      if (!parser_match_id(parser, "target")) {
        error_report(parser->errors, ERROR_SYNTAX,
                     parser->lexer->current_token.line,
                     parser->lexer->current_token.column, "Expected 'target'");
      } else
        lexer_next_token(parser->lexer);
      ASTNode *t = parse_expression(parser);
      return ast_pid_target(t);
    }

    if (parser_match(parser, TOK_PIN)) {
      lexer_next_token(parser->lexer);
      ASTNode *pin = parse_expression(parser);
      parser_expect(parser, TOK_TO);
      ASTNode *value = parse_expression(parser);
      return ast_analog_write(pin, value);
    } else if (parser_match_id(parser, "index")) {
      /* legacy: set index N of arrname to val */
      lexer_next_token(parser->lexer);
      Token arr_tok = parser->lexer->current_token;
      arr_tok.value = strdup(arr_tok.value);
      parser_expect(parser, TOK_ID);
      parser_expect(parser, TOK_OF);
      ASTNode *index = parse_expression(parser);
      target = ast_array_access(ast_identifier(arr_tok.value), index);
    } else {
      /* Common case: consume identifier name */
      Token var_tok = parser->lexer->current_token;
      var_tok.value = strdup(var_tok.value);
      parser_expect(parser, TOK_ID);
      target = ast_identifier(var_tok.value);

      /* V3.0: set name[i] to val  — array element */
      if (parser_match(parser, TOK_LBRACKET)) {
        lexer_next_token(parser->lexer);
        ASTNode *idx = parse_expression(parser);

        parser_expect(parser, TOK_RBRACKET);
        target = ast_array_access(target, idx);
      }

      /* V3.0: set name.field to val  — struct member (possibly chained) */
      while (parser_match(parser, TOK_DOT)) {
        lexer_next_token(parser->lexer);
        Token member_tok = parser->lexer->current_token;
        member_tok.value = strdup(member_tok.value);
        lexer_next_token(parser->lexer);
        target = ast_struct_access(target, member_tok.value);
      }
    }

    parser_expect(parser, TOK_TO);
    ASTNode *value = parse_expression(parser);
    return ast_assignment(target, value);
  }

  // Change statement: change x by 5  /  change x.field by 5  /  change x[i] by
  // 5
  if (parser_match(parser, TOK_CHANGE)) {
    lexer_next_token(parser->lexer);

    Token var_tok = parser->lexer->current_token;
    var_tok.value = strdup(var_tok.value);
    parser_expect(parser, TOK_ID);
    ASTNode *target = ast_identifier(var_tok.value);

    /* V3.0: change name[i] by delta */
    if (parser_match(parser, TOK_LBRACKET)) {
      lexer_next_token(parser->lexer);
      ASTNode *idx = parse_expression(parser);
      parser_expect(parser, TOK_RBRACKET);
      target = ast_array_access(target, idx);
    }
    /* V3.0: change name.field by delta */
    while (parser_match(parser, TOK_DOT)) {
      lexer_next_token(parser->lexer);
      Token member_tok = parser->lexer->current_token;
      member_tok.value = strdup(member_tok.value);
      lexer_next_token(parser->lexer);
      target = ast_struct_access(target, member_tok.value);
    }

    parser_expect(parser, TOK_BY);
    ASTNode *delta = parse_expression(parser);

    /* Clone target manually for the right side of OP_ADD to prevent double-free
     * in AST destruction */
    ASTNode *target_clone = clone_target(target);

    /* Generate: target = target + delta */
    ASTNode *add_expr = ast_binary_op(OP_ADD, target_clone, delta);
    return ast_assignment(target, add_expr);
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

    if (parser_match(parser, TOK_MS_TOK)) {
      lexer_next_token(parser->lexer);
    } else if (parser_match(parser, TOK_US_TOK)) {
      lexer_next_token(parser->lexer);
      ASTNode **args = malloc(sizeof(ASTNode *));
      args[0] = duration;
      return ast_call("delayMicroseconds", args, 1);
    } else if (parser_match(parser, TOK_ID)) {
      const char *unit = parser->lexer->current_token.value;
      if (strcmp(unit, "seconds") == 0 || strcmp(unit, "s") == 0 ||
          strcmp(unit, "sec") == 0 || strcmp(unit, "second") == 0) {
        duration = ast_binary_op(OP_MUL, duration, ast_number(1000));
        lexer_next_token(parser->lexer);
      } else if (strcmp(unit, "milliseconds") == 0 || strcmp(unit, "ms") == 0 ||
                 strcasecmp(unit, "millisecond") == 0) {
        lexer_next_token(parser->lexer);
      } else if (strcmp(unit, "microseconds") == 0 || strcmp(unit, "us") == 0 ||
                 strcasecmp(unit, "microsecond") == 0) {
        lexer_next_token(parser->lexer);
        ASTNode **args = malloc(sizeof(ASTNode *));
        args[0] = duration;
        return ast_call("delayMicroseconds", args, 1);
      }
    }
    return ast_wait(duration);
  }

  // Wait Microseconds
  if (parser_match(parser, TOK_WAIT_US)) {
    lexer_next_token(parser->lexer);
    ASTNode *duration = parse_expression(parser);
    ASTNode **args = malloc(sizeof(ASTNode *));
    args[0] = duration;
    return ast_call("delayMicroseconds", args, 1);
  }

  // Print / Println
  if (parser->lexer->current_token.type == TOK_PRINT ||
      parser->lexer->current_token.type == TOK_PRINTLN) {
    int is_ln = parser->lexer->current_token.type == TOK_PRINTLN;
    lexer_next_token(parser->lexer);
    ASTNode *value = parse_expression(parser);
    return is_ln ? ast_println_stmt(value) : ast_print_stmt(value);
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
        ASTNode **stmts = malloc(sizeof(ASTNode *));
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

    Token lang_tok = parser->lexer->current_token;
    lang_tok.value = strdup(lang_tok.value);
    parser_expect(parser, TOK_STRING_LIT);

    parser_expect(parser, TOK_DEF);
    Token name_tok = parser->lexer->current_token;
    name_tok.value = strdup(name_tok.value);
    parser_expect(parser, TOK_ID);
    parser_expect(parser, TOK_LPAREN);

    int param_capacity = 4;
    char **param_names = malloc(sizeof(char *) * param_capacity);
    Type **param_types = malloc(sizeof(Type *) * param_capacity);
    int param_count = 0;

    while (!parser_match(parser, TOK_RPAREN) &&
           !parser_match(parser, TOK_EOF)) {
      Token param_tok = parser->lexer->current_token;
      param_tok.value = strdup(param_tok.value);
      parser_expect(parser, TOK_ID);

      Type *ptype = type_float(); /* default */
      if (parser_match(parser, TOK_COLON)) {
        lexer_next_token(parser->lexer);
        if (parser_match(parser, TOK_ARRAY)) {
          lexer_next_token(parser->lexer);
          Type *elem_type = type_float();
          if (parser_match(parser, TOK_INT_KW)) {
            elem_type = type_int();
            lexer_next_token(parser->lexer);
          } else if (parser_match(parser, TOK_FLOAT_KW)) {
            elem_type = type_float();
            lexer_next_token(parser->lexer);
          }
          ptype = type_array(elem_type, -1);
        } else if (parser_match(parser, TOK_INT_KW)) {
          ptype = type_int();
          lexer_next_token(parser->lexer);
        } else if (parser_match(parser, TOK_FLOAT_KW)) {
          ptype = type_float();
          lexer_next_token(parser->lexer);
        } else if (parser_match(parser, TOK_BOOL_KW)) {
          ptype = type_bool();
          lexer_next_token(parser->lexer);
        } else if (parser_match(parser, TOK_BYTE_KW)) {
          ptype = type_byte();
          lexer_next_token(parser->lexer);
        }
      }

      if (param_count >= param_capacity) {
        param_capacity *= 2;
        param_names = realloc(param_names, sizeof(char *) * param_capacity);
        param_types = realloc(param_types, sizeof(Type *) * param_capacity);
      }
      param_names[param_count] = strdup(param_tok.value);
      param_types[param_count] = ptype;
      param_count++;
      if (parser_match(parser, TOK_COMMA)) {
        lexer_next_token(parser->lexer);
      }
    }
    parser_expect(parser, TOK_RPAREN);

    Type *ret_type = type_void();
    if (parser_match(parser, TOK_ARROW)) {
      lexer_next_token(parser->lexer);
      if (parser_match(parser, TOK_ARRAY)) {
        lexer_next_token(parser->lexer);
        Type *elem_type = type_float();
        if (parser_match(parser, TOK_INT_KW)) {
          elem_type = type_int();
          lexer_next_token(parser->lexer);
        } else if (parser_match(parser, TOK_FLOAT_KW)) {
          elem_type = type_float();
          lexer_next_token(parser->lexer);
        }
        ret_type = type_array(elem_type, -1);
      } else if (parser_match(parser, TOK_INT_KW)) {
        ret_type = type_int();
        lexer_next_token(parser->lexer);
      } else if (parser_match(parser, TOK_FLOAT_KW)) {
        ret_type = type_float();
        lexer_next_token(parser->lexer);
      }
    }

    return ast_extern_function_def(name_tok.value, param_names, param_types,
                                   param_count, ret_type, lang_tok.value);
  }

  // Function definition: def name(param1, param2) { }
  if (parser_match(parser, TOK_DEF)) {
    lexer_next_token(parser->lexer);
    Token name_tok = parser->lexer->current_token;
    name_tok.value = strdup(name_tok.value);
    parser_expect(parser, TOK_ID);
    parser_expect(parser, TOK_LPAREN);

    int param_capacity = 4;
    char **param_names = malloc(sizeof(char *) * param_capacity);
    Type **param_types = malloc(sizeof(Type *) * param_capacity);
    int param_count = 0;

    while (!parser_match(parser, TOK_RPAREN) &&
           !parser_match(parser, TOK_EOF)) {
      /* Optional type annotation before parameter name */
      Type *ptype = type_float(); /* default */
      if (parser_match(parser, TOK_INT_KW)) {
        ptype = type_int();
        lexer_next_token(parser->lexer);
      } else if (parser_match(parser, TOK_FLOAT_KW)) {
        ptype = type_float();
        lexer_next_token(parser->lexer);
      } else if (parser_match(parser, TOK_BOOL_KW)) {
        ptype = type_bool();
        lexer_next_token(parser->lexer);
      } else if (parser_match(parser, TOK_BYTE_KW)) {
        ptype = type_byte();
        lexer_next_token(parser->lexer);
      }

      Token param_tok = parser->lexer->current_token;
      param_tok.value = strdup(param_tok.value);
      if (!parser_match(parser, TOK_ID)) {
        error_report(parser->errors, ERROR_SYNTAX,
                     parser->lexer->current_token.line,
                     parser->lexer->current_token.column,
                     "Expected parameter name in function definition");
        break;
      }
      lexer_next_token(parser->lexer);
      if (param_count >= param_capacity) {
        param_capacity *= 2;
        param_names = realloc(param_names, sizeof(char *) * param_capacity);
        param_types = realloc(param_types, sizeof(Type *) * param_capacity);
      }
      param_names[param_count] = strdup(param_tok.value);
      param_types[param_count] = ptype;
      param_count++;
      if (parser_match(parser, TOK_COMMA))
        lexer_next_token(parser->lexer);
    }
    parser_expect(parser, TOK_RPAREN);
    parser_expect(parser, TOK_LBRACE);
    parser->in_function++;
    symbol_table_enter_scope(parser->symbols);
    ASTNode *body = parse_block(parser);
    symbol_table_exit_scope(parser->symbols);
    parser->in_function--;
    parser_expect(parser, TOK_RBRACE);

    return ast_function_def(name_tok.value, param_names, param_types,
                            param_count, type_void(), body);
  }

  // Loop (forever or N times)
  if (parser_match(parser, TOK_LOOP)) {
    lexer_next_token(parser->lexer);
    if (parser_match(parser, TOK_FOREVER)) {
      lexer_next_token(parser->lexer);
      parser_expect(parser, TOK_LBRACE);
      parser->in_loop++;
      ASTNode *body = parse_block(parser);
      parser->in_loop--;
      parser_expect(parser, TOK_RBRACE);
      return ast_forever(body);
    } else {
      ASTNode *count = parse_expression(parser);
      if (parser_match(parser, TOK_ID) &&
          strcmp(parser->lexer->current_token.value, "times") == 0) {
        lexer_next_token(parser->lexer);
      }
      parser_expect(parser, TOK_LBRACE);
      parser->in_loop++;
      ASTNode *body = parse_block(parser);
      parser->in_loop--;
      parser_expect(parser, TOK_RBRACE);
      return ast_repeat(count, body);
    }
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

  // For logic: for var from start to end { body }
  if (parser_match(parser, TOK_FOR)) {
    lexer_next_token(parser->lexer);
    Token var_tok = parser->lexer->current_token;
    var_tok.value = strdup(var_tok.value);
    parser_expect(parser, TOK_ID);
    parser_expect(parser, TOK_FROM);
    ASTNode *start_expr = parse_expression(parser);
    parser_expect(parser, TOK_TO);
    ASTNode *end_expr = parse_expression(parser);

    ASTNode *step_expr = NULL;
    if (parser_match(parser, TOK_BY)) {
      lexer_next_token(parser->lexer);
      step_expr = parse_expression(parser);
    }

    parser_expect(parser, TOK_LBRACE);

    symbol_table_enter_scope(parser->symbols);
    symbol_table_add(parser->symbols, var_tok.value, SYMBOL_VARIABLE,
                     type_int(), var_tok.line);

    // Suppress unused warning since iterating a loop counts as semantic
    // utilization
    symbol_table_lookup_current_scope(parser->symbols, var_tok.value)->is_used =
        1;

    parser->in_loop++;
    ASTNode *body = parse_block(parser);
    parser->in_loop--;

    symbol_table_exit_scope(parser->symbols);

    parser_expect(parser, TOK_RBRACE);
    return ast_for(var_tok.value, start_expr, end_expr, step_expr, body);
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

  // Continue
  if (parser_match(parser, TOK_CONTINUE)) {
    lexer_next_token(parser->lexer);
    return ast_continue();
  }

  /* ============================================================
   * Library Wrapper Statement Handlers
   * ============================================================ */

  /* attach servo pin N */
  /* attach dht11 pin N  /  attach dht22 pin N */
  /* attach strip pin N count C */
  /* attach lcd columns C rows R */
  if (parser_match_id(parser, "attach")) {
    lexer_next_token(parser->lexer);

    /* --- Wave 1 Wrappers --- */
    if (parser_match(parser, TOK_SERVO)) {
      lexer_next_token(parser->lexer);
      parser_expect(parser, TOK_PIN);
      ASTNode *pin = parse_expression(parser);
      return ast_servo_attach(pin);
    }
    if (parser_match_id(parser, "dht11") || parser_match_id(parser, "dht22")) {
      int type = parser_match_id(parser, "dht11") ? 11 : 22;
      lexer_next_token(parser->lexer);
      parser_expect(parser, TOK_PIN);
      ASTNode *pin = parse_expression(parser);
      return ast_dht_attach(pin, type);
    }
    if (parser_match_id(parser, "strip")) {
      lexer_next_token(parser->lexer);
      parser_expect(parser, TOK_PIN);
      ASTNode *pin = parse_expression(parser);
      if (!parser_match_id(parser, "count")) {
        error_report(parser->errors, ERROR_SYNTAX,
                     parser->lexer->current_token.line,
                     parser->lexer->current_token.column, "Expected 'count'");
      } else {
        lexer_next_token(parser->lexer);
      }
      ASTNode *count = parse_expression(parser);
      return ast_neopixel_init(pin, count);
    }
    if (parser_match_id(parser, "lcd")) {
      lexer_next_token(parser->lexer);
      if (!parser_match_id(parser, "columns")) {
        error_report(parser->errors, ERROR_SYNTAX,
                     parser->lexer->current_token.line,
                     parser->lexer->current_token.column, "Expected 'columns'");
      } else {
        lexer_next_token(parser->lexer);
      }
      ASTNode *cols = parse_expression(parser);
      if (!parser_match_id(parser, "rows")) {
        error_report(parser->errors, ERROR_SYNTAX,
                     parser->lexer->current_token.line,
                     parser->lexer->current_token.column, "Expected 'rows'");
      } else {
        lexer_next_token(parser->lexer);
      }
      ASTNode *rows = parse_expression(parser);
      return ast_lcd_init(cols, rows);
    }

    /* --- Wave 2 Wrappers --- */
    if (parser_match(parser, TOK_STEPPER)) {
      lexer_next_token(parser->lexer);
      if (!parser_match_id(parser, "step")) {
        error_report(parser->errors, ERROR_SYNTAX,
                     parser->lexer->current_token.line,
                     parser->lexer->current_token.column, "Expected 'step'");
      } else {
        lexer_next_token(parser->lexer);
      }
      ASTNode *step_pin = parse_expression(parser);
      if (!parser_match_id(parser, "dir")) {
        error_report(parser->errors, ERROR_SYNTAX,
                     parser->lexer->current_token.line,
                     parser->lexer->current_token.column, "Expected 'dir'");
      } else {
        lexer_next_token(parser->lexer);
      }
      ASTNode *dir_pin = parse_expression(parser);
      return ast_stepper_attach(step_pin, dir_pin);
    }
    if (parser_match(parser, TOK_MOTOR)) {
      lexer_next_token(parser->lexer);
      if (!parser_match_id(parser, "enable") &&
          !parser_match(parser, TOK_ENABLE)) {
        error_report(parser->errors, ERROR_SYNTAX,
                     parser->lexer->current_token.line,
                     parser->lexer->current_token.column, "Expected 'enable'");
      } else {
        lexer_next_token(parser->lexer);
      }
      ASTNode *en_pin = parse_expression(parser);
      if (!parser_match_id(parser, "forward")) {
        error_report(parser->errors, ERROR_SYNTAX,
                     parser->lexer->current_token.line,
                     parser->lexer->current_token.column, "Expected 'forward'");
      } else {
        lexer_next_token(parser->lexer);
      }
      ASTNode *fwd_pin = parse_expression(parser);
      if (!parser_match_id(parser, "reverse")) {
        error_report(parser->errors, ERROR_SYNTAX,
                     parser->lexer->current_token.line,
                     parser->lexer->current_token.column, "Expected 'reverse'");
      } else {
        lexer_next_token(parser->lexer);
      }
      ASTNode *rev_pin = parse_expression(parser);
      return ast_motor_attach(en_pin, fwd_pin, rev_pin);
    }
    if (parser_match(parser, TOK_ENCODER)) {
      lexer_next_token(parser->lexer);
      if (!parser_match_id(parser, "pin_a")) {
        error_report(parser->errors, ERROR_SYNTAX,
                     parser->lexer->current_token.line,
                     parser->lexer->current_token.column, "Expected 'pin_a'");
      } else {
        lexer_next_token(parser->lexer);
      }
      ASTNode *pin_a = parse_expression(parser);
      if (!parser_match_id(parser, "pin_b")) {
        error_report(parser->errors, ERROR_SYNTAX,
                     parser->lexer->current_token.line,
                     parser->lexer->current_token.column, "Expected 'pin_b'");
      } else {
        lexer_next_token(parser->lexer);
      }
      ASTNode *pin_b = parse_expression(parser);
      return ast_encoder_attach(pin_a, pin_b);
    }
    if (parser_match(parser, TOK_ESC)) {
      lexer_next_token(parser->lexer);
      parser_expect(parser, TOK_PIN);
      ASTNode *pin = parse_expression(parser);
      return ast_esc_attach(pin);
    }
    /* Wave 4 Wrappers */
    if (parser_match(parser, TOK_IMU)) {
      lexer_next_token(parser->lexer);
      ASTNode *port = NULL;
      if (parser_match(parser, TOK_I2C) || parser_match(parser, TOK_SPI)) {
        port = ast_string(parser->lexer->current_token.value);
        lexer_next_token(parser->lexer);
      } else {
        port = parse_expression(parser);
      }
      return ast_imu_attach(port);
    }
    if (parser_match(parser, TOK_GPS)) {
      lexer_next_token(parser->lexer);
      ASTNode *port = NULL;
      if (parser_match(parser, TOK_SERIAL)) {
        port = ast_string(parser->lexer->current_token.value);
        lexer_next_token(parser->lexer);
      } else {
        port = parse_expression(parser);
      }
      ASTNode *baud = parse_expression(parser); /* Expect 9600 */
      return ast_gps_attach(port, baud);
    }
    if (parser_match(parser, TOK_LIDAR)) {
      lexer_next_token(parser->lexer);
      ASTNode *port = NULL;
      if (parser_match(parser, TOK_I2C) || parser_match(parser, TOK_SPI)) {
        port = ast_string(parser->lexer->current_token.value);
        lexer_next_token(parser->lexer);
      } else {
        port = parse_expression(parser);
      }
      return ast_lidar_attach(port);
    }
    if (parser_match(parser, TOK_PID)) {
      lexer_next_token(parser->lexer);
      if (!parser_match_id(parser, "kp")) {
        error_report(parser->errors, ERROR_SYNTAX,
                     parser->lexer->current_token.line,
                     parser->lexer->current_token.column, "Expected 'kp'");
      } else {
        lexer_next_token(parser->lexer);
      }
      ASTNode *kp = parse_expression(parser);
      if (!parser_match_id(parser, "ki")) {
        error_report(parser->errors, ERROR_SYNTAX,
                     parser->lexer->current_token.line,
                     parser->lexer->current_token.column, "Expected 'ki'");
      } else {
        lexer_next_token(parser->lexer);
      }
      ASTNode *ki = parse_expression(parser);
      if (!parser_match_id(parser, "kd")) {
        error_report(parser->errors, ERROR_SYNTAX,
                     parser->lexer->current_token.line,
                     parser->lexer->current_token.column, "Expected 'kd'");
      } else {
        lexer_next_token(parser->lexer);
      }
      ASTNode *kd = parse_expression(parser);
      return ast_pid_attach(kp, ki, kd);
    }
  }

  /* ============================================================
   * Wave 4: Navigation & Storage Statement Handlers
   * ============================================================ */

  /* mount sd pin N */
  if (parser_match_id(parser, "mount")) {
    lexer_next_token(parser->lexer);
    if (parser_match(parser, TOK_SD)) {
      lexer_next_token(parser->lexer);
      if (parser_match(parser, TOK_PIN))
        lexer_next_token(parser->lexer);
      ASTNode *pin = parse_expression(parser);
      return ast_sd_mount(pin);
    }
  }

  /* open file "name" */
  if (parser_match(parser, TOK_OPEN)) {
    // If not matching serial/i2c/spi from Wave 1, check Wave 4 file
    // The previous TOK_OPEN block for serial is far above. We need to be
    // careful. Wait, TOK_OPEN is handled starting at line 2778. If we reach
    // here, we are OUTSIDE the TOK_OPEN block because that block returned NULL
    // if it didn't match serial/i2c/spi. Instead of overriding, we should
    // capture `open file` here. Alternatively, we can let it evaluate. The
    // parser_match(parser, TOK_OPEN) at 2778 consumes the token! We cannot
    // catch `open` here if it was already consumed on line 2778. Let me fix
    // this by injecting the open/write/close file logic as a separate tool call
    // into the TOK_OPEN block. Wait, this block is inside parser_statement but
    // outside the top TOK_OPEN.
  }

  /* write file expr */
  if (parser_match(parser, TOK_WRITE_KW)) {
    // Similarly, write is consumed above at 2838. I must add the file logic
    // directly into the TOK_WRITE_KW block or use a different keyword.
  }

  /* close file */
  if (parser_match_id(parser, "close")) {
    lexer_next_token(parser->lexer);
    if (parser_match(parser, TOK_FILE)) {
      lexer_next_token(parser->lexer);
      return ast_file_close();
    }
  }

  /* move servo/stepper/motor */
  if (parser_match_id(parser, "move")) {
    lexer_next_token(parser->lexer);
    if (parser_match(parser, TOK_SERVO)) {
      lexer_next_token(parser->lexer);
      parser_expect(parser, TOK_TO);
      ASTNode *angle = parse_expression(parser);
      return ast_servo_move(angle);
    }
    if (parser_match(parser, TOK_STEPPER)) {
      lexer_next_token(parser->lexer);
      ASTNode *steps = parse_expression(parser);
      return ast_stepper_move(steps);
    }
    if (parser_match(parser, TOK_MOTOR)) {
      lexer_next_token(parser->lexer);
      int dir = 1; /* Default forward */
      if (parser_match_id(parser, "forward")) {
        dir = 1;
        lexer_next_token(parser->lexer);
      } else if (parser_match_id(parser, "reverse")) {
        dir = -1;
        lexer_next_token(parser->lexer);
      }
      if (parser_match_id(parser, "at")) {
        lexer_next_token(parser->lexer);
      }
      ASTNode *speed = parse_expression(parser);
      return ast_motor_move(dir, speed);
    }
  }

  /* detach servo pin N */
  if (parser_match_id(parser, "detach")) {
    lexer_next_token(parser->lexer);
    if (parser_match(parser, TOK_SERVO) || parser_match_id(parser, "servo")) {
      lexer_next_token(parser->lexer);
      if (parser_match(parser, TOK_PIN))
        lexer_next_token(parser->lexer);
      ASTNode *pin = parse_expression(parser);
      return ast_servo_detach(pin);
    }
  }

  /* stop motor */
  if (parser_match(parser, TOK_STOP)) {
    lexer_next_token(parser->lexer);
    if (parser_match(parser, TOK_I2C)) {
      lexer_next_token(parser->lexer);
      return ast_i2c_stop();
    }
    if (parser_match(parser, TOK_MOTOR)) {
      lexer_next_token(parser->lexer);
      return ast_motor_stop();
    }
  }

  /* show pixels */
  if (parser_match_id(parser, "show")) {
    lexer_next_token(parser->lexer);
    if (parser_match_id(parser, "pixels")) {
      lexer_next_token(parser->lexer);
      return ast_neopixel_show();
    }
  }

  /* clear pixels */
  if (parser_match_id(parser, "clear")) {
    lexer_next_token(parser->lexer);
    if (parser_match_id(parser, "pixels")) {
      lexer_next_token(parser->lexer);
      return ast_neopixel_clear();
    }
  }

  /* reset encoder */
  if (parser_match_id(parser, "reset")) {
    lexer_next_token(parser->lexer);
    if (!parser_match(parser, TOK_ENCODER)) {
      error_report(parser->errors, ERROR_SYNTAX,
                   parser->lexer->current_token.line,
                   parser->lexer->current_token.column, "Expected 'encoder'");
    } else {
      lexer_next_token(parser->lexer);
    }
    return ast_encoder_reset();
  }

  /* lcd print "text" line N  /  lcd clear */
  if (parser_match_id(parser, "lcd")) {
    lexer_next_token(parser->lexer);
    const char *action = parser->lexer->current_token.value;
    if (strcmp(action, "print") == 0) {
      lexer_next_token(parser->lexer);
      ASTNode *text = parse_expression(parser);
      ASTNode *line = NULL;
      if (parser_match_id(parser, "line")) {
        lexer_next_token(parser->lexer);
        line = parse_expression(parser);
      }
      return ast_lcd_print(text, line);
    }
    if (strcmp(action, "clear") == 0) {
      lexer_next_token(parser->lexer);
      return ast_lcd_clear();
    }
  }

  // Handle special identifier-based statements: const
  if (parser_match(parser, TOK_ID)) {
    const char *kw = parser->lexer->current_token.value;

    // Check for 'const' keyword pretending to be an identifier
    if (strcmp(kw, "const") == 0) {
      lexer_next_token(parser->lexer);
      Token name_tok = parser->lexer->current_token;
      name_tok.value = strdup(name_tok.value);
      parser_expect(parser, TOK_ID);
      ASTNode *value = NULL;
      if (parser_match(parser, TOK_ASSIGN)) {
        lexer_next_token(parser->lexer);
        value = parse_expression(parser);
      }
      return ast_var_decl_const(name_tok.value, NULL, value);
    }
  }

  // Function call as statement: name(args...)
  if (parser_match(parser, TOK_ID)) {
    Token name_tok = parser->lexer->current_token;
    name_tok.value = strdup(name_tok.value);
    // Peek ahead: if next token after ID is '(', it's a function call
    lexer_next_token(parser->lexer); // consume the ID
    if (parser_match(parser, TOK_LPAREN)) {
      lexer_next_token(parser->lexer); // consume '('
      int arg_capacity = 16;
      ASTNode **args = malloc(sizeof(ASTNode *) * arg_capacity);
      int arg_count = 0;
      while (!parser_match(parser, TOK_RPAREN) &&
             !parser_match(parser, TOK_EOF)) {
        if (arg_count >= arg_capacity) {
          arg_capacity *= 2;
          args = realloc(args, sizeof(ASTNode *) * arg_capacity);
        }
        args[arg_count++] = parse_expression(parser);
        if (parser_match(parser, TOK_COMMA))
          lexer_next_token(parser->lexer);
      }
      parser_expect(parser, TOK_RPAREN);
      return ast_call(name_tok.value, args, arg_count);
    }
    // Array element assignment: name[index] = expr
    if (parser_match(parser, TOK_LBRACKET)) {
      lexer_next_token(parser->lexer);
      ASTNode *index = parse_expression(parser);
      parser_expect(parser, TOK_RBRACKET);
      ASTNode *target = ast_array_access(ast_identifier(name_tok.value), index);
      // Could be chained: name[i].field = expr
      while (parser_match(parser, TOK_DOT)) {
        lexer_next_token(parser->lexer);
        Token member_tok = parser->lexer->current_token;
        member_tok.value = strdup(member_tok.value);
        lexer_next_token(parser->lexer);
        target = ast_struct_access(target, member_tok.value);
      }
      if (parser_match(parser, TOK_ASSIGN)) {
        lexer_next_token(parser->lexer);
        ASTNode *value = parse_expression(parser);
        return ast_assignment(target, value);
      }
      return target; // expression statement
    }
    // Struct field assignment: name.field = expr
    if (parser_match(parser, TOK_DOT)) {
      ASTNode *target = ast_identifier(name_tok.value);
      while (parser_match(parser, TOK_DOT)) {
        lexer_next_token(parser->lexer);
        Token member_tok = parser->lexer->current_token;
        member_tok.value = strdup(member_tok.value);
        lexer_next_token(parser->lexer);
        target = ast_struct_access(target, member_tok.value);
      }
      if (parser_match(parser, TOK_ASSIGN)) {
        lexer_next_token(parser->lexer);
        ASTNode *value = parse_expression(parser);
        return ast_assignment(target, value);
      }
      return target; // expression statement
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

  /* ============================================================
   * V3.0 Statement Handlers
   * ============================================================ */

  /* make array <name>[<size>] of <type>
     make buffer <name>[<size>] of <type>
     make int|float|bool|byte <name> = <expr>
     make array int|float|bool|byte <name>[<size>]
     shared make <type|var> <name> = <expr>        */
  /* (TOK_MAKE already consumed above for 'make var' — we need to catch
      the other forms BEFORE the var-only version.  The fix: we already
      consumed 'make' + 'var' above, but for new keywords we fall through.
      Insert these handlers BEFORE the existing make-var block by checking
      inside the TOK_MAKE branch.  Since we can't rewrite that block here,
      we handle the MAKE-prefixed forms that weren't caught above.)        */

  /* push <buffer> <expr> */
  if (parser_match(parser, TOK_PUSH)) {
    lexer_next_token(parser->lexer);
    Token buf_tok = parser->lexer->current_token;
    buf_tok.value = strdup(buf_tok.value);
    parser_expect(parser, TOK_ID);
    ASTNode *val = parse_expression(parser);
    return ast_buffer_push(buf_tok.value, val);
  }

  /* on pin N rising/falling/changing { } */
  if (parser_match(parser, TOK_ON)) {
    lexer_next_token(parser->lexer);
    if (parser_match(parser, TOK_PIN)) {
      lexer_next_token(parser->lexer);
      Token pin_tok = parser->lexer->current_token;
      pin_tok.value = strdup(pin_tok.value);
      int pin_num = (int)atof(pin_tok.value);
      lexer_next_token(parser->lexer);
      InterruptMode mode = INT_MODE_RISING;
      if (parser_match(parser, TOK_RISING)) {
        mode = INT_MODE_RISING;
        lexer_next_token(parser->lexer);
      } else if (parser_match(parser, TOK_FALLING)) {
        mode = INT_MODE_FALLING;
        lexer_next_token(parser->lexer);
      } else if (parser_match(parser, TOK_CHANGING)) {
        mode = INT_MODE_CHANGING;
        lexer_next_token(parser->lexer);
      }
      parser_expect(parser, TOK_LBRACE);
      ASTNode *body = parse_block(parser);
      parser_expect(parser, TOK_RBRACE);
      return ast_interrupt_pin(pin_num, mode, body);
    }
    /* on timer every N us/ms { } */
    if (parser_match(parser, TOK_TIMER)) {
      lexer_next_token(parser->lexer);
      /* expect 'every' */
      if (parser_match(parser, TOK_EVERY))
        lexer_next_token(parser->lexer);
      Token interval_tok = parser->lexer->current_token;
      interval_tok.value = strdup(interval_tok.value);
      int interval = (int)atof(interval_tok.value);
      lexer_next_token(parser->lexer);
      int is_us = 1;
      if (parser_match(parser, TOK_MS_TOK)) {
        is_us = 0;
        lexer_next_token(parser->lexer);
      } else if (parser_match(parser, TOK_US_TOK)) {
        is_us = 1;
        lexer_next_token(parser->lexer);
      }
      parser_expect(parser, TOK_LBRACE);
      ASTNode *body = parse_block(parser);
      parser_expect(parser, TOK_RBRACE);
      return ast_interrupt_timer(interval, is_us, body);
    }
    /* on error { } — used inside try/on error: handled by try block */
    error_report(parser->errors, ERROR_SYNTAX,
                 parser->lexer->current_token.line,
                 parser->lexer->current_token.column,
                 "Expected 'pin N' or 'timer' after 'on'");
    return NULL;
  }

  /* open serial at N baud */
  if (parser_match(parser, TOK_OPEN)) {
    lexer_next_token(parser->lexer);
    if (parser_match(parser, TOK_SERIAL)) {
      lexer_next_token(parser->lexer);
      if (parser_match_id(parser, "at"))
        lexer_next_token(parser->lexer);
      Token baud_tok = parser->lexer->current_token;
      baud_tok.value = strdup(baud_tok.value);
      int baud = (int)atof(baud_tok.value);
      lexer_next_token(parser->lexer);
      if (parser_match(parser, TOK_BAUD))
        lexer_next_token(parser->lexer);
      return ast_serial_open(baud);
    }
    if (parser_match(parser, TOK_I2C)) {
      lexer_next_token(parser->lexer);
      return ast_i2c_open();
    }
    if (parser_match(parser, TOK_SPI)) {
      lexer_next_token(parser->lexer);
      if (parser_match_id(parser, "at"))
        lexer_next_token(parser->lexer);
      Token freq_tok = parser->lexer->current_token;
      freq_tok.value = strdup(freq_tok.value);
      int freq = (int)atof(freq_tok.value);
      lexer_next_token(parser->lexer);
      if (parser_match(parser, TOK_HZ))
        lexer_next_token(parser->lexer);
      return ast_spi_open(freq);
    }
    /* Wave 4: open file "name" */
    if (parser_match(parser, TOK_FILE)) {
      lexer_next_token(parser->lexer);
      ASTNode *filename = parse_expression(parser);
      return ast_file_open(filename);
    }
    error_report(parser->errors, ERROR_SYNTAX,
                 parser->lexer->current_token.line,
                 parser->lexer->current_token.column,
                 "Expected 'serial', 'i2c', 'spi', or 'file' after 'open'");
    return NULL;
  }

  /* send serial <expr> */
  if (parser_match(parser, TOK_SEND)) {
    lexer_next_token(parser->lexer);
    if (parser_match(parser, TOK_SERIAL)) {
      lexer_next_token(parser->lexer);
      ASTNode *val = parse_expression(parser);
      return ast_serial_send(val);
    }
  }

  /* radio_send_peer(peer_id, data) */
  if (parser_match(parser, TOK_RADIO_SEND)) {
    lexer_next_token(parser->lexer);
    parser_expect(parser, TOK_LPAREN);
    ASTNode *peer = parse_expression(parser);
    parser_expect(parser, TOK_COMMA);
    ASTNode *data = parse_expression(parser);
    parser_expect(parser, TOK_RPAREN);
    return ast_radio_send(peer, data);
  }

  /* write i2c device <addr> value <val>
     write i2c device <addr> register <reg>   (high-level write) */
  if (parser_match(parser, TOK_WRITE_KW)) {
    lexer_next_token(parser->lexer);
    if (parser_match(parser, TOK_I2C)) {
      lexer_next_token(parser->lexer);
      if (parser_match(parser, TOK_DEVICE))
        lexer_next_token(parser->lexer);
      ASTNode *addr = parse_expression(parser);
      if (parser_match_id(parser, "value"))
        lexer_next_token(parser->lexer);
      ASTNode *val = parse_expression(parser);
      return ast_i2c_device_write(addr, val);
    } else if (parser_match(parser, TOK_FILE)) {
      /* Wave 4: write file expr */
      lexer_next_token(parser->lexer);
      ASTNode *data = parse_expression(parser);
      return ast_file_write(data);
    } else if (parser_match(parser, TOK_DEVICE)) {
      lexer_next_token(parser->lexer);
      Token dev_name = parser->lexer->current_token;
      dev_name.value = strdup(dev_name.value);
      parser_expect(parser, TOK_ID);

      if (parser_match_id(parser, "value"))
        lexer_next_token(parser->lexer);
      ASTNode *val = parse_expression(parser);

      ProtocolType proto = PROTOCOL_UART;
      Symbol *sym = symbol_table_lookup(parser->symbols, dev_name.value);
      if (sym && sym->kind == SYMBOL_DEVICE)
        proto = sym->protocol;

      return ast_device_write(dev_name.value, proto, val);
    }
  }

  /* read i2c device <addr> register <reg> count <expr> into <array>
     In statements context, catch "read i2c device" array operations
     that look like expressions but execute statelessly into buffers */
  if (parser_match(parser, TOK_READ)) {
    // We will push back if not I2C array read, or we can just parse it as an
    // expression statement. Since `read` is usually an expression, if it sits
    // as a statement, we can just parse an expression.
    ASTNode *expr = parse_expression(parser);
    if (expr && expr->type == NODE_I2C_DEVICE_READ_ARRAY) {
      return expr;
    }
  }

  /* try { } on error { } */
  if (parser_match(parser, TOK_TRY)) {
    lexer_next_token(parser->lexer);
    parser_expect(parser, TOK_LBRACE);
    ASTNode *try_blk = parse_block(parser);
    parser_expect(parser, TOK_RBRACE);
    ASTNode *err_blk = NULL;
    /* expect 'on error' */
    if (parser_match(parser, TOK_ON)) {
      lexer_next_token(parser->lexer);
      /* consume 'error' identifier */
      if (parser_match(parser, TOK_ID) &&
          strcmp(parser->lexer->current_token.value, "error") == 0) {
        lexer_next_token(parser->lexer);
      }
      parser_expect(parser, TOK_LBRACE);
      err_blk = parse_block(parser);
      parser_expect(parser, TOK_RBRACE);
    }
    return ast_try(try_blk, err_blk);
  }

  /* enable watchdog timeout <N>ms OR enable ota "name" [password "pw"] OR
   * enable interrupts */
  if (parser_match(parser, TOK_ENABLE)) {
    lexer_next_token(parser->lexer);
    if (parser_match(parser, TOK_WATCHDOG)) {
      lexer_next_token(parser->lexer);
      if (parser_match(parser, TOK_TIMEOUT))
        lexer_next_token(parser->lexer);
      Token ms_tok = parser->lexer->current_token;
      ms_tok.value = strdup(ms_tok.value);
      int ms = (int)atof(ms_tok.value);
      lexer_next_token(parser->lexer);
      if (parser_match(parser, TOK_MS_TOK))
        lexer_next_token(parser->lexer);
      return ast_watchdog_enable(ms);
    } else if (parser_match_id(parser, "ota")) {
      /* enable ota "hostname" [password "secret"] */
      lexer_next_token(parser->lexer);
      char *hostname = strdup("kinetrix");
      char *password = NULL;
      if (parser_match(parser, TOK_STRING_LIT)) {
        free(hostname);
        hostname = strdup(parser->lexer->current_token.value);
        lexer_next_token(parser->lexer);
      }
      if (parser_match_id(parser, "password")) {
        lexer_next_token(parser->lexer);
        if (parser_match(parser, TOK_STRING_LIT)) {
          password = strdup(parser->lexer->current_token.value);
          lexer_next_token(parser->lexer);
        }
      }
      ASTNode *node = ast_ota_enable(hostname, password);
      free(hostname);
      if (password)
        free(password);
      return node;
    } else if (parser_match(parser, TOK_INTERRUPTS)) {
      lexer_next_token(parser->lexer);
      return ast_enable_interrupts();
    } else if (parser_match(parser, TOK_BLE)) {
      lexer_next_token(parser->lexer);
      Token name_tok = parser->lexer->current_token;
      name_tok.value = strdup(name_tok.value);
      parser_expect(parser, TOK_STRING_LIT);
      ASTNode *name = ast_string(name_tok.value);
      free(name_tok.value);
      return ast_ble_enable(name);
    }
  }

  if (parser_match(parser, TOK_DISABLE)) {
    lexer_next_token(parser->lexer);
    if (parser_match(parser, TOK_INTERRUPTS)) {
      lexer_next_token(parser->lexer);
      return ast_disable_interrupts();
    }
  }

  /* ============================================================
   * Wave 3: Communication Statement Handlers
   * ============================================================ */

  /* connect wifi "ssid" password "pass" | connect mqtt "url" port N | connect
   * websocket "url" */
  if (parser_match(parser, TOK_CONNECT)) {
    lexer_next_token(parser->lexer);
    if (parser_match(parser, TOK_WIFI)) {
      lexer_next_token(parser->lexer);
      ASTNode *ssid = parse_expression(parser);
      parser_expect_id(parser, "password");
      ASTNode *password = parse_expression(parser);
      return ast_wifi_connect(ssid, password);
    } else if (parser_match(parser, TOK_MQTT)) {
      lexer_next_token(parser->lexer);
      ASTNode *broker = parse_expression(parser);
      parser_expect_id(parser, "port");
      ASTNode *port = parse_expression(parser);
      return ast_mqtt_connect(broker, port);
    } else if (parser_match(parser, TOK_WEBSOCKET)) {
      lexer_next_token(parser->lexer);
      ASTNode *url = parse_expression(parser);
      return ast_ws_connect(url);
    }
  }

  /* ble advertise "data" | ble send expr */
  if (parser_match(parser, TOK_BLE)) {
    lexer_next_token(parser->lexer);
    if (parser_match_id(parser, "advertise")) {
      lexer_next_token(parser->lexer);
      ASTNode *data = parse_expression(parser);
      return ast_ble_advertise(data);
    } else if (parser_match(parser, TOK_SEND)) {
      lexer_next_token(parser->lexer);
      ASTNode *data = parse_expression(parser);
      return ast_ble_send(data);
    }
  }

  /* mqtt subscribe "topic" | mqtt publish "topic" expr */
  if (parser_match(parser, TOK_MQTT)) {
    lexer_next_token(parser->lexer);
    if (parser_match(parser, TOK_SUBSCRIBE)) {
      lexer_next_token(parser->lexer);
      ASTNode *topic = parse_expression(parser);
      return ast_mqtt_subscribe(topic);
    } else if (parser_match(parser, TOK_PUBLISH)) {
      lexer_next_token(parser->lexer);
      ASTNode *topic = parse_expression(parser);
      ASTNode *payload = parse_expression(parser);
      return ast_mqtt_publish(topic, payload);
    }
  }

  /* http post "url" body expr */
  if (parser_match(parser, TOK_HTTP)) {
    lexer_next_token(parser->lexer);
    if (parser_match_id(parser, "post")) {
      lexer_next_token(parser->lexer);
      ASTNode *url = parse_expression(parser);
      parser_expect_id(parser, "body");
      ASTNode *body = parse_expression(parser);
      return ast_http_post(url, body);
    }
  }

  /* ws send expr | ws close */
  if (parser_match(parser, TOK_WEBSOCKET)) {
    lexer_next_token(parser->lexer);
    if (parser_match(parser, TOK_SEND)) {
      lexer_next_token(parser->lexer);
      ASTNode *data = parse_expression(parser);
      return ast_ws_send(data);
    } else if (parser_match_id(parser, "close")) {
      lexer_next_token(parser->lexer);
      return ast_ws_close();
    }
  }

  /* feed watchdog */
  if (parser_match(parser, TOK_FEED)) {
    lexer_next_token(parser->lexer);
    if (parser_match(parser, TOK_WATCHDOG))
      lexer_next_token(parser->lexer);
    return ast_watchdog_feed();
  }

  /* assert <expr> else <call> */
  if (parser_match(parser, TOK_ASSERT)) {
    lexer_next_token(parser->lexer);
    ASTNode *cond = parse_expression(parser);
    ASTNode *action = NULL;
    if (parser_match(parser, TOK_ELSE)) {
      lexer_next_token(parser->lexer);
      action = parse_expression(parser);
    }
    return ast_assert(cond, action);
  }

  /* define type Name { T field ... }
     define device name as uart/i2c/spi at addr  */
  if (parser_match(parser, TOK_DEFINE)) {
    lexer_next_token(parser->lexer);
    if (parser_match(parser, TOK_TYPE_KW)) {
      lexer_next_token(parser->lexer);
      Token name_tok = parser->lexer->current_token;
      name_tok.value = strdup(name_tok.value);
      parser_expect(parser, TOK_ID);
      parser_expect(parser, TOK_LBRACE);
      /* Parse fields: each field is "<type> <fieldname>" */
      StructField *fields = malloc(sizeof(StructField) * 32);
      int nfields = 0;
      while (!parser_match(parser, TOK_RBRACE) &&
             !parser_match(parser, TOK_EOF)) {
        Type *ftype = type_float(); /* default */
        if (parser_match(parser, TOK_INT_KW)) {
          ftype = type_int();
          lexer_next_token(parser->lexer);
        } else if (parser_match(parser, TOK_FLOAT_KW)) {
          ftype = type_float();
          lexer_next_token(parser->lexer);
        } else if (parser_match(parser, TOK_BOOL_KW)) {
          ftype = type_bool();
          lexer_next_token(parser->lexer);
        } else if (parser_match(parser, TOK_BYTE_KW)) {
          ftype = type_byte();
          lexer_next_token(parser->lexer);
        } else if (parser_match(parser, TOK_VAR)) {
          lexer_next_token(parser->lexer);
        }

        Token fname_tok = parser->lexer->current_token;
        fname_tok.value = strdup(fname_tok.value);
        lexer_next_token(parser->lexer); // Advance token unconditionally

        if (!fname_tok.value || fname_tok.value[0] == '\0') {
          error_report(
              parser->errors, ERROR_SYNTAX, fname_tok.line, fname_tok.column,
              "Expected valid field name (ID or keyword) in struct definition");
          continue;
        }

        if (nfields < 32) {
          fields[nfields].name = strdup(fname_tok.value);
          fields[nfields].type = ftype;
          nfields++;
        }
      }
      parser_expect(parser, TOK_RBRACE);
      return ast_struct_def(name_tok.value, fields, nfields);
    }
    if (parser_match(parser, TOK_DEVICE)) {
      lexer_next_token(parser->lexer);
      Token dev_name = parser->lexer->current_token;
      dev_name.value = strdup(dev_name.value);
      parser_expect(parser, TOK_ID);
      if (parser_match_id(parser, "as"))
        lexer_next_token(parser->lexer);
      ProtocolType proto = PROTOCOL_UART;
      if (parser_match(parser, TOK_SERIAL)) {
        proto = PROTOCOL_UART;
        lexer_next_token(parser->lexer);
      } else if (parser_match(parser, TOK_I2C)) {
        proto = PROTOCOL_I2C;
        lexer_next_token(parser->lexer);
      } else if (parser_match(parser, TOK_SPI)) {
        proto = PROTOCOL_SPI;
        lexer_next_token(parser->lexer);
      } else if (parser_match(parser, TOK_ID) &&
                 strcmp(parser->lexer->current_token.value, "uart") == 0) {
        proto = PROTOCOL_UART;
        lexer_next_token(parser->lexer);
      }
      if (parser_match_id(parser, "at"))
        lexer_next_token(parser->lexer);
      ASTNode *addr = parse_expression(parser);
      symbol_table_add_device(parser->symbols, dev_name.value, proto);
      return ast_device_def(dev_name.value, proto, addr);
    }
    return NULL;
  }

  /* task <name> { body } */
  if (parser_match(parser, TOK_TASK)) {
    lexer_next_token(parser->lexer);
    Token tname = parser->lexer->current_token;
    tname.value = strdup(tname.value);
    parser_expect(parser, TOK_ID);
    parser_expect(parser, TOK_LBRACE);
    ASTNode *body = parse_block(parser);
    parser_expect(parser, TOK_RBRACE);
    return ast_task_def(tname.value, body);
  }

  /* start task <name> */
  if (parser_match(parser, TOK_START)) {
    lexer_next_token(parser->lexer);
    if (parser_match(parser, TOK_TASK))
      lexer_next_token(parser->lexer);
    Token tname = parser->lexer->current_token;
    tname.value = strdup(tname.value);
    parser_expect(parser, TOK_ID);
    return ast_task_start(tname.value);
  }

  /* shared make <type|var> <name> = <expr> */
  if (parser_match(parser, TOK_SHARED)) {
    lexer_next_token(parser->lexer);
    if (parser_match(parser, TOK_MAKE))
      lexer_next_token(parser->lexer);
    Type *t = type_float();
    if (parser_match(parser, TOK_INT_KW)) {
      t = type_int();
      lexer_next_token(parser->lexer);
    } else if (parser_match(parser, TOK_FLOAT_KW)) {
      t = type_float();
      lexer_next_token(parser->lexer);
    } else if (parser_match(parser, TOK_BOOL_KW)) {
      t = type_bool();
      lexer_next_token(parser->lexer);
    } else if (parser_match(parser, TOK_BYTE_KW)) {
      t = type_byte();
      lexer_next_token(parser->lexer);
    } else if (parser_match(parser, TOK_VAR)) {
      lexer_next_token(parser->lexer);
    } else if (parser_match_id(parser, "string")) {
      t = type_string();
      lexer_next_token(parser->lexer);
    }
    Token vname = parser->lexer->current_token;
    vname.value = strdup(vname.value);
    parser_expect(parser, TOK_ID);
    ASTNode *init = NULL;
    if (parser_match(parser, TOK_ASSIGN)) {
      lexer_next_token(parser->lexer);
      init = parse_expression(parser);
    }
    return ast_var_decl_ex(vname.value, t, init, 1 /* is_shared */);
  }

  return NULL;
}

// Parse block
static ASTNode *parse_block(Parser *parser) {
  int capacity = 100;
  ASTNode **statements = malloc(sizeof(ASTNode *) * capacity);
  int count = 0;
  int is_unreachable = 0;

  while (!parser_match(parser, TOK_RBRACE) && !parser_match(parser, TOK_EOF)) {
    Token start_tok = parser->lexer->current_token;
    start_tok.value = strdup(start_tok.value);
    ASTNode *stmt = parse_statement(parser);
    if (stmt) {
      if (is_unreachable) {
        // Hardcode warning emission without blocking compilation
        fprintf(stderr, "Warning: Unreachable code detected at line %d\n",
                start_tok.line);
        is_unreachable = 0; // Only warn once per block to prevent spam
      }

      if (stmt->type == NODE_RETURN || stmt->type == NODE_BREAK ||
          stmt->type == NODE_CONTINUE) {
        is_unreachable = 1;
      }

      if (count >= capacity) {
        capacity *= 2;
        statements = realloc(statements, sizeof(ASTNode *) * capacity);
      }
      statements[count++] = stmt;
    } else {
      // CRITICAL FIX: If statement is NULL, we must advance to avoid infinite
      // loop This happens when we encounter an unexpected token
      error_report(parser->errors, ERROR_SYNTAX,
                   parser->lexer->current_token.line,
                   parser->lexer->current_token.column,
                   "Unexpected token '%s', skipping",
                   parser->lexer->current_token.value
                       ? parser->lexer->current_token.value
                       : "NULL");
      lexer_next_token(parser->lexer); // Advance to prevent infinite loop
    }
  }

  return ast_block(statements, count);
}

// Main parse function
ASTNode *parser_parse(Parser *parser) {
  int capacity = 1024;
  ASTNode **statements = malloc(sizeof(ASTNode *) * capacity);
  int count = 0;

  // Parse global top-level declarations (functions, variables, definitions,
  // shared tasks)
  while (!parser_match(parser, TOK_PROGRAM) && !parser_match(parser, TOK_EOF)) {
    if (parser_match(parser, TOK_INCLUDE)) {
      lexer_next_token(parser->lexer);
      if (parser_match(parser, TOK_STRING_LIT)) {
        char *rel_path = strdup(parser->lexer->current_token.value);
        lexer_next_token(parser->lexer);

        char base_dir[1024];
        if (parser->lexer->file_path) {
          char *path_copy = strdup(parser->lexer->file_path);
          strcpy(base_dir, dirname(path_copy));
          free(path_copy);
        } else {
          strcpy(base_dir, ".");
        }

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", base_dir, rel_path);

        FILE *inc_file = fopen(full_path, "r");
        if (!inc_file) {
          const char *kpath = getenv("KINETRIX_PATH");
          if (kpath) {
            snprintf(full_path, sizeof(full_path), "%s/%s", kpath, rel_path);
            inc_file = fopen(full_path, "r");
          }
        }
        free(rel_path);

        if (!inc_file) {
          error_report(parser->errors, ERROR_INTERNAL,
                       parser->lexer->current_token.line,
                       parser->lexer->current_token.column,
                       "Could not open included file: %s", full_path);
        } else {
          Parser *inc_parser =
              parser_create(inc_file, full_path, parser->errors);
          ASTNode *inc_ast = parser_parse(inc_parser);
          if (inc_ast && inc_ast->type == NODE_PROGRAM &&
              inc_ast->data.program.main_block) {
            ASTNode *inc_block = inc_ast->data.program.main_block;
            for (int i = 0; i < inc_block->data.block.statement_count; i++) {
              if (count >= capacity) {
                capacity *= 2;
                statements = realloc(statements, sizeof(ASTNode *) * capacity);
              }
              statements[count++] = inc_block->data.block.statements[i];
            }
          }
          fclose(inc_file);
          // Do not free inc_parser entirely as strings exist in the unified AST
        }
      } else {
        error_report(parser->errors, ERROR_SYNTAX,
                     parser->lexer->current_token.line,
                     parser->lexer->current_token.column,
                     "Expected string literal after 'include'");
        lexer_next_token(parser->lexer);
      }
      continue;
    }

    ASTNode *stmt = parse_statement(parser);
    if (stmt) {
      if (count >= capacity) {
        capacity *= 2;
        statements = realloc(statements, sizeof(ASTNode *) * capacity);
      }
      statements[count++] = stmt;
    } else {
      error_report(parser->errors, ERROR_SYNTAX,
                   parser->lexer->current_token.line,
                   parser->lexer->current_token.column,
                   "Unexpected token at global scope, skipping");
      lexer_next_token(parser->lexer);
    }
  }

  // Skip optional "program" keyword and optional name
  if (parser_match(parser, TOK_PROGRAM)) {
    lexer_next_token(parser->lexer);
    /* Also skip the optional program name identifier */
    if (parser_match(parser, TOK_ID)) {
      lexer_next_token(parser->lexer);
    }
  }

  // Parse main program block statements
  if (parser_match(parser, TOK_LBRACE)) {
    lexer_next_token(parser->lexer);
    while (!parser_match(parser, TOK_RBRACE) &&
           !parser_match(parser, TOK_EOF)) {
      ASTNode *stmt = parse_statement(parser);
      if (stmt) {
        statements[count++] = stmt;
      } else {
        error_report(
            parser->errors, ERROR_SYNTAX, parser->lexer->current_token.line,
            parser->lexer->current_token.column, "Unexpected token, skipping");
        lexer_next_token(parser->lexer);
      }
    }
    parser_expect(parser, TOK_RBRACE);
  }

  // Create combined main block and program node
  ASTNode *main_block = ast_block(statements, count);
  return ast_program(NULL, 0, main_block);
}

static ASTNode *clone_target(ASTNode *node) {
  if (!node)
    return NULL;
  if (node->type == NODE_IDENTIFIER) {
    return ast_identifier(node->data.identifier.name);
  } else if (node->type == NODE_ARRAY_ACCESS) {
    return ast_array_access(clone_target(node->data.array_access.array),
                            clone_target(node->data.array_access.index));
  } else if (node->type == NODE_STRUCT_ACCESS) {
    return ast_struct_access(clone_target(node->data.struct_access.object),
                             node->data.struct_access.member);
  } else if (node->type == NODE_NUMBER) {
    return ast_number(node->data.number.value);
  } else if (node->type == NODE_BINARY_OP) {
    return ast_binary_op(node->data.binary_op.op,
                         clone_target(node->data.binary_op.left),
                         clone_target(node->data.binary_op.right));
  } else if (node->type == NODE_UNARY_OP) {
    return ast_unary_op(node->data.unary_op.op,
                        clone_target(node->data.unary_op.operand));
  } else if (node->type == NODE_CALL) {
    ASTNode **args = NULL;
    if (node->data.call.arg_count > 0) {
      args = malloc(sizeof(ASTNode *) * node->data.call.arg_count);
      for (int i = 0; i < node->data.call.arg_count; ++i) {
        args[i] = clone_target(node->data.call.args[i]);
      }
    }
    return ast_call(node->data.call.name, args, node->data.call.arg_count);
  }
  return NULL;
}
