/* Kinetrix Raspberry Pi Pico (MicroPython) Code Generator */

#include "codegen.h"
#include "ast.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

static void pico_indent(CodeGen *gen) {
    for (int i = 0; i < gen->indent_level; i++) fprintf(gen->output, "    ");
}
static void pico_emit(CodeGen *gen, const char *fmt, ...) {
    va_list a; va_start(a, fmt); vfprintf(gen->output, fmt, a); va_end(a);
}
static void pico_emit_line(CodeGen *gen, const char *fmt, ...) {
    pico_indent(gen);
    va_list a; va_start(a, fmt); vfprintf(gen->output, fmt, a); va_end(a);
    fprintf(gen->output, "\n");
}

static void pico_expr(CodeGen *gen, ASTNode *node);
static void pico_stmt(CodeGen *gen, ASTNode *node);

static void pico_expr(CodeGen *gen, ASTNode *node) {
    if (!node) return;
    switch (node->type) {
        case NODE_NUMBER:     pico_emit(gen, "%g", node->data.number.value); break;
        case NODE_STRING:     pico_emit(gen, "\"%s\"", node->data.string.value); break;
        case NODE_BOOL:       pico_emit(gen, "%s", node->data.boolean.value ? "True" : "False"); break;
        case NODE_IDENTIFIER: pico_emit(gen, "%s", node->data.identifier.name); break;
        case NODE_BINARY_OP: {
            const char *op = "+";
            switch (node->data.binary_op.op) {
                case OP_ADD: op="+"; break; case OP_SUB: op="-"; break;
                case OP_MUL: op="*"; break; case OP_DIV: op="/"; break;
                case OP_MOD: op="%"; break; case OP_EQ:  op="=="; break;
                case OP_NEQ: op="!="; break; case OP_LT: op="<"; break;
                case OP_GT:  op=">"; break; case OP_LTE: op="<="; break;
                case OP_GTE: op=">="; break; case OP_AND: op="and"; break;
                case OP_OR:  op="or"; break; default: break;
            }
            pico_emit(gen, "(");
            pico_expr(gen, node->data.binary_op.left);
            pico_emit(gen, " %s ", op);
            pico_expr(gen, node->data.binary_op.right);
            pico_emit(gen, ")");
            break;
        }
        case NODE_UNARY_OP:
            pico_emit(gen, node->data.unary_op.op == OP_NOT ? "not (" : "-(");
            pico_expr(gen, node->data.unary_op.operand);
            pico_emit(gen, ")");
            break;
        case NODE_CALL: {
            const char *nm = node->data.call.name;
            if (strcmp(nm, "map") == 0 && node->data.call.arg_count == 5) {
                pico_emit(gen, "int((");
                pico_expr(gen, node->data.call.args[0]); pico_emit(gen, " - ");
                pico_expr(gen, node->data.call.args[1]); pico_emit(gen, ") * (");
                pico_expr(gen, node->data.call.args[4]); pico_emit(gen, " - ");
                pico_expr(gen, node->data.call.args[3]); pico_emit(gen, ") // (");
                pico_expr(gen, node->data.call.args[2]); pico_emit(gen, " - ");
                pico_expr(gen, node->data.call.args[1]); pico_emit(gen, ") + ");
                pico_expr(gen, node->data.call.args[3]); pico_emit(gen, ")");
            } else if (strcmp(nm, "constrain") == 0) {
                pico_emit(gen, "max(");
                pico_expr(gen, node->data.call.args[1]);
                pico_emit(gen, ", min(");
                pico_expr(gen, node->data.call.args[2]);
                pico_emit(gen, ", ");
                pico_expr(gen, node->data.call.args[0]);
                pico_emit(gen, "))");
            } else if (strcmp(nm, "delayMicroseconds") == 0) {
                pico_emit(gen, "utime.sleep_us(int(");
                pico_expr(gen, node->data.call.args[0]);
                pico_emit(gen, "))");
            } else {
                pico_emit(gen, "%s(", nm);
                for (int i = 0; i < node->data.call.arg_count; i++) {
                    if (i > 0) pico_emit(gen, ", ");
                    pico_expr(gen, node->data.call.args[i]);
                }
                pico_emit(gen, ")");
            }
            break;
        }
        case NODE_ANALOG_READ:
            pico_emit(gen, "ADC("); pico_expr(gen, node->data.gpio.pin);
            pico_emit(gen, ").read_u16() >> 6");  /* scale 0-65535 → 0-1023 */
            break;
        case NODE_GPIO_READ:
            pico_emit(gen, "Pin("); pico_expr(gen, node->data.gpio.pin);
            pico_emit(gen, ", Pin.IN).value()");
            break;
        default: pico_emit(gen, "0"); break;
    }
}

static void pico_stmt(CodeGen *gen, ASTNode *node) {
    if (!node) return;
    switch (node->type) {
        case NODE_VAR_DECL:
            pico_indent(gen);
            pico_emit(gen, "%s = ", node->data.var_decl.name);
            if (node->data.var_decl.initializer) pico_expr(gen, node->data.var_decl.initializer);
            else pico_emit(gen, "0");
            pico_emit(gen, "\n");
            break;
        case NODE_ASSIGNMENT:
            pico_indent(gen);
            pico_expr(gen, node->data.assignment.target);
            pico_emit(gen, " = ");
            pico_expr(gen, node->data.assignment.value);
            pico_emit(gen, "\n");
            break;
        case NODE_IF:
            pico_indent(gen);
            pico_emit(gen, "if ");
            pico_expr(gen, node->data.if_stmt.condition);
            pico_emit(gen, ":\n");
            gen->indent_level++;
            pico_stmt(gen, node->data.if_stmt.then_block);
            gen->indent_level--;
            if (node->data.if_stmt.else_block) {
                pico_emit_line(gen, "else:\n");
                gen->indent_level++;
                pico_stmt(gen, node->data.if_stmt.else_block);
                gen->indent_level--;
            }
            pico_emit(gen, "\n");
            break;
        case NODE_WHILE:
            pico_indent(gen);
            pico_emit(gen, "while ");
            pico_expr(gen, node->data.while_loop.condition);
            pico_emit(gen, ":\n");
            gen->indent_level++;
            pico_stmt(gen, node->data.while_loop.body);
            gen->indent_level--;
            pico_emit(gen, "\n");
            break;
        case NODE_REPEAT: {
            int id = gen->loop_counter++;
            pico_indent(gen);
            pico_emit(gen, "for _i%d in range(int(", id);
            pico_expr(gen, node->data.repeat_loop.count);
            pico_emit(gen, ")):\n");
            gen->indent_level++;
            pico_stmt(gen, node->data.repeat_loop.body);
            gen->indent_level--;
            break;
        }
        case NODE_FOREVER:
            pico_indent(gen); pico_emit(gen, "while True:\n");
            gen->indent_level++;
            pico_stmt(gen, node->data.forever_loop.body);
            gen->indent_level--;
            break;
        case NODE_BLOCK:
            for (int i = 0; i < node->data.block.statement_count; i++)
                pico_stmt(gen, node->data.block.statements[i]);
            break;
        case NODE_RETURN:
            pico_indent(gen); pico_emit(gen, "return");
            if (node->data.return_stmt.value) { pico_emit(gen, " "); pico_expr(gen, node->data.return_stmt.value); }
            pico_emit(gen, "\n");
            break;
        case NODE_BREAK: pico_emit_line(gen, "break\n"); break;
        case NODE_GPIO_WRITE:
            pico_indent(gen); pico_emit(gen, "Pin(");
            pico_expr(gen, node->data.gpio.pin);
            pico_emit(gen, ", Pin.OUT).value(");
            pico_expr(gen, node->data.gpio.value);
            pico_emit(gen, ")\n");
            break;
        case NODE_ANALOG_WRITE:
            pico_indent(gen); pico_emit(gen, "_pwm = PWM(Pin(");
            pico_expr(gen, node->data.gpio.pin);
            pico_emit(gen, ")); _pwm.duty_u16(int(");
            pico_expr(gen, node->data.gpio.value);
            pico_emit(gen, " * 257))\n");
            break;
        case NODE_WAIT:
            pico_indent(gen); pico_emit(gen, "utime.sleep_ms(int(");
            pico_expr(gen, node->data.unary.child);
            pico_emit(gen, "))\n");
            break;
        case NODE_PRINT:
            pico_indent(gen); pico_emit(gen, "print(");
            pico_expr(gen, node->data.unary.child);
            pico_emit(gen, ")\n");
            break;
        case NODE_CALL:
            pico_indent(gen); pico_expr(gen, node); pico_emit(gen, "\n");
            break;
        case NODE_FUNCTION_DEF:
            if (node->data.function_def.is_extern) {
                pico_indent(gen); pico_emit(gen, "# Extern %s function: %s\n", 
                                            node->data.function_def.extern_lang, 
                                            node->data.function_def.name);
                break;
            }
            pico_indent(gen); pico_emit(gen, "def %s(", node->data.function_def.name);
            for (int i = 0; i < node->data.function_def.param_count; i++) {
                if (i > 0) pico_emit(gen, ", ");
                pico_emit(gen, "%s", node->data.function_def.param_names[i]);
            }
            pico_emit(gen, "):\n");
            gen->indent_level++;
            pico_stmt(gen, node->data.function_def.body);
            gen->indent_level--;
            pico_emit(gen, "\n");
            break;
        default: break;
    }
}

void codegen_generate_pico(CodeGen *gen, ASTNode *program) {
    if (!program || program->type != NODE_PROGRAM) return;
    pico_emit_line(gen, "# Generated by Kinetrix Compiler (Target: Raspberry Pi Pico / MicroPython)");
    pico_emit_line(gen, "# Flash: Thonny IDE  OR  mpremote copy robot.py :main.py\n");
    pico_emit_line(gen, "from machine import Pin, ADC, PWM, I2C, UART");
    pico_emit_line(gen, "import utime, math\n");

    ASTNode *block = program->data.program.main_block;
    if (block && block->type == NODE_BLOCK) {
        for (int i = 0; i < block->data.block.statement_count; i++)
            if (block->data.block.statements[i]->type == NODE_FUNCTION_DEF)
                pico_stmt(gen, block->data.block.statements[i]);
        for (int i = 0; i < block->data.block.statement_count; i++)
            if (block->data.block.statements[i]->type != NODE_FUNCTION_DEF)
                pico_stmt(gen, block->data.block.statements[i]);
    } else if (block) {
        pico_stmt(gen, block);
    }
}
