/* Kinetrix ESP32 Code Generator
 * Target: ESP32 / ESP8266
 * Output: Arduino-compatible C++ with ESP32 extensions
 * Compile with: Arduino IDE (ESP32 board package) or arduino-cli
 */

#include "codegen.h"
#include "ast.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

// Forward declarations
static void esp32_expression(CodeGen *gen, ASTNode *node);
static void esp32_statement(CodeGen *gen, ASTNode *node);

// ============================================================
// EXPRESSION GENERATION
// ============================================================

static void esp32_expression(CodeGen *gen, ASTNode *node) {
    if (node == NULL) return;

    switch (node->type) {
        case NODE_NUMBER:
            codegen_emit(gen, "%g", node->data.number.value);
            break;
        case NODE_STRING:
            codegen_emit(gen, "\"%s\"", node->data.string.value);
            break;
        case NODE_BOOL:
            codegen_emit(gen, "%d", node->data.boolean.value);
            break;
        case NODE_IDENTIFIER:
            codegen_emit(gen, "%s", node->data.identifier.name);
            break;

        case NODE_BINARY_OP: {
            const char *op_str = "?";
            switch (node->data.binary_op.op) {
                case OP_ADD: op_str = "+";  break;
                case OP_SUB: op_str = "-";  break;
                case OP_MUL: op_str = "*";  break;
                case OP_DIV: op_str = "/";  break;
                case OP_MOD: op_str = "%";  break;
                case OP_EQ:  op_str = "=="; break;
                case OP_NEQ: op_str = "!="; break;
                case OP_LT:  op_str = "<";  break;
                case OP_GT:  op_str = ">";  break;
                case OP_LTE: op_str = "<="; break;
                case OP_GTE: op_str = ">="; break;
                case OP_AND: op_str = "&&"; break;
                case OP_OR:  op_str = "||"; break;
                default: break;
            }
            if (node->data.binary_op.op == OP_MOD) {
                codegen_emit(gen, "((int)(");
                esp32_expression(gen, node->data.binary_op.left);
                codegen_emit(gen, ") %% (int)(");
                esp32_expression(gen, node->data.binary_op.right);
                codegen_emit(gen, "))");
            } else {
                codegen_emit(gen, "(");
                esp32_expression(gen, node->data.binary_op.left);
                codegen_emit(gen, " %s ", op_str);
                esp32_expression(gen, node->data.binary_op.right);
                codegen_emit(gen, ")");
            }
            break;
        }

        case NODE_UNARY_OP:
            if (node->data.unary_op.op == OP_NOT) {
                codegen_emit(gen, "!(");
                esp32_expression(gen, node->data.unary_op.operand);
                codegen_emit(gen, ")");
            } else {
                codegen_emit(gen, "-(");
                esp32_expression(gen, node->data.unary_op.operand);
                codegen_emit(gen, ")");
            }
            break;

        case NODE_CALL:
            codegen_emit(gen, "%s(", node->data.call.name);
            for (int i = 0; i < node->data.call.arg_count; i++) {
                if (i > 0) codegen_emit(gen, ", ");
                esp32_expression(gen, node->data.call.args[i]);
            }
            codegen_emit(gen, ")");
            break;

        case NODE_ANALOG_READ:
            // ESP32 has 12-bit ADC (0-4095) — expose raw value
            codegen_emit(gen, "analogRead(A");
            esp32_expression(gen, node->data.gpio.pin);
            codegen_emit(gen, ")");
            break;

        case NODE_GPIO_READ:
            codegen_emit(gen, "digitalRead(");
            esp32_expression(gen, node->data.gpio.pin);
            codegen_emit(gen, ")");
            break;

        case NODE_ARRAY_ACCESS:
            esp32_expression(gen, node->data.array_access.array);
            codegen_emit(gen, "[(int)(");
            esp32_expression(gen, node->data.array_access.index);
            codegen_emit(gen, ")]");
            break;

        default:
            codegen_emit(gen, "0 /* unsupported expr */");
            break;
    }
}

// ============================================================
// STATEMENT GENERATION
// ============================================================

static void esp32_statement(CodeGen *gen, ASTNode *node) {
    if (node == NULL) return;

    switch (node->type) {
        case NODE_VAR_DECL:
            codegen_emit_line(gen, "float %s", node->data.var_decl.name);
            if (node->data.var_decl.initializer) {
                codegen_emit(gen, " = ");
                esp32_expression(gen, node->data.var_decl.initializer);
            }
            codegen_emit(gen, ";\n");
            break;

        case NODE_ASSIGNMENT:
            codegen_emit_indent(gen);
            esp32_expression(gen, node->data.assignment.target);
            codegen_emit(gen, " = ");
            esp32_expression(gen, node->data.assignment.value);
            codegen_emit(gen, ";\n");
            break;

        case NODE_IF:
            codegen_emit_line(gen, "if (");
            esp32_expression(gen, node->data.if_stmt.condition);
            codegen_emit(gen, ") {\n");
            gen->indent_level++;
            esp32_statement(gen, node->data.if_stmt.then_block);
            gen->indent_level--;
            codegen_emit_line(gen, "}");
            if (node->data.if_stmt.else_block) {
                codegen_emit(gen, " else {\n");
                gen->indent_level++;
                esp32_statement(gen, node->data.if_stmt.else_block);
                gen->indent_level--;
                codegen_emit_line(gen, "}\n");
            } else {
                codegen_emit(gen, "\n");
            }
            break;

        case NODE_WHILE:
            codegen_emit_line(gen, "while (");
            esp32_expression(gen, node->data.while_loop.condition);
            codegen_emit(gen, ") {\n");
            gen->indent_level++;
            esp32_statement(gen, node->data.while_loop.body);
            gen->indent_level--;
            codegen_emit_line(gen, "}\n");
            break;

        case NODE_REPEAT: {
            int id = gen->loop_counter++;
            codegen_emit_line(gen, "for (int _i%d = 0; _i%d < (int)(", id, id);
            esp32_expression(gen, node->data.repeat_loop.count);
            codegen_emit(gen, "); _i%d++) {\n", id);
            gen->indent_level++;
            esp32_statement(gen, node->data.repeat_loop.body);
            gen->indent_level--;
            codegen_emit_line(gen, "}\n");
            break;
        }

        case NODE_FOREVER:
            codegen_emit_line(gen, "while (1) {\n");
            gen->indent_level++;
            esp32_statement(gen, node->data.forever_loop.body);
            gen->indent_level--;
            codegen_emit_line(gen, "}\n");
            break;

        case NODE_BLOCK:
            for (int i = 0; i < node->data.block.statement_count; i++)
                esp32_statement(gen, node->data.block.statements[i]);
            break;

        case NODE_RETURN:
            codegen_emit_line(gen, "return");
            if (node->data.return_stmt.value) {
                codegen_emit(gen, " ");
                esp32_expression(gen, node->data.return_stmt.value);
            }
            codegen_emit(gen, ";\n");
            break;

        case NODE_BREAK:
            codegen_emit_line(gen, "break;\n");
            break;

        case NODE_GPIO_WRITE:
            codegen_emit_indent(gen);
            codegen_emit(gen, "digitalWrite(");
            esp32_expression(gen, node->data.gpio.pin);
            codegen_emit(gen, ", ");
            esp32_expression(gen, node->data.gpio.value);
            codegen_emit(gen, ");\n");
            break;

        case NODE_ANALOG_WRITE:
            // ESP32 uses ledcWrite for PWM — emit as analogWrite for simplicity
            // (works with ESP32 Arduino core)
            codegen_emit_indent(gen);
            codegen_emit(gen, "analogWrite(");
            esp32_expression(gen, node->data.gpio.pin);
            codegen_emit(gen, ", ");
            esp32_expression(gen, node->data.gpio.value);
            codegen_emit(gen, ");\n");
            break;

        case NODE_WAIT:
            codegen_emit_indent(gen);
            codegen_emit(gen, "delay(");
            esp32_expression(gen, node->data.unary.child);
            codegen_emit(gen, ");\n");
            break;

        case NODE_PRINT:
            codegen_emit_indent(gen);
            codegen_emit(gen, "Serial.println(");
            esp32_expression(gen, node->data.unary.child);
            codegen_emit(gen, ");\n");
            break;

        case NODE_CALL:
            codegen_emit_indent(gen);
            esp32_expression(gen, node);
            codegen_emit(gen, ";\n");
            break;

        case NODE_TONE:
            codegen_emit_line(gen, "tone(");
            esp32_expression(gen, node->data.gpio.pin);
            codegen_emit(gen, ", ");
            esp32_expression(gen, node->data.gpio.value);
            codegen_emit(gen, ");\n");
            break;

        case NODE_NOTONE:
            codegen_emit_line(gen, "noTone(");
            esp32_expression(gen, node->data.gpio.pin);
            codegen_emit(gen, ");\n");
            break;

        case NODE_FUNCTION_DEF:
            if (node->data.function_def.is_extern) {
                codegen_emit_line(gen, "/* Extern %s function: %s */", 
                                  node->data.function_def.extern_lang, 
                                  node->data.function_def.name);
                break;
            }
            codegen_emit_line(gen, "void %s(", node->data.function_def.name);
            for (int i = 0; i < node->data.function_def.param_count; i++) {
                if (i > 0) codegen_emit(gen, ", ");
                codegen_emit(gen, "float %s", node->data.function_def.param_names[i]);
            }
            codegen_emit(gen, ") {\n");
            gen->indent_level++;
            esp32_statement(gen, node->data.function_def.body);
            gen->indent_level--;
            codegen_emit_line(gen, "}\n");
            break;

        default:
            break;
    }
}

// ============================================================
// PROGRAM ENTRY POINT
// ============================================================

void codegen_generate_esp32(CodeGen *gen, ASTNode *program) {
    if (!program || program->type != NODE_PROGRAM) return;

    // Header
    codegen_emit_line(gen, "// Generated by Kinetrix Compiler (Target: ESP32)");
    codegen_emit_line(gen, "// Board: ESP32 Dev Module");
    codegen_emit_line(gen, "// Upload via: Arduino IDE with ESP32 board package\n");
    codegen_emit_line(gen, "#include <Arduino.h>\n");

    // Hoist function definitions
    if (program->data.program.main_block &&
        program->data.program.main_block->type == NODE_BLOCK) {
        ASTNode *block = program->data.program.main_block;
        for (int i = 0; i < block->data.block.statement_count; i++) {
            ASTNode *s = block->data.block.statements[i];
            if (s && s->type == NODE_FUNCTION_DEF) {
                if (s->data.function_def.is_extern) {
                    codegen_emit_line(gen, "extern void %s(", s->data.function_def.name);
                    for (int j = 0; j < s->data.function_def.param_count; j++) {
                        if (j > 0) codegen_emit(gen, ", ");
                        codegen_emit(gen, "float %s", s->data.function_def.param_names[j]);
                    }
                    codegen_emit(gen, ");\n\n");
                    continue;
                }
                codegen_emit_line(gen, "void %s(", s->data.function_def.name);
                for (int j = 0; j < s->data.function_def.param_count; j++) {
                    if (j > 0) codegen_emit(gen, ", ");
                    codegen_emit(gen, "float %s", s->data.function_def.param_names[j]);
                }
                codegen_emit(gen, ") {\n");
                gen->indent_level++;
                esp32_statement(gen, s->data.function_def.body);
                gen->indent_level--;
                codegen_emit_line(gen, "}\n\n");
            }
        }
    }

    // setup()
    codegen_emit_line(gen, "void setup() {");
    gen->indent_level++;
    codegen_emit_line(gen, "Serial.begin(115200);  // ESP32 default baud");
    codegen_emit_line(gen, "analogReadResolution(12);  // ESP32 12-bit ADC (0-4095)");
    codegen_emit_line(gen, "for (int i = 2; i <= 33; i++) pinMode(i, OUTPUT);");
    gen->indent_level--;
    codegen_emit_line(gen, "}\n");

    // loop()
    codegen_emit_line(gen, "void loop() {");
    gen->indent_level++;
    if (program->data.program.main_block) {
        ASTNode *block = program->data.program.main_block;
        if (block->type == NODE_BLOCK) {
            for (int i = 0; i < block->data.block.statement_count; i++) {
                ASTNode *s = block->data.block.statements[i];
                if (s && s->type != NODE_FUNCTION_DEF)
                    esp32_statement(gen, s);
            }
        } else {
            esp32_statement(gen, block);
        }
    }
    gen->indent_level--;
    codegen_emit_line(gen, "}");
}
