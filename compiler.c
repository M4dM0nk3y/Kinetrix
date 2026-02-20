/* Kinetrix Compiler V2.8 - The Drone Update (I2C) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKEN_LEN 256

typedef enum {
    TOK_EOF, TOK_ID, TOK_NUMBER, TOK_STRING_LIT,
    TOK_PROGRAM, TOK_LBRACE, TOK_RBRACE, TOK_SEMI, TOK_LPAREN, TOK_RPAREN, TOK_COMMA,
    TOK_TURN, TOK_ON, TOK_OFF, TOK_WAIT, TOK_SET, TOK_TO, TOK_MAKE, TOK_CHANGE, TOK_BY,
    TOK_FOREVER, TOK_IF, TOK_ELSE, TOK_READ, TOK_ANALOG, TOK_PULSE, TOK_SERVO, TOK_PRINT, TOK_DEF, TOK_REPEAT,
    TOK_TONE, TOK_NOTONE, TOK_FREQ, TOK_RETURN,
    TOK_WHILE, TOK_BREAK,
    TOK_ARRAY, TOK_SIZE, TOK_INDEX, TOK_OF, TOK_LBRACKET, TOK_RBRACKET,
    TOK_SERIAL,
    TOK_I2C, TOK_BEGIN, TOK_START, TOK_SEND, TOK_STOP, // NEW I2C TOKENS
    TOK_PIN, TOK_IS, TOK_HIGH, TOK_LOW, TOK_VAR,
    TOK_SIN, TOK_COS, TOK_TAN, TOK_SQRT,
    TOK_ACOS, TOK_ASIN, TOK_ATAN, TOK_ATAN2,
    TOK_ASSIGN, TOK_EQ, TOK_LT, TOK_GT, TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_AND, TOK_OR
} TokenType;

typedef struct { TokenType type; char value[MAX_TOKEN_LEN]; } Token;

FILE *src_file, *out_file;
Token current_token;
int next_char = ' '; 
int loop_depth = 0; 

int is_keyword(TokenType t) {
    return (t == TOK_TURN || t == TOK_WAIT || t == TOK_SET || t == TOK_CHANGE || 
            t == TOK_MAKE || t == TOK_IF || t == TOK_FOREVER || t == TOK_RBRACE || 
            t == TOK_SERVO || t == TOK_PRINT || t == TOK_DEF || t == TOK_REPEAT ||
            t == TOK_TONE || t == TOK_NOTONE || t == TOK_RETURN || t == TOK_WHILE || t == TOK_BREAK ||
            t == TOK_ARRAY || t == TOK_I2C);
}

void next_token() {
    while (isspace(next_char)) next_char = fgetc(src_file);
    if (next_char == EOF) { current_token.type = TOK_EOF; return; }

    if (next_char == '"') {
        int i = 0; next_char = fgetc(src_file); 
        while (next_char != '"' && next_char != EOF && i < 255) {
            current_token.value[i++] = next_char; next_char = fgetc(src_file);
        }
        current_token.value[i] = '\0';
        if (next_char == '"') next_char = fgetc(src_file); 
        current_token.type = TOK_STRING_LIT;
        return;
    }

    if (next_char == '(') { current_token.type = TOK_LPAREN; strcpy(current_token.value, "("); next_char = fgetc(src_file); return; }
    if (next_char == ')') { current_token.type = TOK_RPAREN; strcpy(current_token.value, ")"); next_char = fgetc(src_file); return; }
    if (next_char == '[') { current_token.type = TOK_LBRACKET; strcpy(current_token.value, "["); next_char = fgetc(src_file); return; }
    if (next_char == ']') { current_token.type = TOK_RBRACKET; strcpy(current_token.value, "]"); next_char = fgetc(src_file); return; }
    if (next_char == ',') { current_token.type = TOK_COMMA; strcpy(current_token.value, ","); next_char = fgetc(src_file); return; }

    if (isalpha(next_char) || next_char == '_') {
        int i = 0;
        while ((isalnum(next_char) || next_char == '_') && i < 255) {
            current_token.value[i++] = next_char; next_char = fgetc(src_file);
        }
        current_token.value[i] = '\0';
        
        if (!strcmp(current_token.value, "program")) current_token.type = TOK_PROGRAM;
        else if (!strcmp(current_token.value, "def")) current_token.type = TOK_DEF;
        else if (!strcmp(current_token.value, "repeat")) current_token.type = TOK_REPEAT;
        else if (!strcmp(current_token.value, "turn")) current_token.type = TOK_TURN;
        else if (!strcmp(current_token.value, "on")) current_token.type = TOK_ON;
        else if (!strcmp(current_token.value, "off")) current_token.type = TOK_OFF;
        else if (!strcmp(current_token.value, "wait")) current_token.type = TOK_WAIT;
        else if (!strcmp(current_token.value, "forever")) current_token.type = TOK_FOREVER;
        else if (!strcmp(current_token.value, "if")) current_token.type = TOK_IF;
        else if (!strcmp(current_token.value, "else")) current_token.type = TOK_ELSE;
        else if (!strcmp(current_token.value, "make")) current_token.type = TOK_MAKE;
        else if (!strcmp(current_token.value, "var")) current_token.type = TOK_VAR;
        else if (!strcmp(current_token.value, "set")) current_token.type = TOK_SET;
        else if (!strcmp(current_token.value, "change")) current_token.type = TOK_CHANGE;
        else if (!strcmp(current_token.value, "to")) current_token.type = TOK_TO;
        else if (!strcmp(current_token.value, "by")) current_token.type = TOK_BY;
        else if (!strcmp(current_token.value, "read")) current_token.type = TOK_READ;
        else if (!strcmp(current_token.value, "analog")) current_token.type = TOK_ANALOG;
        else if (!strcmp(current_token.value, "pulse")) current_token.type = TOK_PULSE;
        else if (!strcmp(current_token.value, "serial")) current_token.type = TOK_SERIAL;
        else if (!strcmp(current_token.value, "servo")) current_token.type = TOK_SERVO;
        else if (!strcmp(current_token.value, "print")) current_token.type = TOK_PRINT;
        else if (!strcmp(current_token.value, "tone")) current_token.type = TOK_TONE;
        else if (!strcmp(current_token.value, "notone")) current_token.type = TOK_NOTONE;
        else if (!strcmp(current_token.value, "freq")) current_token.type = TOK_FREQ;
        else if (!strcmp(current_token.value, "return")) current_token.type = TOK_RETURN;
        else if (!strcmp(current_token.value, "while")) current_token.type = TOK_WHILE;
        else if (!strcmp(current_token.value, "break")) current_token.type = TOK_BREAK;
        else if (!strcmp(current_token.value, "array")) current_token.type = TOK_ARRAY;
        else if (!strcmp(current_token.value, "size")) current_token.type = TOK_SIZE;
        else if (!strcmp(current_token.value, "index")) current_token.type = TOK_INDEX;
        else if (!strcmp(current_token.value, "of")) current_token.type = TOK_OF;
        
        // NEW I2C TOKENS
        else if (!strcmp(current_token.value, "i2c")) current_token.type = TOK_I2C;
        else if (!strcmp(current_token.value, "begin")) current_token.type = TOK_BEGIN;
        else if (!strcmp(current_token.value, "start")) current_token.type = TOK_START;
        else if (!strcmp(current_token.value, "send")) current_token.type = TOK_SEND;
        else if (!strcmp(current_token.value, "stop")) current_token.type = TOK_STOP;
        
        else if (!strcmp(current_token.value, "pin")) current_token.type = TOK_PIN;
        else if (!strcmp(current_token.value, "is")) current_token.type = TOK_IS;
        else if (!strcmp(current_token.value, "high")) current_token.type = TOK_HIGH;
        else if (!strcmp(current_token.value, "low")) current_token.type = TOK_LOW;
        else if (!strcmp(current_token.value, "and")) current_token.type = TOK_AND;
        else if (!strcmp(current_token.value, "or")) current_token.type = TOK_OR;
        else if (!strcmp(current_token.value, "sin")) current_token.type = TOK_SIN;
        else if (!strcmp(current_token.value, "cos")) current_token.type = TOK_COS;
        else if (!strcmp(current_token.value, "tan")) current_token.type = TOK_TAN;
        else if (!strcmp(current_token.value, "sqrt")) current_token.type = TOK_SQRT;
        else if (!strcmp(current_token.value, "acos")) current_token.type = TOK_ACOS;
        else if (!strcmp(current_token.value, "asin")) current_token.type = TOK_ASIN;
        else if (!strcmp(current_token.value, "atan")) current_token.type = TOK_ATAN;
        else if (!strcmp(current_token.value, "atan2")) current_token.type = TOK_ATAN2;
        
        else current_token.type = TOK_ID;
        return;
    }
    
    // Allow Hex numbers (0x68) for I2C addresses
    if (isdigit(next_char)) {
        int i = 0;
        int has_dot = 0;
        int has_x = 0;
        while (isdigit(next_char) || (next_char == '.' && !has_dot) || (next_char == 'x' && !has_x) || (has_x && isxdigit(next_char))) {
            if (next_char == '.') has_dot = 1;
            if (next_char == 'x') has_x = 1;
            current_token.value[i++] = next_char; 
            next_char = fgetc(src_file);
        }
        current_token.value[i] = '\0';
        current_token.type = TOK_NUMBER;
        return;
    }
    
    if (next_char == '#') { while (next_char != '\n' && next_char != EOF) next_char = fgetc(src_file); next_token(); return; }
    
    if (next_char == '{') current_token.type = TOK_LBRACE;
    else if (next_char == '}') current_token.type = TOK_RBRACE;
    else if (next_char == '=') current_token.type = TOK_ASSIGN;
    else if (next_char == '<') current_token.type = TOK_LT;
    else if (next_char == '>') current_token.type = TOK_GT;
    else if (next_char == '+') current_token.type = TOK_PLUS;
    else if (next_char == '-') current_token.type = TOK_MINUS;
    else if (next_char == '*') current_token.type = TOK_STAR; 
    else if (next_char == '/') current_token.type = TOK_SLASH; 
    else { current_token.type = TOK_ID; } 
    strcpy(current_token.value, (char[]){(char)next_char, '\0'});
    next_char = fgetc(src_file);
}

void parse_block(); 

int is_operator(TokenType t) {
    return (t == TOK_PLUS || t == TOK_MINUS || t == TOK_STAR || t == TOK_SLASH || 
            t == TOK_IS || t == TOK_LT || t == TOK_GT || t == TOK_AND || t == TOK_OR);
}

// HELPER: Eat opening parenthesis if present
void eat_lparen() {
    if (current_token.type == TOK_LPAREN) next_token();
}
// HELPER: Eat closing parenthesis if present
void eat_rparen() {
    if (current_token.type == TOK_RPAREN) next_token();
}

void parse_expression() {
    int expect_operator = 0; 

    while (current_token.type != TOK_LBRACE && current_token.type != TOK_TO && current_token.type != TOK_BY && current_token.type != TOK_FREQ && current_token.type != TOK_RPAREN && current_token.type != TOK_SEMI && current_token.type != TOK_OF && current_token.type != TOK_COMMA && !is_keyword(current_token.type)) {
        
        if (expect_operator && (current_token.type == TOK_ID || current_token.type == TOK_NUMBER || current_token.type == TOK_READ || current_token.type == TOK_SIN || current_token.type == TOK_COS)) {
            break; 
        }

        if (current_token.type == TOK_READ) {
            next_token();
            if (current_token.type == TOK_ANALOG) {
                next_token(); 
                if (current_token.type == TOK_PIN) next_token(); 
                fprintf(out_file, "analogRead(A"); 
                fprintf(out_file, "%s)", current_token.value);
                next_token(); 
            }
            else if (current_token.type == TOK_PULSE) {
                next_token();
                if (current_token.type == TOK_PIN) next_token();
                fprintf(out_file, "pulseIn(");
                fprintf(out_file, "%s, 1)", current_token.value);
                next_token();
            }
            else if (current_token.type == TOK_SERIAL) {
                next_token();
                fprintf(out_file, "Serial.parseFloat()");
            }
            // NEW: Read I2C
            // Usage: read i2c <addr>
            // Translation: (Wire.requestFrom(addr, 1), Wire.read()) -- Comma operator allows this!
            else if (current_token.type == TOK_I2C) {
                next_token();
                fprintf(out_file, "(Wire.requestFrom(");
                parse_expression(); // Address
                fprintf(out_file, ", 1), Wire.read())");
            }
            expect_operator = 1; 
        } 
        // FIXED MATH FUNCTIONS: Eat LPAREN and RPAREN
        else if (current_token.type == TOK_SIN) { next_token(); eat_lparen(); fprintf(out_file, "sin("); parse_expression(); fprintf(out_file, ")"); eat_rparen(); expect_operator=1; }
        else if (current_token.type == TOK_COS) { next_token(); eat_lparen(); fprintf(out_file, "cos("); parse_expression(); fprintf(out_file, ")"); eat_rparen(); expect_operator=1; }
        else if (current_token.type == TOK_TAN) { next_token(); eat_lparen(); fprintf(out_file, "tan("); parse_expression(); fprintf(out_file, ")"); eat_rparen(); expect_operator=1; }
        else if (current_token.type == TOK_SQRT) { next_token(); eat_lparen(); fprintf(out_file, "sqrt("); parse_expression(); fprintf(out_file, ")"); eat_rparen(); expect_operator=1; }
        else if (current_token.type == TOK_ACOS) { next_token(); eat_lparen(); fprintf(out_file, "acos("); parse_expression(); fprintf(out_file, ")"); eat_rparen(); expect_operator=1; }
        else if (current_token.type == TOK_ASIN) { next_token(); eat_lparen(); fprintf(out_file, "asin("); parse_expression(); fprintf(out_file, ")"); eat_rparen(); expect_operator=1; }
        else if (current_token.type == TOK_ATAN) { next_token(); eat_lparen(); fprintf(out_file, "atan("); parse_expression(); fprintf(out_file, ")"); eat_rparen(); expect_operator=1; }
        else if (current_token.type == TOK_ATAN2) { 
            next_token(); 
            eat_lparen(); 
            fprintf(out_file, "atan2("); 
            parse_expression(); 
            fprintf(out_file, ", "); 
            if (current_token.type == TOK_COMMA) next_token(); 
            parse_expression(); 
            fprintf(out_file, ")"); 
            eat_rparen(); 
            expect_operator=1; 
        }
        else if (current_token.type == TOK_STRING_LIT) {
            fprintf(out_file, "\"%s\"", current_token.value);
            next_token();
            expect_operator = 1;
        }
        else if (is_operator(current_token.type)) {
            if (current_token.type == TOK_IS) fprintf(out_file, "==");
            else if (current_token.type == TOK_AND) fprintf(out_file, "&&");
            else if (current_token.type == TOK_OR) fprintf(out_file, "||");
            else fprintf(out_file, "%s", current_token.value);
            fprintf(out_file, " ");
            next_token();
            expect_operator = 0; 
        }
        else if (current_token.type == TOK_LPAREN) {
            fprintf(out_file, "(");
            next_token();
            expect_operator = 0; 
        }
        else {
            if (current_token.type == TOK_PIN) { next_token(); } 
            if (current_token.type == TOK_HIGH) fprintf(out_file, "1");
            else if (current_token.type == TOK_LOW) fprintf(out_file, "0");
            else fprintf(out_file, "%s", current_token.value);
            fprintf(out_file, " ");
            if (current_token.type != TOK_PIN) expect_operator = 1;
            next_token();
        }
    }
}

void parse_statement() {
    if (current_token.type == TOK_WAIT) {
        next_token(); fprintf(out_file, "delay("); parse_expression(); fprintf(out_file, ");\n");
    }
    else if (current_token.type == TOK_PRINT) { 
        next_token(); 
        fprintf(out_file, "Serial.println("); parse_expression(); fprintf(out_file, ");\n");
    }
    // NEW: I2C Commands
    else if (current_token.type == TOK_I2C) {
        next_token();
        if (current_token.type == TOK_BEGIN) {
            next_token();
            fprintf(out_file, "Wire.begin();\n");
        }
        else if (current_token.type == TOK_START) {
            next_token();
            fprintf(out_file, "Wire.beginTransmission("); parse_expression(); fprintf(out_file, ");\n");
        }
        else if (current_token.type == TOK_SEND) {
            next_token();
            fprintf(out_file, "Wire.write((byte)"); parse_expression(); fprintf(out_file, ");\n");
        }
        else if (current_token.type == TOK_STOP) {
            next_token();
            fprintf(out_file, "Wire.endTransmission();\n");
        }
    }
    else if (current_token.type == TOK_RETURN) {
        next_token(); fprintf(out_file, "return "); parse_expression(); fprintf(out_file, ";\n");
    }
    else if (current_token.type == TOK_WHILE) {
        next_token(); fprintf(out_file, "while ("); parse_expression(); fprintf(out_file, ") ");
        parse_block();
    }
    else if (current_token.type == TOK_BREAK) {
        next_token(); fprintf(out_file, "break;\n");
    }
    else if (current_token.type == TOK_TONE) {
        next_token();
        if (current_token.type == TOK_PIN) next_token();
        fprintf(out_file, "tone("); parse_expression();
        if (current_token.type == TOK_FREQ) next_token();
        fprintf(out_file, ", "); parse_expression(); fprintf(out_file, ");\n");
    }
    else if (current_token.type == TOK_NOTONE) {
        next_token();
        if (current_token.type == TOK_PIN) next_token();
        fprintf(out_file, "noTone("); parse_expression(); fprintf(out_file, ");\n");
    }
    else if (current_token.type == TOK_REPEAT) { 
        next_token(); 
        loop_depth++;
        fprintf(out_file, "for(int _i%d=0; _i%d < ", loop_depth, loop_depth); 
        parse_expression(); 
        fprintf(out_file, "; _i%d++) ", loop_depth);
        parse_block();
        loop_depth--;
    }
    else if (current_token.type == TOK_TURN) {
        next_token(); int state = (current_token.type == TOK_ON) ? 1 : 0;
        next_token(); if (current_token.type == TOK_PIN) next_token(); 
        fprintf(out_file, "digitalWrite("); parse_expression(); fprintf(out_file, ", %d);\n", state);
    }
    else if (current_token.type == TOK_SERVO) { 
        next_token(); 
        if (current_token.type == TOK_PIN) next_token();
        fprintf(out_file, "analogWrite("); parse_expression(); 
        if (current_token.type == TOK_SET) next_token(); 
        fprintf(out_file, ", ("); parse_expression(); fprintf(out_file, " * 255 / 180));\n");
    }
    else if (current_token.type == TOK_SET) { 
        next_token(); 
        if (current_token.type == TOK_INDEX) {
            next_token(); 
        }
        else if (current_token.type == TOK_ARRAY) {
             next_token();
             fprintf(out_file, "%s[(int)(", current_token.value); 
             next_token();
             if (current_token.type == TOK_ID && !strcmp(current_token.value, "at")) next_token(); 
             
             parse_expression(); 
             fprintf(out_file, ")] = "); 
             
             if (current_token.type == TOK_TO) next_token();
             parse_expression(); 
             fprintf(out_file, ";\n");
        }
        else {
            if (current_token.type == TOK_PIN) next_token();
            fprintf(out_file, "analogWrite("); parse_expression();
            if (current_token.type == TOK_TO) next_token();
            fprintf(out_file, ", "); parse_expression(); fprintf(out_file, ");\n");
        }
    }
    else if (current_token.type == TOK_CHANGE) { 
        next_token(); 
        fprintf(out_file, "%s", current_token.value); 
        next_token();
        if (current_token.type == TOK_BY) next_token();
        fprintf(out_file, " += "); 
        parse_expression(); fprintf(out_file, ";\n");
    }
    else if (current_token.type == TOK_MAKE) {
        next_token(); 
        if (current_token.type == TOK_ARRAY) {
            next_token();
            fprintf(out_file, "float %s", current_token.value); 
            next_token();
            if (current_token.type == TOK_SIZE) next_token();
            fprintf(out_file, "[");
            parse_expression(); 
            fprintf(out_file, "];\n");
        }
        else {
            if (current_token.type == TOK_VAR) next_token();
            fprintf(out_file, "float %s", current_token.value);
            next_token();
            if (current_token.type == TOK_ASSIGN) { fprintf(out_file, " = "); next_token(); parse_expression(); }
            fprintf(out_file, ";\n");
        }
    }
    else if (current_token.type == TOK_IF) {
        next_token(); 
        if (current_token.type == TOK_PIN) {
            next_token(); fprintf(out_file, "if (digitalRead("); parse_expression(); fprintf(out_file, ") == 1) ");
        } else {
            fprintf(out_file, "if ("); parse_expression(); fprintf(out_file, ") ");
        }
        parse_block();
    }
    else if (current_token.type == TOK_FOREVER) {
        next_token(); fprintf(out_file, "while(1) "); parse_block();
    }
    else if (current_token.type == TOK_ID) {
        char func_name[MAX_TOKEN_LEN];
        strcpy(func_name, current_token.value);
        next_token();
        if (current_token.type == TOK_LPAREN) { 
             next_token();
             fprintf(out_file, "%s(", func_name);
             parse_expression(); 
             fprintf(out_file, ");\n");
             if (current_token.type == TOK_RPAREN) next_token(); 
        } else {
             fprintf(out_file, "%s();\n", func_name); 
        }
    }
    else if (current_token.type == TOK_LBRACE) parse_block();
    else next_token(); 
}

void parse_block() {
    if (current_token.type == TOK_LBRACE) next_token();
    fprintf(out_file, "{\n");
    while (current_token.type != TOK_RBRACE && current_token.type != TOK_EOF) parse_statement();
    if (current_token.type == TOK_RBRACE) next_token();
    fprintf(out_file, "}\n");
}

int main(int argc, char **argv) {
    out_file = fopen("Kinetrix_Arduino.ino", "w");
    fprintf(out_file, "#include <Wire.h>\n"); // NEW: Always include Wire library!
    src_file = fopen(argv[1], "r");
    if (!src_file) return 1;
    next_char = fgetc(src_file); next_token(); 
    
    while (current_token.type != TOK_EOF) {
        if (current_token.type == TOK_DEF) {
            next_token(); 
            char func_name[MAX_TOKEN_LEN];
            strcpy(func_name, current_token.value);
            next_token(); 
            if (current_token.type == TOK_LPAREN) {
                 next_token();
                 fprintf(out_file, "float %s(float %s) ", func_name, current_token.value);
                 next_token(); 
                 if (current_token.type == TOK_RPAREN) next_token(); 
            } else {
                 fprintf(out_file, "float %s() ", func_name);
            }
            parse_block(); 
        }
        else if (current_token.type == TOK_PROGRAM) {
            next_token(); next_token(); 
            fprintf(out_file, "void setup() { Serial.begin(9600); Serial.setTimeout(100); for(int i=2; i<=13; i++) pinMode(i, OUTPUT); }\nvoid loop() ");
            parse_block();
        } 
        else next_token(); 
    }
    fclose(src_file); fclose(out_file);
    return 0;
}
