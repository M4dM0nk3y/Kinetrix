/* Kinetrix Raspberry Pi Code Generator
 * Target: Raspberry Pi (Python 3 + RPi.GPIO)
 * Output: Python 3 script
 * Run with: python3 robot.py
 * Requirements: pip install RPi.GPIO Adafruit-MCP3008
 */

#include "codegen.h"
#include "ast.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

// Python uses spaces-only indentation (4 spaces)
static void rpi_indent(CodeGen *gen) {
    for (int i = 0; i < gen->indent_level; i++)
        fprintf(gen->output, "    ");
}

static void rpi_emit(CodeGen *gen, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(gen->output, fmt, args);
    va_end(args);
}

static void rpi_emit_line(CodeGen *gen, const char *fmt, ...) {
    rpi_indent(gen);
    va_list args;
    va_start(args, fmt);
    vfprintf(gen->output, fmt, args);
    va_end(args);
    fprintf(gen->output, "\n");
}

// Forward declarations
static void rpi_expression(CodeGen *gen, ASTNode *node);
static void rpi_statement(CodeGen *gen, ASTNode *node);

// ============================================================
// EXPRESSION GENERATION
// ============================================================

static void rpi_expression(CodeGen *gen, ASTNode *node) {
    if (!node) return;

    switch (node->type) {
        case NODE_NUMBER:
            rpi_emit(gen, "%g", node->data.number.value);
            break;
        case NODE_STRING:
            rpi_emit(gen, "\"%s\"", node->data.string.value);
            break;
        case NODE_BOOL:
            rpi_emit(gen, "%s", node->data.boolean.value ? "True" : "False");
            break;
        case NODE_IDENTIFIER:
            rpi_emit(gen, "%s", node->data.identifier.name);
            break;

        case NODE_BINARY_OP: {
            const char *op_str = "?";
            switch (node->data.binary_op.op) {
                case OP_ADD: op_str = "+";   break;
                case OP_SUB: op_str = "-";   break;
                case OP_MUL: op_str = "*";   break;
                case OP_DIV: op_str = "/";   break;
                case OP_MOD: op_str = "%";   break;
                case OP_EQ:  op_str = "==";  break;
                case OP_NEQ: op_str = "!=";  break;
                case OP_LT:  op_str = "<";   break;
                case OP_GT:  op_str = ">";   break;
                case OP_LTE: op_str = "<=";  break;
                case OP_GTE: op_str = ">=";  break;
                case OP_AND: op_str = "and"; break;
                case OP_OR:  op_str = "or";  break;
                default: break;
            }
            rpi_emit(gen, "(");
            rpi_expression(gen, node->data.binary_op.left);
            rpi_emit(gen, " %s ", op_str);
            rpi_expression(gen, node->data.binary_op.right);
            rpi_emit(gen, ")");
            break;
        }

        case NODE_UNARY_OP:
            if (node->data.unary_op.op == OP_NOT) {
                rpi_emit(gen, "not (");
                rpi_expression(gen, node->data.unary_op.operand);
                rpi_emit(gen, ")");
            } else {
                rpi_emit(gen, "-(");
                rpi_expression(gen, node->data.unary_op.operand);
                rpi_emit(gen, ")");
            }
            break;

        case NODE_CALL: {
            // Remap built-in functions to Python equivalents
            const char *name = node->data.call.name;
            if (strcmp(name, "map") == 0 && node->data.call.arg_count == 5) {
                // map(v, fl, fh, tl, th) → int((v - fl) * (th - tl) / (fh - fl) + tl)
                rpi_emit(gen, "int((");
                rpi_expression(gen, node->data.call.args[0]);
                rpi_emit(gen, " - ");
                rpi_expression(gen, node->data.call.args[1]);
                rpi_emit(gen, ") * (");
                rpi_expression(gen, node->data.call.args[4]);
                rpi_emit(gen, " - ");
                rpi_expression(gen, node->data.call.args[3]);
                rpi_emit(gen, ") / (");
                rpi_expression(gen, node->data.call.args[2]);
                rpi_emit(gen, " - ");
                rpi_expression(gen, node->data.call.args[1]);
                rpi_emit(gen, ") + ");
                rpi_expression(gen, node->data.call.args[3]);
                rpi_emit(gen, ")");
            } else if (strcmp(name, "constrain") == 0) {
                rpi_emit(gen, "max(");
                rpi_expression(gen, node->data.call.args[1]);
                rpi_emit(gen, ", min(");
                rpi_expression(gen, node->data.call.args[2]);
                rpi_emit(gen, ", ");
                rpi_expression(gen, node->data.call.args[0]);
                rpi_emit(gen, "))");
            } else if (strcmp(name, "random") == 0) {
                rpi_emit(gen, "random.randint(");
                rpi_expression(gen, node->data.call.args[0]);
                rpi_emit(gen, ", ");
                rpi_expression(gen, node->data.call.args[1]);
                rpi_emit(gen, ")");
            } else if (strcmp(name, "delayMicroseconds") == 0) {
                rpi_emit(gen, "time.sleep(");
                rpi_expression(gen, node->data.call.args[0]);
                rpi_emit(gen, " / 1000000.0)");
            } else {
                // User-defined function call
                rpi_emit(gen, "%s(", name);
                for (int i = 0; i < node->data.call.arg_count; i++) {
                    if (i > 0) rpi_emit(gen, ", ");
                    rpi_expression(gen, node->data.call.args[i]);
                }
                rpi_emit(gen, ")");
            }
            break;
        }

        case NODE_ANALOG_READ:
            // Reads via MCP3008 ADC chip (SPI-connected to RPi)
            rpi_emit(gen, "mcp.read_adc(");
            rpi_expression(gen, node->data.gpio.pin);
            rpi_emit(gen, ")");
            break;

        case NODE_GPIO_READ:
            rpi_emit(gen, "GPIO.input(");
            rpi_expression(gen, node->data.gpio.pin);
            rpi_emit(gen, ")");
            break;

        default:
            rpi_emit(gen, "0  # unsupported expression");
            break;
    }
}

// ============================================================
// STATEMENT GENERATION
// ============================================================

static void rpi_statement(CodeGen *gen, ASTNode *node) {
    if (!node) return;

    switch (node->type) {
        case NODE_VAR_DECL:
            rpi_indent(gen); rpi_emit(gen, "%s", node->data.var_decl.name);
            if (node->data.var_decl.initializer) {
                rpi_emit(gen, " = ");
                rpi_expression(gen, node->data.var_decl.initializer);
            } else {
                rpi_emit(gen, " = 0");
            }
            rpi_emit(gen, "\n");
            break;

        case NODE_ASSIGNMENT:
            rpi_indent(gen);
            rpi_expression(gen, node->data.assignment.target);
            rpi_emit(gen, " = ");
            rpi_expression(gen, node->data.assignment.value);
            rpi_emit(gen, "\n");
            break;

        case NODE_IF:
            rpi_indent(gen); rpi_emit(gen, "if ");
            rpi_expression(gen, node->data.if_stmt.condition);
            rpi_emit(gen, ":\n");
            gen->indent_level++;
            rpi_statement(gen, node->data.if_stmt.then_block);
            gen->indent_level--;
            if (node->data.if_stmt.else_block) {
                // Check if else block is a single if (else-if chain → elif)
                ASTNode *else_b = node->data.if_stmt.else_block;
                if (else_b->type == NODE_BLOCK &&
                    else_b->data.block.statement_count == 1 &&
                    else_b->data.block.statements[0]->type == NODE_IF) {
                    // elif
                    ASTNode *elif = else_b->data.block.statements[0];
                    rpi_indent(gen); rpi_emit(gen, "elif ");
                    rpi_expression(gen, elif->data.if_stmt.condition);
                    rpi_emit(gen, ":\n");
                    gen->indent_level++;
                    rpi_statement(gen, elif->data.if_stmt.then_block);
                    gen->indent_level--;
                    if (elif->data.if_stmt.else_block) {
                        rpi_emit_line(gen, "else:");
                        gen->indent_level++;
                        rpi_statement(gen, elif->data.if_stmt.else_block);
                        gen->indent_level--;
                    }
                } else {
                    rpi_emit_line(gen, "else:");
                    gen->indent_level++;
                    rpi_statement(gen, else_b);
                    gen->indent_level--;
                }
            }
            rpi_emit(gen, "\n");
            break;

        case NODE_WHILE:
            rpi_indent(gen); rpi_emit(gen, "while ");
            rpi_expression(gen, node->data.while_loop.condition);
            rpi_emit(gen, ":\n");
            gen->indent_level++;
            rpi_statement(gen, node->data.while_loop.body);
            gen->indent_level--;
            rpi_emit(gen, "\n");
            break;

        case NODE_REPEAT: {
            int id = gen->loop_counter++;
            rpi_emit_line(gen, "for _i%d in range(int(", id);
            rpi_expression(gen, node->data.repeat_loop.count);
            rpi_emit(gen, ")):\n");
            gen->indent_level++;
            rpi_statement(gen, node->data.repeat_loop.body);
            gen->indent_level--;
            rpi_emit(gen, "\n");
            break;
        }

        case NODE_FOREVER:
            rpi_emit_line(gen, "while True:\n");
            gen->indent_level++;
            rpi_statement(gen, node->data.forever_loop.body);
            gen->indent_level--;
            rpi_emit(gen, "\n");
            break;

        case NODE_BLOCK:
            for (int i = 0; i < node->data.block.statement_count; i++)
                rpi_statement(gen, node->data.block.statements[i]);
            break;

        case NODE_RETURN:
            rpi_indent(gen);
            rpi_emit(gen, "return");
            if (node->data.return_stmt.value) {
                rpi_emit(gen, " ");
                rpi_expression(gen, node->data.return_stmt.value);
            }
            rpi_emit(gen, "\n");
            break;

        case NODE_BREAK:
            rpi_emit_line(gen, "break\n");
            break;

        case NODE_GPIO_WRITE:
            rpi_emit_line(gen, "GPIO.output(");
            rpi_expression(gen, node->data.gpio.pin);
            rpi_emit(gen, ", ");
            rpi_expression(gen, node->data.gpio.value);
            rpi_emit(gen, ")\n");
            break;

        case NODE_ANALOG_WRITE:
            // RPi uses software PWM
            rpi_emit_line(gen, "_pwm_");
            rpi_expression(gen, node->data.gpio.pin);
            rpi_emit(gen, ".ChangeDutyCycle(");
            rpi_expression(gen, node->data.gpio.value);
            rpi_emit(gen, " * 100.0 / 255.0)\n");
            break;

        case NODE_WAIT:
            rpi_emit_line(gen, "time.sleep(");
            rpi_expression(gen, node->data.unary.child);
            rpi_emit(gen, " / 1000.0)\n");
            break;

        case NODE_PRINT:
            rpi_emit_line(gen, "print(");
            rpi_expression(gen, node->data.unary.child);
            rpi_emit(gen, ")\n");
            break;

        case NODE_CALL:
            rpi_indent(gen);
            rpi_expression(gen, node);
            rpi_emit(gen, "\n");
            break;

        case NODE_FUNCTION_DEF:
            if (node->data.function_def.is_extern) {
                rpi_emit_line(gen, "# Extern %s function: %s", 
                              node->data.function_def.extern_lang, 
                              node->data.function_def.name);
                break;
            }
            rpi_indent(gen); rpi_emit(gen, "def %s(", node->data.function_def.name);
            for (int i = 0; i < node->data.function_def.param_count; i++) {
                if (i > 0) rpi_emit(gen, ", ");
                rpi_emit(gen, "%s", node->data.function_def.param_names[i]);
            }
            rpi_emit(gen, "):\n");
            gen->indent_level++;
            rpi_statement(gen, node->data.function_def.body);
            gen->indent_level--;
            rpi_emit(gen, "\n");
            break;

        default:
            break;
    }
}

// ============================================================
// PROGRAM ENTRY POINT
// ============================================================

void codegen_generate_rpi(CodeGen *gen, ASTNode *program) {
    if (!program || program->type != NODE_PROGRAM) return;

    // Python header and imports
    rpi_emit_line(gen, "#!/usr/bin/env python3");
    rpi_emit_line(gen, "# Generated by Kinetrix Compiler (Target: Raspberry Pi)");
    rpi_emit_line(gen, "# Run with: python3 robot.py");
    rpi_emit_line(gen, "# Requirements: pip install RPi.GPIO Adafruit-MCP3008\n");
    rpi_emit_line(gen, "import RPi.GPIO as GPIO");
    rpi_emit_line(gen, "import time");
    rpi_emit_line(gen, "import random");
    rpi_emit_line(gen, "import math");
    rpi_emit_line(gen, "import busio");
    rpi_emit_line(gen, "import digitalio");
    rpi_emit_line(gen, "import board");
    rpi_emit_line(gen, "import adafruit_mcp3xxx.mcp3008 as MCP");
    rpi_emit_line(gen, "from adafruit_mcp3xxx.analog_in import AnalogIn\n");
    rpi_emit_line(gen, "# GPIO setup");
    rpi_emit_line(gen, "GPIO.setmode(GPIO.BCM)");
    rpi_emit_line(gen, "GPIO.setwarnings(False)");
    rpi_emit_line(gen, "for _pin in range(2, 28): GPIO.setup(_pin, GPIO.OUT)\n");
    rpi_emit_line(gen, "# ADC setup (MCP3008 via SPI)");
    rpi_emit_line(gen, "try:");
    rpi_emit_line(gen, "    _spi = busio.SPI(clock=board.SCK, MISO=board.MISO, MOSI=board.MOSI)");
    rpi_emit_line(gen, "    _cs  = digitalio.DigitalInOut(board.CE0)");
    rpi_emit_line(gen, "    mcp  = MCP.MCP3008(_spi, _cs)");
    rpi_emit_line(gen, "except: mcp = None  # No ADC connected\n");

    // Function definitions
    if (program->data.program.main_block &&
        program->data.program.main_block->type == NODE_BLOCK) {
        ASTNode *block = program->data.program.main_block;
        for (int i = 0; i < block->data.block.statement_count; i++) {
            ASTNode *s = block->data.block.statements[i];
            if (s && s->type == NODE_FUNCTION_DEF) {
                rpi_statement(gen, s);
            }
        }
    }

    // Main execution in try/finally for GPIO cleanup
    rpi_emit_line(gen, "try:");
    gen->indent_level++;
    if (program->data.program.main_block) {
        ASTNode *block = program->data.program.main_block;
        if (block->type == NODE_BLOCK) {
            for (int i = 0; i < block->data.block.statement_count; i++) {
                ASTNode *s = block->data.block.statements[i];
                if (s && s->type != NODE_FUNCTION_DEF)
                    rpi_statement(gen, s);
            }
        } else {
            rpi_statement(gen, block);
        }
    }
    gen->indent_level--;
    rpi_emit_line(gen, "except KeyboardInterrupt:");
    rpi_emit_line(gen, "    print(\"\\nStopped by user\")");
    rpi_emit_line(gen, "finally:");
    rpi_emit_line(gen, "    GPIO.cleanup()");
    rpi_emit_line(gen, "    print(\"GPIO cleaned up\")");
}
